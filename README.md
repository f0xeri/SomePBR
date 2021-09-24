# Интерактивная система по созданию дизайна нишевого шкафа и его раскроя

# Сборка проекта с использованием MinGW

1. Установить CMake версии 3.0 и выше
2. Установить MinGW-Get и через него установить пакет mingw-core или установить MinGW-w64, если компьютер это поддерживает
3. Установить Git версии 2.13 и выше (чтобы рекурсивное клонирование работало)
4. Открыть консоль
5. git clone --recursive https://github.com/f0xeri/WardrobeDesigner.git
6. cd FoxEngine
7. mkdir build
8. cmake -S .. -B . -G "MinGW Makefiles"
9. mingw32-make