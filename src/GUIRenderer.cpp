//
// Created by Yaroslav on 05.04.2021.
//

#include <Object/WardrobeElements/WardrobeVerticalElement.hpp>
#include <Object/WardrobeElements/WardrobeHorizontalShelf.hpp>
#include <Logger.hpp>
#include <imgui_internal.h>
#include "GUIRenderer.hpp"
#include "Algorythm.h"


GUIRenderer::GUIRenderer(GLFWwindow *window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    //ImGuiIO &io = ImGui::GetIO();
    ImFontConfig font_config;
    font_config.OversampleH = 2; //or 2 is the same
    font_config.OversampleV = 1;
    font_config.PixelSnapH = 1;

    static const ImWchar ranges[] =
            {
                    0x0020, 0x00FF, // Basic Latin + Latin Supplement
                    0x0400, 0x044F, // Cyrillic
                    0,
            };
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("res/fonts/NotoSans-Regular.ttf", 18.0f, &font_config, ranges);
    ImGui::StyleColorsLight();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    const char* glslVersion = "#version 330";
    ImGui_ImplOpenGL3_Init(glslVersion);
}

void GUIRenderer::renderDebugInfo(State *state)
{
    ImGuiWindowFlags window_flags = 0;
    window_flags |= ImGuiWindowFlags_NoBackground;
    window_flags |= ImGuiWindowFlags_NoTitleBar;
    //window_flags |= ImGuiWindowFlags_NoResize;
    ImGui::SetNextWindowPos({0, 0}, ImGuiCond_Once);
    ImGui::Begin("Debug222", &state->showDebug, window_flags);
    std::stringstream debugSS;
    debugSS << "Camera position:\n" << "X: " << state->camera->pos.x << " Y: " << state->camera->pos.y << " Z: "
            << state->camera->pos.z << "\n\n";
    ImGui::Text("%s", debugSS.str().c_str());
    /*if (ImGui::Button("Close (F3)"))
        show_debug = false;*/
    std::stringstream resSS;

    ImGui::Text("%s", resSS.str().c_str());
    ImGui::End();
}

