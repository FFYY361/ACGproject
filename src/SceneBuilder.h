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
    
    // Build a second Cornell Box with circular disk light, metal and glass spheres
    static void BuildCornellBox2(Scene* scene);
    
    // Build a glass material test scene
    static void BuildGlassTestScene(Scene* scene);
    
    // Build Cornell Box with mesh model and glass sphere
    static void BuildCornellBoxMesh(Scene* scene);
    
    // Build Cornell Box 3 with flat walls, prism and cube
    static void BuildCornellBox3(Scene* scene);
    
    // Build Cornell Box 4 with mirror sphere and transparent cube
    static void BuildCornellBox4(Scene* scene);
    
    // Build a procedural scene (no textures needed)
    static void BuildProceduralScene(Scene* scene);
    
    // Build bedroom scene using bedroom mesh
    static void BuildBedroomScene(Scene* scene);
    
    // Build the default demo scene
    static void BuildDefaultScene(Scene* scene);
    
    // Build a scene showcasing different materials
    static void BuildMaterialShowcase(Scene* scene);
    
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
