#pragma once
#include "long_march.h"

// Simple material structure for ray tracing
struct Material {
    glm::vec3 base_color;
    float roughness;
    float metallic;
    float transmission;  // 0 = opaque, 1 = fully transparent
    glm::vec3 emission;
    float ior;           // Index of refraction (e.g., 1.5 for glass)

    Material()
        : base_color(0.8f, 0.8f, 0.8f)
        , roughness(0.5f)
        , metallic(0.0f)
        , transmission(0.0f)
        , emission(0.0f, 0.0f, 0.0f)
        , ior(1.5f) {}

    Material(const glm::vec3& color, float rough = 0.5f, float metal = 0.0f, 
             const glm::vec3& emis = glm::vec3(0.0f, 0.0f, 0.0f), 
             float trans = 0.0f, float index_of_refraction = 1.5f)
        : base_color(color)
        , roughness(rough)
        , metallic(metal)
        , transmission(trans)
        , emission(emis)
        , ior(index_of_refraction) {}
};