std::string current_item;
int selected_item_index;
void GUIRenderer::renderSettings(State *state)
{
    float settingsWinX, settingsWinY, settingsWinW, settingsWinH;
    float objWinX, objWinY, objWinW, objWinH;

    settingsWinX = objWinX = Window::_width - Window::_width / 6.0f;
    settingsWinY = menuBarHeight;
    settingsWinW = objWinW = Window::_width / 6.0f;
    settingsWinH = state->pickedObject == -1 ? Window::_height + menuBarHeight : Window::_height - 2 * Window::_height / 3 + menuBarHeight;

    objWinY = Window::_height - 2 * Window::_height / 3 + menuBarHeight;
    objWinH = Window::_height - Window::_height / 3 - menuBarHeight;

    ImGuiWindowFlags window_flags = 0;
    ImGui::SetNextWindowPos({settingsWinX, settingsWinY}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({settingsWinW, settingsWinH});
    window_flags |= ImGuiWindowFlags_NoScrollbar;
    window_flags |= ImGuiWindowFlags_NoCollapse;
    window_flags |= ImGuiWindowFlags_NoResize;

    ImGui::Begin("Настройки", nullptr, window_flags);
    ImGui::Text("Стенки шкафа");
    ImGui::Checkbox("Основание", &state->wardrobeGenerator->bottomSide->enabled);
    ImGui::Checkbox("Задняя", &state->wardrobeGenerator->backSide->enabled);
    ImGui::Checkbox("Верхняя", &state->wardrobeGenerator->topSide->enabled);
    ImGui::Checkbox("Левая", &state->wardrobeGenerator->leftSide->enabled);
    ImGui::Checkbox("Правая", &state->wardrobeGenerator->rightSide->enabled);

    if (state->pickedObject != -1)
    {
        ImGui::SetNextWindowPos({objWinX, objWinY}, ImGuiCond_Always);
        ImGui::SetNextWindowSize({objWinW, objWinH});
        std::stringstream stringstream;
        stringstream << "Настройки детали #" << state->pickedObject;

        ImGui::Begin(stringstream.str().c_str(), nullptr, window_flags);
        ImGui::Text("Позиция");

        float x = from_mm(state->scene->objects[state->pickedObject]->position.x);
        float y = from_mm(state->scene->objects[state->pickedObject]->position.y);
        float z = from_mm(state->scene->objects[state->pickedObject]->position.z);

        ImGui::InputFloat("pos x", &x, 1.001f);
        ImGui::InputFloat("pos y", &y, 1.001f);
        ImGui::InputFloat("pos z", &z, 1.001f);

        // If picked object is cube or derived of cube we can get and set size
        Cube *object = dynamic_cast<Cube *>(state->scene->objects[state->pickedObject]);
        if (object != nullptr)
        {
            float szx = from_mm(object->size.x);
            float szy = from_mm(object->size.y);
            float szz = from_mm(object->size.z);

            ImGui::Text("Размер");
            ImGui::InputFloat("size x", &szx, 1.0f);
            ImGui::InputFloat("size y", &szy, 1.0f);
            ImGui::InputFloat("size z", &szz, 1.0f);

            object->size.x = to_mm(szx);
            object->size.y = to_mm(szy);
            object->size.z = to_mm(szz);
        }

        ImGui::Text("Текстура");
        current_item = state->scene->objects[state->pickedObject]->texture->name;
        ImGui::SetNextItemWidth(settingsWinW - 14);

        float prevItemSpacingY = ImGui::GetStyle().ItemSpacing.y;
        ImGui::GetStyle().ItemSpacing.y = 0;
        /* HACK: I don't know how to set Combo height properly, ImGui calculates height like "fontsize + framepadding * 2"
         * so i just change framepadding.y for Combo and then revert it back */
        ImGui::GetStyle().FramePadding.y = 7;
        if(ImGui::BeginCombo("##texture", (std::string("           ") + current_item).c_str()))
        {
            ImGui::GetStyle().FramePadding.y = 3;
            for(const auto& [name, texture] : state->wardrobeTextures)
            {
                bool is_selected = (current_item == name);
                auto drawList = ImGui::GetWindowDrawList();
                ImGuiSelectableFlags flags = 0;
                if(ImGui::Selectable((std::string("           ") + name).c_str(), is_selected, flags, {settingsWinW, 32}))
                {
                    //selected_item_index = n;
                    current_item = name;
                    state->scene->objects[state->pickedObject]->texture = state->wardrobeTextures.at(current_item);
                }
                auto rect_min = ImGui::GetItemRectMin();
                auto rect_max = ImGui::GetItemRectMax();
                rect_min.x += 2;
                rect_max.x = rect_min.x + 32;
                rect_max.y = rect_min.y + 32;
                drawList->AddImage(reinterpret_cast<ImTextureID>(state->wardrobeTextures.at(name)->texture), rect_min, rect_max, ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255));
                if(is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        else
        {
            auto rect_min = ImGui::GetItemRectMin();
            auto rect_max = ImGui::GetItemRectMax();
            rect_max.x = rect_min.x + 32;
            rect_max.y = rect_min.y + 32;
            auto drawList = ImGui::GetWindowDrawList();
            drawList->AddImage(reinterpret_cast<ImTextureID>(state->wardrobeTextures.at(current_item)->texture), rect_min, rect_max, ImVec2(0,0), ImVec2(1,1), IM_COL32(255,255,255,255));
        }
        ImGui::GetStyle().FramePadding.y = 3;
        ImGui::GetStyle().ItemSpacing.y = prevItemSpacingY;

        ImGui::Spacing();
        ImGui::Spacing();
        bool disabled = false;
        if (dynamic_cast<WardrobeEdge*>(object))
        {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            disabled = true;
        }
        if (ImGui::Button("Удалить элемент", {settingsWinW - 14, 32}))
        {
            auto deletable_object = (WardrobeHorizontalShelf*)state->scene->objects[state->pickedObject];

            auto must_be_deleted = state->cm.getClustersToDelete(deletable_object->c);

            while (!must_be_deleted.empty())
            {
                auto c = must_be_deleted.back();
                must_be_deleted.pop_back();
                state->scene->objects.erase(
                    std::remove(state->scene->objects.begin(), state->scene->objects.end(), c->linked_object),
                    state->scene->objects.end());
                delete c;
            }


            state->scene->objects.erase(state->scene->objects.begin() + state->pickedObject);
            state->pickedObject = -1;
        }
        else
        {
            if (!FLOAT_EQUAL(to_mm(x), state->scene->objects[state->pickedObject]->position.x) ||
                !FLOAT_EQUAL(to_mm(y), state->scene->objects[state->pickedObject]->position.y) ||
                !FLOAT_EQUAL(to_mm(z), state->scene->objects[state->pickedObject]->position.z))
            {
                state->scene->objects[state->pickedObject]->start_move(state);
                state->scene->objects[state->pickedObject]->position.x = to_mm(x);
                state->scene->objects[state->pickedObject]->position.y = to_mm(y);
                state->scene->objects[state->pickedObject]->position.z = to_mm(z);
                state->scene->objects[state->pickedObject]->end_move();
            }
        }

        if (disabled)
        {
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
        }

        ImGui::End();
    }
    ImGui::End();
}


void GUIRenderer::renderWardrobeMenu(State *state)
{
    float addObjWinX, addObjWinY, addObjWinW, addObjWinH;

    addObjWinX = 0;
    addObjWinY = menuBarHeight;
    addObjWinW = Window::_width / 6.0f;
    addObjWinH = Window::_height / 2 - menuBarHeight;


    ImGuiWindowFlags window_flags = 0;
    ImGui::SetNextWindowPos({addObjWinX, addObjWinY}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({addObjWinW, addObjWinH});
    window_flags |= ImGuiWindowFlags_NoScrollbar;
    window_flags |= ImGuiWindowFlags_NoCollapse;
    window_flags |= ImGuiWindowFlags_NoResize;
    ImGui::Begin("Добавление детали", nullptr, window_flags);
    std::vector<std::string> items;
    items.emplace_back("Вертикальная перегородка");
    items.emplace_back("Горизонтальная полка");
    int i = 0;
    for (auto & item : items) {
        if (ImGui::Button(item.c_str(), {addObjWinW - 18, 32}) && !state->cursorLocked)
        {
            switch (i)
            {
                case 0:
                {
                    state->toInsert = 1;
                    state->pickedObject = -1;
                    break;
                }
                case 1:
                {
                    state->toInsert = 2;
                    state->pickedObject = -1;
                    break;
                }
                default:
                    state->toInsert = 0;
                    break;
            }
        }
        i++;
    }
    ImGui::End();
}


std::vector<std::vector<int>> convertDataForAlg(State *state)
{
    std::vector<std::vector<int>> ret;
    for (auto& object : state->scene->objects)
    {
        if ((dynamic_cast<WardrobeEdge*>(object) || dynamic_cast<WardrobeHorizontalShelf*>(object) || dynamic_cast<WardrobeVerticalElement*>(object)) && object->enabled)
        {
            std::vector<int> rect { 0, 0, 0, 0 };
            Cube* obj = dynamic_cast<Cube*>(object);

            bool f = false;
            if (!FLOAT_EQUAL(obj->size.x, state->wardrobeGenerator->boardThickness))
            {
                rect.push_back(obj->size.x * 320.0f);
                std::cout << obj->size.x * 320.0f << " ";
            }
            if (!FLOAT_EQUAL(obj->size.y, state->wardrobeGenerator->boardThickness))
            {
                rect.push_back(obj->size.y * 320.0f);
                std::cout << obj->size.y * 320.0f << " ";
            }
            if (!FLOAT_EQUAL(obj->size.z, state->wardrobeGenerator->boardThickness))
            {
                rect.push_back(obj->size.z * 320.0f);
                std::cout << obj->size.z * 320.0f << " ";
            }

            std::cout << "\n";
            if (rect.size() <= 6)
                ret.push_back(rect);
        }
    }
    return ret;
}

int width = 0, height = 0, depth = 0, boardThickness = 0, baseHeight = 0;
bool exportImage = false;
unsigned int dspCount = 0;
unsigned int selectedDsp = 0;
std::string currentDsp;
std::vector<std::string> dspNames;
std::vector<Texture*> dspTextures;
void GUIRenderer::renderMenuBar(State *state)
{
    bool newProject = false;

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Файл"))
        {
            if (ImGui::MenuItem("Новый проект"))
            {
                newProject = true;
            }
            if (ImGui::MenuItem("Открыть проект"))
            {

            }
            if (ImGui::MenuItem("Сохранить проект"))
            {

            }
            if (ImGui::MenuItem("Экспортировать развертку"))
            {
                exportImage = true;
                auto data = convertDataForAlg(state);
                auto res = algorythm(data, 2750, 1830, state->wardrobeGenerator->boardThickness);
                dspCount = res.second;
                for (int i = 0; i < dspCount; i++)
                {
                    dspTextures.push_back(Texture::createEmptyTexture(2750, 1830, 3, std::string("img" + std::to_string(i)).c_str()));
                    for (unsigned int p = 0; p < 2750 * 1830 * 3; p++)
                    {
                        dspTextures[i]->data[p] = 255;
                    }
                }
                for (auto &rect : res.first)
                {
                    for (unsigned int i = rect.x; i <= rect.x + rect.width; i++)
                    {
                        unsigned int location = (i + (rect.y * 2750)) * 3;
                        dspTextures[rect.dspN]->data[location] = 0;
                        dspTextures[rect.dspN]->data[location + 1] = 0;
                        dspTextures[rect.dspN]->data[location + 2] = 0;
                        location = (i + ((rect.y + rect.height) * 2750)) * 3;
                        dspTextures[rect.dspN]->data[location] = 0;
                        dspTextures[rect.dspN]->data[location + 1] = 0;
                        dspTextures[rect.dspN]->data[location + 2] = 0;
                    }
                    for (unsigned int i = rect.y; i <= rect.y + rect.height; i++)
                    {
                        unsigned int location = ((rect.x + rect.width) + (i * 2750)) * 3;
                        dspTextures[rect.dspN]->data[location] = 0;
                        dspTextures[rect.dspN]->data[location + 1] = 0;
                        dspTextures[rect.dspN]->data[location + 2] = 0;
                        location = (rect.x + (i * 2750)) * 3;
                        dspTextures[rect.dspN]->data[location] = 0;
                        dspTextures[rect.dspN]->data[location + 1] = 0;
                        dspTextures[rect.dspN]->data[location + 2] = 0;
                    }
                    dspTextures[rect.dspN]->updateTexture();
                }
            }
            ImGui::EndMenu();
        }
        if (newProject)
        {
            state->scene->objects.clear();
            ImGui::OpenPopup("Создание нового проекта");
        }

        float newProjWinX, newProjWinY, newProjWinW, newProjWinH;

        newProjWinW = Window::_width / 3.0;
        newProjWinH = Window::_height / 3.0;
        newProjWinX = Window::_width / 2.0 - newProjWinW / 2.0;
        newProjWinY = Window::_height / 2.0 - newProjWinH / 2.0;

        ImGui::SetNextWindowPos({newProjWinX, newProjWinY}, ImGuiCond_Once);
        ImGui::SetNextWindowSize({newProjWinW, newProjWinH}, ImGuiCond_Once);

        if (exportImage)
        {
            ImGui::Begin("Экспорт развертки", nullptr, ImGuiWindowFlags_HorizontalScrollbar);
            state->cursorLocked = true;
            dspNames.clear();
            for (int i = 0; i < dspCount; i++)
            {
                dspNames.push_back("Лист ДСП #" + std::to_string(i + 1));
            }
            if (currentDsp == "") currentDsp = dspNames[0];
            if (ImGui::BeginCombo("Выбор листа ДСП для просмотра развертки", currentDsp.c_str()))
            {
                for (unsigned int i = 0; i < dspCount; i++)
                {
                    bool isSelected = (currentDsp == dspNames[i]);
                    if (ImGui::Selectable(dspNames[i].c_str(), isSelected))
                    {
                        currentDsp = dspNames[i];
                        selectedDsp = i;
                    }
                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            ImGui::Image(reinterpret_cast<ImTextureID>(dspTextures[selectedDsp]->texture), {float(dspTextures[selectedDsp]->width) / 2, float(dspTextures[selectedDsp]->height) / 2});
            if (ImGui::Button("Отмена"))
            {
                state->cursorLocked = false;
                exportImage = false;
            }
            ImGui::End();
        }

        newProjWinW = Window::_width / 3.0;
        newProjWinH = Window::_height / 3.0;
        newProjWinX = Window::_width / 2.0 - newProjWinW / 2.0;
        newProjWinY = Window::_height / 2.0 - newProjWinH / 2.0;

        ImGui::SetNextWindowPos({newProjWinX, newProjWinY}, ImGuiCond_Always);
        ImGui::SetNextWindowSize({newProjWinW, newProjWinH});
        if (ImGui::BeginPopupModal("Создание нового проекта"))
        {
            ImGui::InputInt("Ширина шкафа", &width);
            ImGui::InputInt("Высота шкафа", &height);
            ImGui::InputInt("Глубина шкафа", &depth);
            ImGui::InputInt("Толщина доски", &boardThickness);
            ImGui::InputInt("Высота основания", &baseHeight);
            if (ImGui::Button("Отмена"))
                ImGui::CloseCurrentPopup();
            ImGui::SameLine();
            if (ImGui::Button("Создать"))
            {
                ImGui::CloseCurrentPopup();
                Cube *floor = new Cube({-10, -1, -10}, {20, 1, 20});
                floor->texture = state->wardrobeTextures.at("res/textures/floor.jpg");
                floor->texScaleX = floor->texScaleY = 8;
                floor->generateVAO();
                state->scene->addObject(floor);

                state->wardrobeGenerator = new WardrobeGenerator({0, 0, 0}, to_mm(width), to_mm(height), to_mm(depth), to_mm(boardThickness), to_mm(baseHeight), state->wardrobeTextures.at("res/textures/woodTexture.jpg"));
                state->scene->addObject(state->wardrobeGenerator->bottomSide);
                state->scene->addObject(state->wardrobeGenerator->backSide);
                state->scene->addObject(state->wardrobeGenerator->topSide);
                state->scene->addObject(state->wardrobeGenerator->leftSide);
                state->scene->addObject(state->wardrobeGenerator->rightSide);
            }
            ImGui::EndPopup();
        }


        if (ImGui::BeginMenu("Справка"))
        {
            if (ImGui::MenuItem("Инструкция"))
            {

            }
            if (ImGui::MenuItem("О программе"))
            {

            }
            ImGui::EndMenu();
        }
        menuBarHeight = ImGui::GetWindowHeight();
        ImGui::EndMainMenuBar();
    }
}


void GUIRenderer::renderElementsList(State *state)
{
    float objListWinX, objListWinY, objListWinW, objListWinH;
    objListWinX = 0;
    objListWinY = Window::_height / 2 - menuBarHeight;
    objListWinW = Window::_width / 6.0f;
    objListWinH = Window::_height / 2 + menuBarHeight;

    ImGuiWindowFlags window_flags = 0;
    ImGui::SetNextWindowPos({objListWinX, objListWinY}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({objListWinW, objListWinH});
    window_flags |= ImGuiWindowFlags_NoScrollbar;
    window_flags |= ImGuiWindowFlags_NoCollapse;
    window_flags |= ImGuiWindowFlags_NoResize;
    ImGui::Begin("Список элементов шкафа", nullptr, window_flags);

    int i = 0;
    for (auto &object : state->scene->objects)
    {
        if (dynamic_cast<WardrobeEdge*>(object))
        {
            if (dynamic_cast<WardrobeEdge*>(object)->enabled)
            {
                if (ImGui::Selectable((std::string("Составная часть шкафа #") + std::to_string(i)).c_str()) && !state->cursorLocked)
                {
                    state->pickedObject = i;
                }
            }
        }
        else if (dynamic_cast<WardrobeHorizontalShelf*>(object))
        {
            if (ImGui::Selectable((std::string("Горизонтальная полка #") + std::to_string(i)).c_str()) && !state->cursorLocked)
            {
                state->pickedObject = i;
            }
        }
        else if (dynamic_cast<WardrobeVerticalElement*>(object))
        {
            if (ImGui::Selectable((std::string("Вертикальная перегородка #") + std::to_string(i)).c_str()) && !state->cursorLocked)
            {
                state->pickedObject = i;
            }
        }
        i++;
    }
    ImGui::End();
}

void GUIRenderer::render(State *state)
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    //if (state->showDebug) renderDebugInfo(state);
    renderMenuBar(state);
    renderSettings(state);
    renderWardrobeMenu(state);
    renderElementsList(state);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    ImGui::EndFrame();
}

void GUIRenderer::renderDebugString(const std::string &string, float x, float y) {
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos({x, y});
        ImGui::Begin("");
        ImGui::Text("%s", string.c_str());
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        ImGui::EndFrame();
    }
}
