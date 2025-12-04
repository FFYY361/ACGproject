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

    // Light source - a small emissive cube above the scene
    auto light = std::make_shared<Light>(
        0,
        glm::vec3(0.0f, 2.0f, 1.0f),
        glm::vec3(15.0f, 15.0f, 15.0f)
    );
    scene->AddLight(light);

    /*auto light = std::make_shared<Light>(
        1,
        glm::vec3(0.0f, 2.0f, 0.0f),
        glm::vec3(15.0f, 15.0f, 15.0f),
        glm::vec3(0.0f, 0.0f, -0.1f),
        glm::vec3(0.1f, 0.0f, 0.0f),
        0.1f
    );
    scene->AddLight(light);*/


    // Ground plane
    scene->AddEntity(CreateGroundPlane());
    
    // Yellow sphere
    scene->AddEntity(CreateSphere(
        glm::vec3(-2.0f, 0.5f, 0.0f),
        0.5f,
        Material(glm::vec3(1.0f, 1.0f, 0.0f), 1.0f, 0.0f)
    ));
    
    // Metallic white sphere
    scene->AddEntity(CreateSphere(
        glm::vec3(0.0f, 0.5f, 0.0f),
        0.5f,
        Material(glm::vec3(1.0f, 1.0f, 1.0f), 0.1f, 1.0f)
    ));
    
    // Blue cube
    scene->AddEntity(CreateCube(
        glm::vec3(2.0f, 0.5f, 0.0f),
        glm::vec3(1.0f, 1.0f, 1.0f),
        Material(glm::vec3(0.0f, 0.0f, 1.0f), 1.0f, 0.0f)
    ));

    
    scene->BuildAccelerationStructures();
}

