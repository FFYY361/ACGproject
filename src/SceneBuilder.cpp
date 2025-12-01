#include "SceneBuilder.h"
#include "glm/gtc/matrix_transform.hpp"

// ===== Helper Functions =====

std::shared_ptr<Entity> SceneBuilder::CreateGroundPlane(
    const glm::vec3& position,
    const glm::vec3& scale,
    const Material& material
) {
    return std::make_shared<Entity>(
        PROJECT_DIR "/meshes/cube.obj",
        material,
        glm::scale(glm::translate(glm::mat4(1.0f), position), scale)
    );
}

std::shared_ptr<Entity> SceneBuilder::CreateSphere(
    const glm::vec3& position,
    float radius,
    const Material& material
) {
    return std::make_shared<Entity>(
        PROJECT_DIR "/meshes/simple_sphere.obj",
        material,
        glm::scale(glm::translate(glm::mat4(1.0f), position), glm::vec3(radius))
    );
}

std::shared_ptr<Entity> SceneBuilder::CreateCube(
    const glm::vec3& position,
    const glm::vec3& scale,
    const Material& material
) {
    return std::make_shared<Entity>(
        PROJECT_DIR "/meshes/cube.obj",
        material,
        glm::scale(glm::translate(glm::mat4(1.0f), position), scale)
    );
}

std::shared_ptr<Light> SceneBuilder::CreatePointLight(
    const glm::vec3& position,
    const glm::vec3& color
) {
    return std::make_shared<Light>(
        0,  // type: 0 = Point light
        position,
        color
    );
}

std::shared_ptr<Light> SceneBuilder::CreateAreaLight(
    const glm::vec3& position,
    const glm::vec3& color,
    const glm::vec3& u,
    const glm::vec3& v
) {
    float area = glm::length(glm::cross(u, v));
    return std::make_shared<Light>(
        1,  // type: 1 = Area light
        position,
        color,
        u,
        v,
        area
    );
}

// ===== Pre-defined Scenes =====

void SceneBuilder::BuildDefaultScene(Scene* scene) {
    scene->Clear();
    
    // Ground plane
    scene->AddEntity(CreateGroundPlane());
    
    // Yellow sphere
    scene->AddEntity(CreateSphere(
        glm::vec3(-2.0f, 0.5f, 0.0f),
        0.5f,
        Material(glm::vec3(1.0f, 1.0f, 0.0f), 0.3f, 0.0f)
    ));
    
    // Metallic white sphere
    scene->AddEntity(CreateSphere(
        glm::vec3(0.0f, 0.5f, 0.0f),
        0.5f,
        Material(glm::vec3(1.0f, 1.0f, 1.0f), 0.0f, 0.9f)
    ));
    
    // Blue cube
    scene->AddEntity(CreateCube(
        glm::vec3(2.0f, 0.5f, 0.0f),
        glm::vec3(1.0f, 1.0f, 1.0f),
        Material(glm::vec3(0.2f, 0.2f, 1.0f), 0.5f, 0.0f)
    ));

    {
        // Light source - a small emissive cube above the scene
        auto light = std::make_shared<Light>(
            0,
            glm::vec3(0.0f, 2.0f, 1.0f),
            glm::vec3(8.0f, 8.0f, 8.0f)
        );
         scene->AddLight(light);
    }
    
    scene->BuildAccelerationStructures();
}

