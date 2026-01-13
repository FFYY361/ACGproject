#ifndef MATERIAL_H
#define MATERIAL_H

struct Material {
    glm::vec3 base_color = glm::vec3(0.8f);
    glm::vec3 emission = glm::vec3(0.0f);
    glm::vec3 alpha = glm::vec3(1.0f);

    float roughness = 0.5f;
    float metallic = 0.0f;
    float specular = 0.0f;
    float transmission = 0.0f;

    glm::vec3 ior = glm::vec3(1.5f);

    float subsurface = 0.0f;
    float clearcoat = 0.0f;
    float sheen = 0.0f;

    int texture_id = -1;
    int normal_id = -1;
    int use_vertex_color = 0;
    int use_toon = 0;

    // Volume
    glm::vec3 volume_emission = glm::vec3(0.0f);
    glm::vec3 volume_absorption = glm::vec3(0.0f);
    glm::vec3 volume_scattering = glm::vec3(0.0f);
    float volume_density = 0.0f;
    float volume_anisotropy = 0.0f;

    // =========================
    // Master Constructor
    // =========================
    Material(
        const glm::vec3& color,
        const glm::vec3& emission_,
        const glm::vec3& alpha_,
        float roughness_,
        float metallic_,
        float specular_,
        float transmission_,
        const glm::vec3& ior_,
        float subsurface_,
        float clearcoat_,
        float sheen_,
        int texture_id_,
        int normal_id_,
        int use_vertex_color_,
        int use_toon_
    )
        : base_color(color),
        emission(emission_),
        alpha(alpha_),
        roughness(roughness_),
        metallic(metallic_),
        specular(specular_),
        transmission(transmission_),
        ior(ior_),
        subsurface(subsurface_),
        clearcoat(clearcoat_),
        sheen(sheen_),
        texture_id(texture_id_),
        normal_id(normal_id_),
        use_vertex_color(use_vertex_color_),
        use_toon(use_toon_)
    {
    }

    // =========================
    // Default
    // =========================
    Material()
        : Material(glm::vec3(0.8f), glm::vec3(0.0f), glm::vec3(1.0f),
            0.5f, 0.0f, 0.0f, 0.0f,
            glm::vec3(1.5f),
            0.0f, 0.0f, 0.0f,
            -1, -1, 0, 0)
    {
    }

    // =========================
    // Your original interfaces
    // =========================

    // vec3 IOR
    Material(const glm::vec3& color, float rough, float metal,
        const glm::vec3& emis, float trans,
        const glm::vec3& ior_vec,
        int tex = -1, int norm = -1,
        const glm::vec3& alpha_in = glm::vec3(1.0f))
        : Material(color, emis, alpha_in,
            rough, metal, 0.0f, trans,
            ior_vec,
            0.0f, 0.0f, 0.0f,
            tex, norm, 0, 0)
    {
    }

    // float IOR compatibility
    Material(const glm::vec3& color, float rough, float metal,
        const glm::vec3& emis, float trans,
        float ior_scalar,
        int tex = -1, int norm = -1,
        const glm::vec3& alpha_in = glm::vec3(1.0f))
        : Material(color, emis, alpha_in,
            rough, metal, 0.0f, trans,
            glm::vec3(ior_scalar),
            0.0f, 0.0f, 0.0f,
            tex, norm, 0, 0)
    {
    }

    // No emission shortcut
    Material(const glm::vec3& color, float rough, float metal)
        : Material(color, glm::vec3(0.0f), glm::vec3(1.0f),
            rough, metal, 0.0f, 0.0f,
            glm::vec3(1.5f),
            0.0f, 0.0f, 0.0f,
            -1, -1, 0, 0)
    {
    }

    // Your "texture workflow" constructor
    Material(const glm::vec3& color,
        const glm::vec3& emission_,
        int texture_id_, int normal_id_,
        float roughness_ = 0.5f,
        float metallic_ = 0.0f,
        float specular_ = 0.0f,
        float transmission_ = 0.0f,
        float ior_scalar = 1.5f,
        float subsurface_ = 0.0f,
        float clearcoat_ = 0.0f,
        float sheen_ = 0.0f,
        const glm::vec3& alpha_ = glm::vec3(1.0f),
        int use_vertex_color_ = 0)
        : Material(color, emission_, alpha_,
            roughness_, metallic_, specular_, transmission_,
            glm::vec3(ior_scalar),
            subsurface_, clearcoat_, sheen_,
            texture_id_, normal_id_,
            use_vertex_color_, 0)
    {
    }
};

#endif // MATERIAL_H