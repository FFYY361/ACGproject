#pragma once
#include "long_march.h"
#include <glm/glm.hpp>

// Simple material structure for ray tracing
struct Material {
    glm::vec3 base_color;
    glm::vec3 emission;
    glm::vec3 alpha;
    float roughness;
    float metallic;
    float transmission;  // 0 = opaque, 1 = fully transparent
    glm::vec3 ior;       // Index of refraction for R/G/B (色散)
    int texture_id;      // -1 = no texture, >= 0 = texture index
    int normal_id;
    int use_vertex_color;  // 0 = use base_color/texture, 1 = use vertex colors
    int use_toon;          // 0 = PBR shading, 1 = Toon/Cel shading
    
    // Volume properties
    glm::vec3 volume_emission;    // Volumetric emission
    glm::vec3 volume_absorption;  // Absorption coefficient (sigma_a)
    glm::vec3 volume_scattering;  // Scattering coefficient (sigma_s)
    float volume_density;         // Density multiplier
    float volume_anisotropy;      // Phase function anisotropy [-1, 1], 0 = isotropic

    Material()
        : base_color(glm::vec3(0.8f))
        , roughness(0.5f)
        , metallic(0.0f)
        , emission(glm::vec3(0.0f))
        , transmission(0.0f)
        , ior(glm::vec3(1.5f))
        , texture_id(-1)
        , normal_id(-1)
        , use_vertex_color(0)
        , use_toon(0)
        , alpha(glm::vec3(0.0f))
        , volume_emission(glm::vec3(0.0f))
        , volume_absorption(glm::vec3(0.0f))
        , volume_scattering(glm::vec3(0.0f))
        , volume_density(0.0f)
        , volume_anisotropy(0.0f) {}

    // Primary constructor: accepts vec3 IOR for dispersion
    Material(const glm::vec3& color, float rough, float metal, const glm::vec3& emis, float trans, const glm::vec3& index_of_refraction, int tex_id = -1, int normal_id = -1, const glm::vec3& alpha_in = glm::vec3(1.0f))
        : base_color(color)
        , roughness(rough)
        , metallic(metal)
        , emission(emis)
        , transmission(trans)
        , ior(index_of_refraction)
        , texture_id(tex_id)
        , normal_id(normal_id)
        , use_vertex_color(0)
        , use_toon(0)
        , alpha(alpha_in)
        , volume_emission(glm::vec3(0.0f))
        , volume_absorption(glm::vec3(0.0f))
        , volume_scattering(glm::vec3(0.0f))
        , volume_density(0.0f)
        , volume_anisotropy(0.0f) {}

    // Convenience constructor: vec3 IOR without explicit emission/alpha
    Material(const glm::vec3& color, float rough, float metal, float trans, const glm::vec3& index_of_refraction, int tex_id = -1, int normal_id = -1)
        : Material(color, rough, metal, glm::vec3(0.0f), trans, index_of_refraction, tex_id, normal_id, glm::vec3(1.0f)) {}

    // Backward-compatible overloads that accept a single float IOR (replicated across RGB)
    inline Material(const glm::vec3& color, float rough, float metal, const glm::vec3& emis, float trans, float index_of_refraction, int tex_id = -1, int normal_id = -1, const glm::vec3& alpha_in = glm::vec3(1.0f))
        : Material(color, rough, metal, emis, trans, glm::vec3(index_of_refraction), tex_id, normal_id, alpha_in) {}

    inline Material(const glm::vec3& color, float rough, float metal, float trans, float index_of_refraction, int tex_id = -1, int normal_id = -1)
        : Material(color, rough, metal, glm::vec3(0.0f), trans, glm::vec3(index_of_refraction), tex_id, normal_id, glm::vec3(1.0f)) {}

    // Convenience: set only emission (others default)
    inline Material(const glm::vec3& color, float rough, float metal, const glm::vec3& emis)
        : Material(color, rough, metal, emis, 0.0f, glm::vec3(1.5f), -1, -1, glm::vec3(1.0f)) {}

    inline Material(const glm::vec3& color, float rough, float metal)
        : Material(color, rough, metal, glm::vec3(0.0f), 0.0f, glm::vec3(1.5f), -1, -1, glm::vec3(1.0f)) {}
};