void SceneBuilder::BuildCornellBox(Scene* scene) {
    scene->Clear();
    
    float box_size = 2.0f;
    float wall_thickness = 0.1f;

    auto light = std::make_shared<Light>(
        1,
        glm::vec3(0.0f, box_size - 0.15f, 0.0f),
        glm::vec3(15.0f, 15.0f, 15.0f),
        glm::vec3(0.0f, 0.0f, -1.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        1.0f
    );
    scene->AddLight(light);

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

    
    scene->BuildAccelerationStructures();
}

void SceneBuilder::BuildCornellBox2(Scene* scene) {
    scene->Clear();
    
    float box_size = 2.0f;
    float wall_thickness = 0.1f;

    // Circular disk area light on ceiling (using small square approximation)
    float disk_radius = 0.8f;  // 增大光源尺寸
    scene->AddLight(CreateAreaLight(
        glm::vec3(0.0f, box_size - 0.15f, 0.0f),
        glm::vec3(15.0f, 15.0f, 15.0f),
        glm::vec3(disk_radius, 0.0f, 0.0f),  // u vector (x direction)
        glm::vec3(0.0f, 0.0f, disk_radius)   // v vector (z direction)
    ));

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
    
    // Metallic sphere (left side)
    scene->AddEntity(CreateSphere(
        glm::vec3(-0.6f, -1.3f, -0.3f),
        0.7f,
        Material(glm::vec3(0.95f, 0.95f, 0.95f), 0.05f, 1.0f, glm::vec3(0.0f), 0.0f, 1.5f)  // High metallic, low roughness
    ));
    
    // Transparent/Glass sphere (right side) - with transmission
    scene->AddEntity(CreateSphere(
        glm::vec3(0.7f, -1.3f, 0.4f),
        0.7f,
        Material(glm::vec3(1.0f, 1.0f, 1.0f), 0.0f, 0.0f, glm::vec3(0.0f), 1.0f, 1.5f)  // Clear glass with no absorption
    ));
    
    scene->BuildAccelerationStructures();
}

void SceneBuilder::BuildCornellBoxMesh(Scene* scene) {
    scene->Clear();
    
    float box_size = 2.0f;
    float wall_thickness = 0.1f;

    // 大圆盘面光源在天花板上
    float disk_radius = 0.6f;
    scene->AddLight(CreateAreaLight(
        glm::vec3(0.0f, box_size - 0.15f, 0.0f),
        glm::vec3(12.0f, 12.0f, 12.0f),  // 白色强光
        glm::vec3(disk_radius, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, disk_radius)
    ));

    // Floor (white)
    scene->AddEntity(CreateGroundPlane(
        glm::vec3(0.0f, -box_size, 0.0f),
        glm::vec3(box_size, wall_thickness, box_size),
        Material(glm::vec3(0.75f, 0.75f, 0.75f), 0.9f, 0.0f)
    ));
    
    // Ceiling (white)
    scene->AddEntity(CreateCube(
        glm::vec3(0.0f, box_size, 0.0f),
        glm::vec3(box_size, wall_thickness, box_size),
        Material(glm::vec3(0.75f, 0.75f, 0.75f), 0.9f, 0.0f)
    ));
    
    // Back wall (white)
    scene->AddEntity(CreateCube(
        glm::vec3(0.0f, 0.0f, -box_size),
        glm::vec3(box_size, box_size, wall_thickness),
        Material(glm::vec3(0.75f, 0.75f, 0.75f), 0.9f, 0.0f)
    ));
    
    // Left wall (red)
    scene->AddEntity(CreateCube(
        glm::vec3(-box_size, 0.0f, 0.0f),
        glm::vec3(wall_thickness, box_size, box_size),
        Material(glm::vec3(0.75f, 0.25f, 0.25f), 0.9f, 0.0f)
    ));
    
    // Right wall (blue)
    scene->AddEntity(CreateCube(
        glm::vec3(box_size, 0.0f, 0.0f),
        glm::vec3(wall_thickness, box_size, box_size),
        Material(glm::vec3(0.25f, 0.25f, 0.75f), 0.9f, 0.0f)
    ));
    
    // 金属立方体 (使用cube.obj)
    scene->AddEntity(CreateCube(
        glm::vec3(-0.6f, -1.2f, -0.3f),
        glm::vec3(0.6f, 0.8f, 0.6f),
        Material(glm::vec3(0.9f, 0.9f, 0.9f), 0.1f, 1.0f)  // 光滑金属
    ));
    
    // 玻璃球 (透明)
    scene->AddEntity(CreateSphere(
        glm::vec3(0.7f, -1.3f, 0.4f),
        0.7f,
        Material(glm::vec3(1.0f, 1.0f, 1.0f), 0.0f, 0.0f, glm::vec3(0.0f), 1.0f, 1.5f)  // 完全透明的玻璃
    ));
    
    scene->BuildAccelerationStructures();
}

void SceneBuilder::BuildGlassTestScene(Scene* scene) {
    scene->Clear();
    
    // Ground plane
    scene->AddEntity(CreateGroundPlane(
        glm::vec3(0.0f, -1.0f, 0.0f),
        glm::vec3(10.0f, 0.1f, 10.0f),
        Material(glm::vec3(0.8f, 0.8f, 0.8f), 0.8f, 0.0f)
    ));
    
    // Back wall
    scene->AddEntity(CreateCube(
        glm::vec3(0.0f, 1.0f, -3.0f),
        glm::vec3(10.0f, 5.0f, 0.1f),
        Material(glm::vec3(0.9f, 0.9f, 0.9f), 0.9f, 0.0f)
    ));
    
    // Glass sphere (clear glass, IOR 1.5)
    scene->AddEntity(CreateSphere(
        glm::vec3(-1.5f, 0.5f, 0.0f),
        0.5f,
        Material(glm::vec3(0.99f, 0.99f, 0.99f), 0.0f, 0.0f, glm::vec3(0.0f), 1.0f, 1.5f)
    ));
    
    // Colored glass sphere (green tint)
    scene->AddEntity(CreateSphere(
        glm::vec3(0.0f, 0.5f, 0.0f),
        0.5f,
        Material(glm::vec3(0.8f, 0.95f, 0.8f), 0.0f, 0.0f, glm::vec3(0.0f), 1.0f, 1.5f)
    ));
    
    // Diamond (higher IOR 2.4)
    scene->AddEntity(CreateSphere(
        glm::vec3(1.5f, 0.5f, 0.0f),
        0.5f,
        Material(glm::vec3(0.99f, 0.99f, 0.99f), 0.0f, 0.0f, glm::vec3(0.0f), 1.0f, 2.4f)
    ));
    
    // Metallic sphere for comparison
    scene->AddEntity(CreateSphere(
        glm::vec3(-1.5f, 0.5f, 1.5f),
        0.5f,
        Material(glm::vec3(1.0f, 0.85f, 0.57f), 0.05f, 1.0f)  // Gold
    ));
    
    // Area light above
    scene->AddLight(CreateAreaLight(
        glm::vec3(0.0f, 3.0f, 0.0f),
        glm::vec3(20.0f, 20.0f, 20.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f)
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
    scene->AddLight(CreatePointLight(
        glm::vec3(0.0f, 4.0f, 0.0f),
        glm::vec3(20.0f, 20.0f, 20.0f)
    ));
    
    scene->BuildAccelerationStructures();
}