void SceneBuilder::BuildCornellBox(Scene* scene) {
    scene->Clear();
    
    float box_size = 2.0f;
    float wall_thickness = 0.1f;
    
    // Floor (white)
    scene->AddEntity(CreateGroundPlane(
        glm::vec3(0.0f, -box_size, 0.0f),
        glm::vec3(box_size, wall_thickness, box_size),
        Material(glm::vec3(0.9f, 0.9f, 0.9f), 0.9f, 0.0f)
    ));
    
    // Ceiling (white)
    scene->AddEntity(CreateCube(
        glm::vec3(0.0f, box_size, 0.0f),
        glm::vec3(box_size, wall_thickness, box_size),
        Material(glm::vec3(0.9f, 0.9f, 0.9f), 0.9f, 0.0f)
    ));
    
    // Back wall (white)
    scene->AddEntity(CreateCube(
        glm::vec3(0.0f, 0.0f, -box_size),
        glm::vec3(box_size, box_size, wall_thickness),
        Material(glm::vec3(0.9f, 0.9f, 0.9f), 0.9f, 0.0f)
    ));
    
    // Left wall (red)
    scene->AddEntity(CreateCube(
        glm::vec3(-box_size, 0.0f, 0.0f),
        glm::vec3(wall_thickness, box_size, box_size),
        Material(glm::vec3(0.9f, 0.1f, 0.1f), 0.9f, 0.0f)
    ));
    
    // Right wall (green)
    scene->AddEntity(CreateCube(
        glm::vec3(box_size, 0.0f, 0.0f),
        glm::vec3(wall_thickness, box_size, box_size),
        Material(glm::vec3(0.1f, 0.9f, 0.1f), 0.9f, 0.0f)
    ));
    
    // Tall box (white)
    scene->AddEntity(CreateCube(
        glm::vec3(-0.5f, -1.0f, -0.5f),
        glm::vec3(0.6f, 1.0f, 0.6f),
        Material(glm::vec3(0.9f, 0.9f, 0.9f), 0.8f, 0.0f)
    ));
    
    // Short box (white)
    scene->AddEntity(CreateCube(
        glm::vec3(0.6f, -1.5f, 0.3f),
        glm::vec3(0.6f, 0.5f, 0.6f),
        Material(glm::vec3(0.9f, 0.9f, 0.9f), 0.8f, 0.0f)
    ));
    
    // Ceiling light
    /*scene->AddEntity(CreateLight(
        glm::vec3(0.0f, box_size - 0.15f, 0.0f),
        glm::vec3(15.0f, 15.0f, 15.0f),
        glm::vec3(0.5f, 0.05f, 0.5f)
    ));*/

    auto light = std::make_shared<Light>(
        1,
        glm::vec3(0.0f, box_size - 0.15f, 0.0f),
        glm::vec3(15.0f, 15.0f, 15.0f),
        glm::vec3(0.0f, 0.0f, -1.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        1.0f
    );
    scene->AddLight(light);
    
    scene->BuildAccelerationStructures();
}

void SceneBuilder::BuildTestScene(Scene* scene) {
    scene->Clear();
    
    // Ground
    scene->AddEntity(CreateGroundPlane());
    
    // Row of spheres with varying properties
    for (int i = 0; i < 5; i++) {
        float x = -4.0f + i * 2.0f;
        float roughness = i / 4.0f;
        
        scene->AddEntity(CreateSphere(
            glm::vec3(x, 0.5f, 0.0f),
            0.5f,
            Material(glm::vec3(1.0f, 0.8f, 0.2f), roughness, 0.0f)
        ));
    }
    
    // Emissive sphere as light source
    scene->AddEntity(CreateSphere(
        glm::vec3(0.0f, 3.0f, 0.0f),
        0.5f,
        Material(glm::vec3(1.0f, 1.0f, 1.0f), 0.0f, 0.0f, glm::vec3(10.0f))
    ));
    
    scene->BuildAccelerationStructures();
}

void SceneBuilder::BuildMaterialShowcase(Scene* scene) {
    scene->Clear();
    
    // Ground
    scene->AddEntity(CreateGroundPlane());
    
    // Back row: Roughness test (0.0 to 1.0)
    for (int i = 0; i < 5; i++) {
        float x = -4.0f + i * 2.0f;
        float roughness = i / 4.0f;
        
        scene->AddEntity(CreateSphere(
            glm::vec3(x, 0.5f, -2.0f),
            0.5f,
            Material(glm::vec3(1.0f, 1.0f, 1.0f), roughness, 0.0f)
        ));
    }
    
    // Front row: Metallic test (0.0 to 1.0)
    for (int i = 0; i < 5; i++) {
        float x = -4.0f + i * 2.0f;
        float metallic = i / 4.0f;
        
        scene->AddEntity(CreateSphere(
            glm::vec3(x, 0.5f, 2.0f),
            0.5f,
            Material(glm::vec3(1.0f, 0.8f, 0.2f), 0.2f, metallic)
        ));
    }
    
    // Colored spheres in the middle
    scene->AddEntity(CreateSphere(
        glm::vec3(-2.0f, 0.5f, 0.0f),
        0.5f,
        Material(glm::vec3(1.0f, 0.0f, 0.0f), 0.3f, 0.0f)  // Red
    ));
    
    scene->AddEntity(CreateSphere(
        glm::vec3(0.0f, 0.5f, 0.0f),
        0.5f,
        Material(glm::vec3(0.0f, 1.0f, 0.0f), 0.3f, 0.0f)  // Green
    ));
    
    scene->AddEntity(CreateSphere(
        glm::vec3(2.0f, 0.5f, 0.0f),
        0.5f,
        Material(glm::vec3(0.0f, 0.0f, 1.0f), 0.3f, 0.0f)  // Blue
    ));
    
    // Light
    scene->AddEntity(CreateLight(
        glm::vec3(0.0f, 4.0f, 0.0f),
        glm::vec3(20.0f, 20.0f, 20.0f)
    ));
    
    scene->BuildAccelerationStructures();
}

void SceneBuilder::BuildEmptyScene(Scene* scene) {
    scene->Clear();
    
    // Just a ground plane
    scene->AddEntity(CreateGroundPlane());
    
    scene->BuildAccelerationStructures();
}

// ===== Your Custom Scenes =====

void SceneBuilder::BuildMyCustomScene(Scene* scene) {
    scene->Clear();
    
    // TODO: Add your custom scene here
    // Example:
    
    // Ground
    scene->AddEntity(CreateGroundPlane(
        glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec3(20.0f, 0.1f, 20.0f),
        Material(glm::vec3(0.3f, 0.3f, 0.35f), 0.9f, 0.0f)
    ));
    
    // Add your objects here...
    // scene->AddEntity(CreateSphere(...));
    // scene->AddEntity(CreateCube(...));
    // scene->AddEntity(CreateLight(...));
    
    scene->BuildAccelerationStructures();
}
