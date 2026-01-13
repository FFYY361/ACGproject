#pragma once
#include "long_march.h"

// Simple material structure for ray tracing
struct Material {
    glm::vec3 base_color;
    glm::vec3 emission;
    glm::vec3 alpha;
    float roughness;
    float metallic;
	float specular;    // Specular intensity
    float transmission;  // 0 = opaque, 1 = fully transparent
    float ior;           // Index of refraction (e.g., 1.5 for glass)
	float subsurface;   // Subsurface scattering factor
	float clearcoat;    // Clearcoat layer intensity
	float sheen;       // Sheen intensity
    int texture_id;      // -1 = no texture, >= 0 = texture index
    int normal_id;
    int use_vertex_color;  // 0 = use base_color/texture, 1 = use vertex colors

    Material()
        : base_color(0.8f, 0.8f, 0.8f)
        , roughness(0.5f)
        , metallic(0.0f)
        , emission(0.0f, 0.0f, 0.0f)
        , transmission(0.0f)
        , ior(1.5f)
        , texture_id(-1)
        , normal_id(-1)
        , use_vertex_color(0)
        , alpha(0.0f, 0.0f, 0.0f) {}

    Material(const glm::vec3& color, float rough = 0.5f, float metal = 0.0f, 
             const glm::vec3& emis = glm::vec3(0.0f, 0.0f, 0.0f), 
             float trans = 0.0f, float index_of_refraction = 1.5f, int tex_id = -1, int normal_id = -1,
             const glm::vec3& alpha = glm::vec3(1.0f, 1.0f, 1.0f))
        : base_color(color)
        , roughness(rough)
        , metallic(metal)
        , emission(emis)
        , transmission(trans)
        , ior(index_of_refraction)
        , texture_id(tex_id) 
        , normal_id(normal_id)
        , use_vertex_color(0)
        , alpha(alpha)
		, subsurface(0.0f)
		, clearcoat(0.0f)
		, sheen(0.0f)
		, specular(0.0f) {}

    Material(
        const glm::vec3& color, const glm::vec3& emission, const int texture_id, const int normal_id,
		float roughness = 0.5f, float metallic = 0.0f, float specular = 0.0f, float transmission = 0.0f,
		float ior = 1.5f, float subsurface = 0.0f, float clearcoat = 0.0f, float sheen = 0.0f,
		const glm::vec3& alpha = glm::vec3(1.0f, 1.0f, 1.0f), int use_vertex_color = 0
    )
        : base_color(color)
        , emission(emission)
        , roughness(roughness)
		, metallic(metallic)
        , specular(specular)
        , transmission(transmission)
		, ior(ior)
		, subsurface(subsurface)
		, clearcoat(clearcoat)
        , sheen(sheen)
        , texture_id(texture_id)
        , normal_id(normal_id)
        , use_vertex_color(use_vertex_color)
		, alpha(alpha) {}
        
};

