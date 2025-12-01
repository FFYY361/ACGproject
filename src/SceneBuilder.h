#pragma once
#include "Scene.h"
#include "Entity.h"
#include "Material.h"
#include <memory>

// SceneBuilder - Factory class for creating different pre-defined scenes
class SceneBuilder {
public:
    // Build a Cornell Box scene (classic ray tracing test scene)
    static void BuildCornellBox(Scene* scene);
    
    // Build a simple test scene with a few primitives
    static void BuildTestScene(Scene* scene);
    
    // Build the default demo scene
    static void BuildDefaultScene(Scene* scene);
    
    // Build a scene showcasing different materials
    static void BuildMaterialShowcase(Scene* scene);
    
    // Build an empty scene (just a ground plane)
    static void BuildEmptyScene(Scene* scene);
    
    // ===== Add your custom scenes below =====
    
    // Example: Build your custom scene
    static void BuildMyCustomScene(Scene* scene);
    
private:
    // Helper functions for common scene elements
    static std::shared_ptr<Entity> CreateGroundPlane(
        const glm::vec3& position = glm::vec3(0.0f, -1.0f, 0.0f),
        const glm::vec3& scale = glm::vec3(10.0f, 0.1f, 10.0f),
        const Material& material = Material(glm::vec3(0.8f, 0.8f, 0.8f), 0.8f, 0.0f)
    );
    
    static std::shared_ptr<Entity> CreateSphere(
        const glm::vec3& position,
        float radius = 1.0f,
        const Material& material = Material()
    );
    
    static std::shared_ptr<Entity> CreateCube(
        const glm::vec3& position,
        const glm::vec3& scale = glm::vec3(1.0f),
        const Material& material = Material()
    );
    
    // Light creation helpers (returns Light struct, not Entity)
    static std::shared_ptr<Light> CreatePointLight(
        const glm::vec3& position,
        const glm::vec3& color = glm::vec3(10.0f)
    );
    
    static std::shared_ptr<Light> CreateAreaLight(
        const glm::vec3& position,
        const glm::vec3& color,
        const glm::vec3& u,
        const glm::vec3& v
    );
};
