#version 460

out vec4 color;

in vec3 fragPos;
in vec3 _normal;

in vec2 pass_texCoord;
in vec4 _fragPosLightSpace;
layout(binding = 0) uniform sampler2D albedoTexture;
layout(binding = 1) uniform sampler2D normalTexture;
layout(binding = 2) uniform sampler2D metallicTexture;
layout(binding = 3) uniform sampler2D roughnessTexture;
layout(binding = 4) uniform sampler2D heightTexture;
layout(binding = 5) uniform sampler2D aoTexture;

layout(binding = 6) uniform sampler2D shadowMap;
layout(binding = 7) uniform samplerCube skybox;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec2 uViewportSize;
uniform vec3 uPosition;
uniform vec3 uDirection;
uniform vec3 uUp;
uniform float uFOV;
uniform float uTime;
uniform int uSamples;

//uniform float roughness;
//uniform float metalness;
float roughness;
float metalness;
uniform vec3 emmisivity;
uniform vec3 reflectance;

uniform int isPicked;

const float kPi = 3.14159265;
const float kShininess = 16.0;

float infinity = 1.0 / 0.0;
vec2 texCoords;

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{
    // количество слоев глубины
    const float minLayers = 8.0;
    const float maxLayers = 32.0;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));
    // размер каждого слоя
    float layerDepth = 1.0 / numLayers;
    // глубина текущего слоя
    float currentLayerDepth = 0.0;
    // величина шага смещения текстурных координат на каждом слое
    // расчитывается на основе вектора P
    vec2 P = viewDir.xy * 0.1;
    vec2 deltaTexCoords = P / numLayers;

    vec2  currentTexCoords     = texCoords;
    float currentDepthMapValue = texture(heightTexture, currentTexCoords).r;

    while(currentLayerDepth < currentDepthMapValue)
    {
        // смещаем текстурные координаты вдоль вектора P
        currentTexCoords -= deltaTexCoords;
        // делаем выборку из карты глубин в текущих текстурных координатах
        currentDepthMapValue = texture(heightTexture, currentTexCoords).r;
        // рассчитываем глубину следующего слоя
        currentLayerDepth += layerDepth;
    }
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    // находим значения глубин до и после нахождения пересечения
    // для использования в линейной интерполяции
    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(heightTexture, prevTexCoords).r - currentLayerDepth + layerDepth;

    // интерполяция текстурных координат
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords;
}

vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(normalTexture, pass_texCoord).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(fragPos);
    vec3 Q2  = dFdy(fragPos);
    vec2 st1 = dFdx(pass_texCoord);
    vec2 st2 = dFdy(pass_texCoord);

    vec3 N   = normalize(_normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

float ShadowCalculation()
{
    vec3 projCoords = _fragPosLightSpace.xyz / _fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
    vec3 normal = normalize(_normal);
    vec3 lightDir = normalize(lightPos - fragPos);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    bias = 0.0005;
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    if(projCoords.z > 1.0)
    shadow = 0.0;

    return 1 - shadow;
}

float when_gt(float x, float y) {
    return max(sign(x - y), 0.0);
}

const float PI = 3.14159265358979323846264338327950288;
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);

    return GeometrySchlickGGX(NdotL, roughness) * GeometrySchlickGGX(NdotV, roughness);
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main()
{
    vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);
    float ambientStrength = 0.45f;
    vec3 ambientLighting = ambientStrength * lightColor;
    vec3 norm = normalize(_normal);
    norm = getNormalFromMap();
    vec3 lightDirection = normalize(lightPos - fragPos);

    vec3 viewDir = normalize(viewPos - fragPos);
    //texCoords = ParallaxMapping(pass_texCoord,  viewDir);
    texCoords = pass_texCoord;

    roughness = texture(roughnessTexture, texCoords).r;
    metalness = texture(metallicTexture, texCoords).r;
    float ao = texture(aoTexture, texCoords).r;
    vec3 albedoMesh = pow(texture(albedoTexture, texCoords).rgb, vec3(2.2));
    //albedoMesh = vec3(1.0, 1.0, 1.0);
    const float F_DI = 0.04;
    vec3 F0 = mix(vec3(F_DI), albedoMesh, metalness);

    const float kEnergyConservation = ( 8.0 + kShininess ) / ( 8.0 * kPi );
    vec3 halfwayDir = normalize(lightDirection + viewDir);

    float distance = length(lightPos - fragPos);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = vec3(1.0) * attenuation;

    float NDF = DistributionGGX(norm, halfwayDir, roughness);
    float G   = GeometrySmith(norm, viewDir, lightDirection, roughness);
    vec3 F    = fresnelSchlick(clamp(dot(halfwayDir, viewDir), 0.0, 1.0), F0);

    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(norm, viewDir), 0.0) * max(dot(norm, lightDirection), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metalness;
    float NdotL = max(dot(norm, lightDirection), 0.0);
    vec3 Lo = (kD * albedoMesh / PI + specular) * NdotL;

    vec3 ambient = vec3(0.03) * albedoMesh * ao;
    color = vec4(ambient + Lo, 1.0);

    // HDR tonemapping
    color = color / (color + vec4(1.0));
    // gamma correct
    color = vec4(emmisivity, 1.0) + pow(color, vec4(1.0/2.2));
    if (isPicked == 1) color = vec4(1.0, 0.0, 0.0, 1.0);
    //color = vec4(PBR(F0, viewDir, lightDirection, halfwayDir, albedoMesh, norm), 1.0);
    //color = texture(roughnessTexture, pass_texCoord).rgba;
}

