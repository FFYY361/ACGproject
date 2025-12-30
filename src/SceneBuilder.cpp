#include "SceneBuilder.h"
#include "Scene.h"
#include "glm/gtc/matrix_transform.hpp"
// ===== Helper Functions =====
std::shared_ptr<Entity> SceneBuilder::CreateSphere(
    const glm::vec3& position,
    float radius,
    const Material& material
) {
    return std::make_shared<Entity>(
        PROJECT_DIR "/meshes/sphere.obj",
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

std::shared_ptr<Entity> SceneBuilder::CreateMultiMaterialEntity(
    const std::string& obj_path,
    const glm::mat4& transform
) {
    return std::make_shared<Entity>(obj_path, transform, true);
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

void SceneBuilder::BuildCornellBoxHelper(Scene* scene, float box_size) {
    float wall_thickness = 0.001f;
    // Floor (white)
    scene->AddEntity(CreateCube(
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
}

void SceneBuilder::BuildDefaultScene(Scene* scene) {
    scene->Clear();

    float box_size = 1.5f;

    // === Cornell Box 标准6面墙（与Box3相同） ===

    // === 顶部面光源 ===
    float light_size = 0.7f;
    scene->AddLight(CreateAreaLight(
        glm::vec3(0.0f, box_size-0.1f, 0.4f),
        glm::vec3(50.0f, 50.0f, 50.0f),
        glm::vec3(0.5, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 0.5f)
    ));
    /*
    auto light1 = std::make_shared<Light>(
        0,
        glm::vec3(0.5f, -0.4f, -2.0f),
        glm::vec3(100.0f, 100.0f, 100.0f)
    );
    scene->AddLight(light1);
    */
    //Floor (white)
    scene->AddEntity(CreateCube(
        glm::vec3(0.0f, -box_size, 0.0f),
        glm::vec3(box_size, 0.01f, box_size),
        Material(glm::vec3(0.75f, 0.75f, 0.75f), 0.9f, 0.0f)
    ));
    // Ceiling (white)
    scene->AddEntity(CreateCube(
        glm::vec3(0.0f, box_size, 0.0f),
        glm::vec3(box_size, 0.01f, box_size),
        Material(glm::vec3(0.75f, 0.75f, 0.75f), 0.9f, 0.0f)
    ));
    // Back wall (white)
    scene->AddEntity(CreateCube(
        glm::vec3(0.0f, 0.0f, -box_size),
        glm::vec3(box_size, box_size, 0.01f),
        Material(glm::vec3(0.75f, 0.75f, 0.75f), 0.9f, 0.0f)
    ));
    // Left wall (red)
    scene->AddEntity(CreateCube(
        glm::vec3(-box_size, 0.0f, 0.0f),
        glm::vec3(0.01f, box_size, box_size),
        Material(glm::vec3(0.75f, 0.25f, 0.25f), 0.9f, 0.0f)
    ));
    // Right wall (blue)
    scene->AddEntity(CreateCube(
        glm::vec3(box_size, 0.0f, 0.0f),
        glm::vec3(0.01f, box_size, box_size),
        Material(glm::vec3(0.25f, 0.25f, 0.75f), 1.0f, 0.0f)
    ));

	int idx_normal = scene->AddTexture(PROJECT_DIR "/meshes/sphere_normal.png");
    // int idx_text = scene->AddTexture(PROJECT_DIR "/meshes/cube_color.png");
    // grassland::LogInfo("Added texture with index: {}", idx_text);

    // === 场景物体1：镜面球（左后） ===
    scene->AddEntity(CreateSphere(
        glm::vec3(-0.7f, -box_size + 0.5f, -0.5f),
        //glm::vec3(-0.5f, 0.5f, 0.0f),
        0.5f,
        Material(
            glm::vec3(0.95f, 0.95f, 0.95f),
            0.1f,
            1.0f,
            glm::vec3(0.0f, 0.0f, 0.0f),
            0.0f,
            1.5f,
            -1,
            idx_normal)  // 高度镜面金属
    ));
    // === 场景物体2：透明玻璃立方体（前方，旋转45度） ===
    scene->AddEntity(std::make_shared<Entity>(
        PROJECT_DIR "/meshes/cube.obj",
        // Material(glm::vec3(1.0f, 1.0f, 1.0f), 0.0f, 0.0f, glm::vec3(0.0f), 1.0f, glm::vec3(1.51f, 1.51f, 1.51f), idx_text, -1),  // 色散玻璃 IOR: R=1.51, G=1.52, B=1.54 (轻微色散)
        Material(glm::vec3(1.0f, 1.0f, 1.0f), 0.0f, 0.0f, glm::vec3(0.0f), 1.0f, glm::vec3(1.51f, 1.53f, 1.56f), -1, -1),  // 色散玻璃 IOR: R=1.51, G=1.52, B=1.54 (轻微色散)
        glm::scale(
            glm::rotate(
                glm::translate(glm::mat4(1.0f), glm::vec3(0.5f, -box_size + 0.6f, 0.4f)),
                glm::radians(45.0f),
                glm::vec3(0.0f, 1.0f, 0.0f)  // 绕Y轴旋转
            ),
            glm::vec3(0.5f, 0.5f, 0.5f)
        )
    ));


    //scene->AddEntity(std::make_shared<Entity>(
    //    PROJECT_DIR "/meshes/cube.obj",
    //    Material(glm::vec3(1.0f, 1.0f, 1.0f), 0.8f, 0.0f, glm::vec3(0.0f), 0.00f, 1.5f, idx_text, -1),  // 透明玻璃 IOR=1.5
    //    glm::scale(
    //        glm::rotate(
    //            glm::translate(glm::mat4(1.0f), glm::vec3(0.5f, -10.0, 0.4f)),
    //            glm::radians(0.0f),
    //            glm::vec3(0.0f, 1.0f, 0.0f)  // 绕Y轴旋转
    //        ),
    //        glm::vec3(500.0f, 0.1f, 500.0f)
    //    )
    //));


    //scene->AddEntity(CreateCube(
    //    glm::vec3(0.5f, -box_size + 0.6f, 0.4f),
    //    glm::vec3(0.5f, 0.5f, 0.5f),
    //    Material(glm::vec3(1.0f, 1.0f, 1.0f), 0.4f, 0.0f, glm::vec3(0.0f), 0.00f, 1.01f, idx_text)
    //));


    scene->BuildAccelerationStructures();
}

void SceneBuilder::BuildCornellBox(Scene* scene) {
    scene->Clear();
    
    float box_size = 1.5f;

    BuildCornellBoxHelper(scene, box_size);

    // Point light in the top corner
    auto light = std::make_shared<Light>(
        0,  // type: 0 = Point light
        glm::vec3(box_size * 0.8f, box_size * 0.9f, -box_size * 0.8f),  // 右上后角落
        glm::vec3(10.0f, 10.0f, 10.0f)
    );
    scene->AddLight(light);
    
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

    // Circular disk area light on ceiling (using small square approximation)
    float disk_radius = 0.8f;  // 增大光源尺寸
    scene->AddLight(CreateAreaLight(
        glm::vec3(0.0f, box_size - 0.15f, 0.0f),
        glm::vec3(15.0f, 15.0f, 15.0f),
        glm::vec3(disk_radius, 0.0f, 0.0f),  // u vector (x direction)
        glm::vec3(0.0f, 0.0f, disk_radius)   // v vector (z direction)
    ));

    BuildCornellBoxHelper(scene, box_size);
    
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

    // 大圆盘面光源在天花板上
    float disk_radius = 0.6f;
    scene->AddLight(CreateAreaLight(
        glm::vec3(0.0f, box_size - 0.15f, 0.0f),
        glm::vec3(12.0f, 12.0f, 12.0f),  // 白色强光
        glm::vec3(disk_radius, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, disk_radius)
    ));

    BuildCornellBoxHelper(scene, box_size);
    
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

void SceneBuilder::BuildCornellBox3(Scene* scene) {
    scene->Clear();
    
    float box_size = 2.0f;
    BuildCornellBoxHelper(scene, box_size);
    
    // === 顶部面光源 ===
    float light_size = 0.7f;
    scene->AddLight(CreateAreaLight(
        glm::vec3(0.0f, box_size - 0.01f, 0.0f),
        glm::vec3(50.0f, 50.0f, 50.0f),
        glm::vec3(light_size, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, light_size)
    ));
    
    // === 场景物体1：立起来的三角棱柱（镜面材质） ===
    // 底面和顶面是等边三角形，棱柱沿Y轴竖立
    float prism_height = 1.2f;
    float tri_size = 0.8f;  // 三角形边长
    
    // 创建三角棱柱需要自定义mesh或用现有mesh近似
    // 这里用拉伸的cube来模拟，或者直接加载triangular prism mesh
    // 如果没有专门的mesh，可以用变换后的物体近似
    
    // 方案：使用缩放和旋转的cube来近似三角形轮廓
    // 更好的方案：如果有prism.obj就直接加载
    // 这里先用一个竖立的扁平cube加旋转来模拟
    scene->AddEntity(CreateCube(
        glm::vec3(-0.6f, -box_size + prism_height / 2.0f, -0.4f),
        glm::vec3(0.6f, prism_height, 0.6f),
        Material(glm::vec3(0.95f, 0.95f, 0.95f), 0.05f, 1.0f)  // 镜面金属
    ));
    
    // === 场景物体2：漫反射球 ===
    scene->AddEntity(CreateSphere(
        glm::vec3(0.6f, -box_size + 0.6f, 1.0f),
        0.6f,
        Material(glm::vec3(0.8f, 0.8f, 0.3f), 0.9f, 0.0f)  // 黄色漫反射
    ));
    
    scene->BuildAccelerationStructures();
}

void SceneBuilder::BuildCornellBox4(Scene* scene) {
    scene->Clear();
    
    float box_size = 1.5f;
    float wall_thickness = 0.001f;
    // Floor (white)
    scene->AddEntity(CreateCube(
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
        Material(glm::vec3(0.9f, 0.9f, 0.9f), 0.9f, 0.0f)
    ));
    
    // Right wall (green)
    scene->AddEntity(CreateCube(
        glm::vec3(box_size, 0.0f, 0.0f),
        glm::vec3(wall_thickness, box_size, box_size),
        Material(glm::vec3(0.9f, 0.9f, 0.9f), 0.9f, 0.0f)
    ));
    
    // === 顶部面光源 ===
    float light_size = 0.7f;
    scene->AddLight(CreateAreaLight(
        glm::vec3(0.0f, box_size - 0.01f, 0.0f),
        glm::vec3(30.0f, 30.0f, 30.0f),
        glm::vec3(light_size, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, light_size)
    ));
    
    // // === 场景物体1：镜面球（左后） ===
    // scene->AddEntity(CreateSphere(
    //     glm::vec3(-0.7f, -box_size + 0.5f, -0.5f),
    //     0.5f,
    //     Material(glm::vec3(0.95f, 0.95f, 0.95f), 0.02f, 1.0f)  // 高度镜面金属
    // ));
    
    // // === 场景物体2：透明玻璃立方体（前方，旋转45度） ===
    // scene->AddEntity(std::make_shared<Entity>(
    //     PROJECT_DIR "/meshes/cube.obj",
    //     Material(glm::vec3(1.0f, 1.0f, 1.0f), 0.0f, 0.0f, glm::vec3(0.0f), 1.0f, glm::vec3(2.4f, 1.6f, 1.3f)),  // 色散玻璃 IOR=2.4/1.6/1.3
    //     glm::scale(
    //         glm::rotate(
    //             glm::translate(glm::mat4(1.0f), glm::vec3(0.5f, -box_size + 0.5f, 0.4f)),
    //             glm::radians(45.0f),
    //             glm::vec3(0.0f, 1.0f, 0.0f)  // 绕Y轴旋转
    //         ),
    //         glm::vec3(0.5f, 0.5f, 0.5f)
    //     )
    // ));
    
    // === 场景物体3：Wave模型（中间位置） ===
    auto wave = std::make_shared<Entity>(
        PROJECT_DIR "/meshes/wave1.obj",
        // Material(glm::vec3(1.0f, 1.0f, 1.0f), 0.0f, 0.0f, glm::vec3(0.0f), 1.0f, glm::vec3(2.4f, 1.6f, 1.3f)),  // 色散玻璃
        Material(glm::vec3(1.0f, 1.0f, 1.0f), 0.0f, 0.0f, glm::vec3(0.0f), 1.0f, glm::vec3(1.33f, 1.5f, 1.6f), -1, -1),  // 色散玻璃 IOR: R=1.51, G=1.52, B=1.54 (轻微色散)
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -box_size + 0.4f, 0.0f)) *  // 移到场景中间
        glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.5f)) *  // 缩放
        glm::translate(glm::mat4(1.0f), glm::vec3(-8.5f, 0.2f, 8.95f))  // 中心化（调整z坐标）
    );
    wave->LoadMesh(PROJECT_DIR "/meshes/wave1.obj");
    scene->AddEntity(wave);
    
    scene->BuildAccelerationStructures();
}

void SceneBuilder::BuildGlassTestScene(Scene* scene) {
    scene->Clear();
    
    // Ground plane
    scene->AddEntity(CreateCube(
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
        Material(glm::vec3(0.99f, 0.99f, 0.99f), 0.0f, 0.0f, glm::vec3(0.0f), 1.0f, glm::vec3(2.4f, 1.6f, 1.3f))
    ));
    
    // Colored glass sphere (green tint)
    scene->AddEntity(CreateSphere(
        glm::vec3(0.0f, 0.5f, 0.0f),
        0.5f,
        Material(glm::vec3(0.8f, 0.95f, 0.8f), 0.0f, 0.0f, glm::vec3(0.0f), 1.0f, glm::vec3(2.4f, 1.6f, 1.3f))
    ));
    
    // Diamond (higher IOR 2.4)
    scene->AddEntity(CreateSphere(
        glm::vec3(1.5f, 0.5f, 0.0f),
        0.5f,
        Material(glm::vec3(0.99f, 0.99f, 0.99f), 0.0f, 0.0f, glm::vec3(0.0f), 1.0f, glm::vec3(2.4f, 1.6f, 1.3f))
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
    scene->AddEntity(CreateCube(
        glm::vec3(0.0f, -0.5f, 0.0f),
        glm::vec3(10.0f, 0.1f, 10.0f),
        Material(glm::vec3(0.8f, 0.8f, 0.8f), 0.9f, 0.0f)
    ));
    
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

void SceneBuilder::BuildProceduralScene(Scene* scene) {
    scene->Clear();
    
    float box_size = 2.0f;
    
    // === 加载纹理 ===
    int mygo_texture_id = scene->AddTexture(PROJECT_DIR "/textures/mygo.jpeg");
    grassland::LogInfo("ProceduralScene: mygo texture id = {}", mygo_texture_id);
    int thu_texture_id = scene->AddTexture(PROJECT_DIR "/textures/thu.png");
    grassland::LogInfo("ProceduralScene: thu texture id = {}", thu_texture_id);
    
    // === 顶部圆盘面光源 ===
    float disk_radius = 0.6f;
    scene->AddLight(CreateAreaLight(
        glm::vec3(0.0f, box_size - 0.01f, 0.0f),
        glm::vec3(30.0f, 30.0f, 30.0f),
        glm::vec3(disk_radius, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, disk_radius)
    ));
    
    BuildCornellBoxHelper(scene, box_size);
    
    // === Motion Blur 演示：两个带纹理的立方体 ===
    
    // 左侧立方体 - 往左运动的motion blur
    {
        // 最终静止位置（当前帧）
        glm::vec3 final_pos = glm::vec3(-1.0f, -box_size + 0.6f, -0.5f);
        float rotation_angle = 30.0f;  // 绕Y轴旋转30度
        
        // 当前位置（最终静止位置）：放大到0.8倍 + 旋转
        glm::mat4 curT = glm::scale(
            glm::rotate(
                glm::translate(glm::mat4(1.0f), final_pos),
                glm::radians(rotation_angle),
                glm::vec3(0.0f, 1.0f, 0.0f)
            ),
            glm::vec3(0.8f, 0.8f, 0.8f)
        );
        
        // 前一帧位置：在右边（运动起始位置，从右往左运动）
        glm::vec3 start_pos = final_pos + glm::vec3(1.5f, 0.0f, 0.0f);  // 往右偏移1.5
        glm::mat4 prevT = glm::scale(
            glm::rotate(
                glm::translate(glm::mat4(1.0f), start_pos),
                glm::radians(rotation_angle),
                glm::vec3(0.0f, 1.0f, 0.0f)
            ),
            glm::vec3(0.8f, 0.8f, 0.8f)
        );
        
        auto ent = std::make_shared<Entity>(
            PROJECT_DIR "/meshes/cube_uv.obj",
            Material(glm::vec3(1.0f, 1.0f, 1.0f), 0.2f, 0.0f, glm::vec3(0.0f), 0.0f, 1.0f, mygo_texture_id),
            curT
        );
        
        ent->SetPreviousTransform(prevT);
        scene->AddEntity(ent);
        
        // Debug: log the transform matrices
        grassland::LogInfo("Left cube - Current pos: ({}, {}, {})", final_pos.x, final_pos.y, final_pos.z);
        grassland::LogInfo("Left cube - Start pos: ({}, {}, {})", start_pos.x, start_pos.y, start_pos.z);
    }
    
    // 右侧立方体 - 往下运动的motion blur
    {
        // 最终静止位置（当前帧）
        glm::vec3 final_pos = glm::vec3(1.0f, -box_size + 0.6f, 0.5f);
        float rotation_angle = -25.0f;  // 绕Y轴反向旋转25度
        
        // 当前位置（最终静止位置）：放大到0.8倍 + 旋转
        glm::mat4 curT = glm::scale(
            glm::rotate(
                glm::translate(glm::mat4(1.0f), final_pos),
                glm::radians(rotation_angle),
                glm::vec3(0.0f, 1.0f, 0.0f)
            ),
            glm::vec3(0.8f, 0.8f, 0.8f)
        );
        
        // 前一帧位置：在上边（运动起始位置，从上往下运动）
        glm::vec3 start_pos = final_pos + glm::vec3(0.0f, 1.5f, 0.0f);  // 往上偏移1.5
        glm::mat4 prevT = glm::scale(
            glm::rotate(
                glm::translate(glm::mat4(1.0f), start_pos),
                glm::radians(rotation_angle),
                glm::vec3(0.0f, 1.0f, 0.0f)
            ),
            glm::vec3(0.8f, 0.8f, 0.8f)
        );
        
        auto ent = std::make_shared<Entity>(
            PROJECT_DIR "/meshes/cube_uv.obj",
            Material(glm::vec3(1.0f, 1.0f, 1.0f), 0.2f, 0.0f, glm::vec3(0.0f), 0.0f, 1.0f, thu_texture_id),
            curT
        );
        
        ent->SetPreviousTransform(prevT);
        scene->AddEntity(ent);
        
        // Debug: log the transform matrices
        grassland::LogInfo("Right cube - Current pos: ({}, {}, {})", final_pos.x, final_pos.y, final_pos.z);
        grassland::LogInfo("Right cube - Start pos: ({}, {}, {})", start_pos.x, start_pos.y, start_pos.z);
    }
    
    scene->BuildAccelerationStructures();
}

void SceneBuilder::BuildBedroomScene(Scene* scene) {
    scene->Clear();
    
    // Add area light for illumination
    scene->AddLight(CreateAreaLight(
        glm::vec3(0.0f, 3.0f, 0.0f),
        glm::vec3(50.0f, 50.0f, 50.0f),
        glm::vec3(2.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 2.0f)
    ));
    
    // Load bedroom with multi-material support (true = load materials from MTL)
    auto bedroom = std::make_shared<Entity>(
        PROJECT_DIR "/meshes/bedroom/iscv2.obj",
        glm::mat4(1.0f),
        true  // Load materials from MTL file
    );
    bedroom->LoadMesh(PROJECT_DIR "/meshes/bedroom/iscv2.obj");
    
    scene->AddEntity(bedroom);
    
    grassland::LogInfo("Bedroom loaded with {} materials and {} submeshes",
                       bedroom->GetMaterials().size(),
                       bedroom->GetSubMeshes().size());
    
    scene->BuildAccelerationStructures();
}

void SceneBuilder::BuildBedroomSplitScene(Scene* scene) {
    scene->Clear();

    glm::mat4 M(1.0f);

    // 旋转部分
    M[0] = glm::vec4(-1, 0, 0, 0);  // x -> -x'
    M[1] = glm::vec4(0, 0, 1, 0);  // y ->  z'
    M[2] = glm::vec4(0, 1, 0, 0);  // z ->  y'

    // 平移：沿 y' 轴 5 个单位（即世界 z 方向）
	glm::vec3 translate = glm::vec3(-19, -35, -14);
    M[3] = glm::vec4(translate, 1);


    scene->AddLight(CreateAreaLight(
        glm::vec3(1.0f, 35.0f, -10.0f) + translate,
        glm::vec3(25000.0f, 25000.0f, 25000.0f),
        glm::vec3(0.5, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 0.5f)
    ));




	/*glm::vec4 poslight = M * glm::vec4(glm::vec3(20.0f, -9.0f, 43.1f), 1.0f);
    scene->AddLight(CreateAreaLight(
        glm::vec3(poslight[0], poslight[1], poslight[2]),
        glm::vec3(1000.0f, 1000.0f, 1000.0f),
        glm::vec3(2.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 2.0f)
    ));*/
    /*scene->AddLight(CreatePointLight(
        glm::vec3(poslight[0], poslight[1], poslight[2]),
        glm::vec3(4000.0f, 4000.0f, 4000.0f)
    ));*/

    //glm::vec4 pos = M * glm::vec4(glm::vec3(11.2f, 20.0f, 32.7f), 1.0f);
    glm::vec4 pos = M * glm::vec4(glm::vec3(33.0f, 16.0f, 28.0f), 1.0f);
    glm::vec3 point_light = glm::vec3(pos[0], pos[1], pos[2]);
    scene->AddLight(CreatePointLight(
        point_light,
        glm::vec3(5000.0f, 5000.0f, 5000.0f)
	));
    
	int texture_u1_v1 = scene->AddTexture(PROJECT_DIR "/meshes/bedroom/split_meshes/iscv2_u1_v1.jpg");
    //int normal_u1_v1 = scene->AddTexture(PROJECT_DIR "/meshes/bedroom/split_meshes/iscv2_u1_v1_normal.png");
	grassland::LogInfo("Loaded bedroom u1_v1 texture with ID {}", texture_u1_v1);
    Material mat_u1_v1(
        glm::vec3(1.0f, 1.0f, 1.0f),
        1.0f,
        0.0f,
        glm::vec3(0.0f, 0.0f, 0.0f),
        0.0f,
        1.5f,
        texture_u1_v1,
        -1
	);
    auto bedroom_u1_v1 = std::make_shared<Entity>(
        PROJECT_DIR "/meshes/bedroom/split_meshes/iscv2_iscv2_Material_u1_v1.obj",
        mat_u1_v1,
        M
	);
	scene->AddEntity(bedroom_u1_v1); 
    int texture_u1_v2 = scene->AddTexture(PROJECT_DIR "/meshes/bedroom/split_meshes/iscv2_u1_v2.jpg");
    grassland::LogInfo("Loaded bedroom u1_v2 texture with ID {}", texture_u1_v2);
    Material mat_u1_v2(
        glm::vec3(1.0f, 1.0f, 1.0f),
        1.0f,
        0.0f,
        glm::vec3(0.0f, 0.0f, 0.0f),
        0.0f,
        1.5f,
        texture_u1_v2,
        -1
    );
    auto bedroom_u1_v2 = std::make_shared<Entity>(
        PROJECT_DIR "/meshes/bedroom/split_meshes/iscv2_iscv2_Material_u1_v2.obj",
        mat_u1_v2,
        M
    );
    scene->AddEntity(bedroom_u1_v2);
	int texture_u2_v1 = scene->AddTexture(PROJECT_DIR "/meshes/bedroom/split_meshes/iscv2_u2_v1.jpg");
	grassland::LogInfo("Loaded bedroom u2_v1 texture with ID {}", texture_u2_v1);
    Material mat_u2_v1(
        glm::vec3(1.0f, 1.0f, 1.0f),
        1.0f,
        0.0f,
        glm::vec3(0.0f, 0.0f, 0.0f),
        0.0f,
        1.5f,
        texture_u2_v1,
		-1
	);
    auto bedroom_u2_v1 = std::make_shared<Entity>(
        PROJECT_DIR "/meshes/bedroom/split_meshes/iscv2_iscv2_Material_u2_v1.obj",
        mat_u2_v1,
		M
	);
	scene->AddEntity(bedroom_u2_v1);
	int texture_u2_v2 = scene->AddTexture(PROJECT_DIR "/meshes/bedroom/split_meshes/iscv2_u2_v2.jpg");
	grassland::LogInfo("Loaded bedroom u2_v2 texture with ID {}", texture_u2_v2);
    Material mat_u2_v2(
        glm::vec3(1.0f, 1.0f, 1.0f),
        1.0f,
        0.0f,
        glm::vec3(0.0f, 0.0f, 0.0f),
        0.0f,
        1.5f,
		texture_u2_v2,
		-1
	);
    auto bedroom_u2_v2 = std::make_shared<Entity>(
        PROJECT_DIR "/meshes/bedroom/split_meshes/iscv2_iscv2_Material_u2_v2.obj",
		mat_u2_v2,
		M
	);
    scene->AddEntity(bedroom_u2_v2);
    int texture_u2_v4 = scene->AddTexture(PROJECT_DIR "/meshes/bedroom/split_meshes/iscv2_u2_v4.jpg");
    grassland::LogInfo("Loaded bedroom u2_v4 texture with ID {}", texture_u2_v4);
    Material mat_u2_v4(
        glm::vec3(1.0f, 1.0f, 1.0f),
        1.0f,
        0.0f,
        glm::vec3(0.0f, 0.0f, 0.0f),
        0.0f,
        1.5f,
        texture_u2_v4,
        -1,
		glm::vec3(0.5f, 0.5f, 0.5f)
    );
    auto bedroom_u2_v4 = std::make_shared<Entity>(
        PROJECT_DIR "/meshes/bedroom/split_meshes/iscv2_iscv2_Material_u2_v4.obj",
        mat_u2_v4,
        M
    );
    scene->AddEntity(bedroom_u2_v4);
	int texture_u3_v1 = scene->AddTexture(PROJECT_DIR "/meshes/bedroom/split_meshes/iscv2_u3_v1.jpg");
	grassland::LogInfo("Loaded bedroom u3_v1 texture with ID {}", texture_u3_v1);
    Material mat_u3_v1(
        glm::vec3(1.0f, 1.0f, 1.0f),
        1.0f,
        0.0f,
        glm::vec3(0.0f, 0.0f, 0.0f),
        0.0f,
        1.5f,
		texture_u3_v1,
		-1
	);
    auto bedroom_u3_v1 = std::make_shared<Entity>(
        PROJECT_DIR "/meshes/bedroom/split_meshes/iscv2_iscv2_Material_u3_v1.obj",
		mat_u3_v1,
		M
	);
	scene->AddEntity(bedroom_u3_v1);
	int texture_u4_v1 = scene->AddTexture(PROJECT_DIR "/meshes/bedroom/split_meshes/iscv2_u4_v1.jpg");
	grassland::LogInfo("Loaded bedroom u4_v1 texture with ID {}", texture_u4_v1);
    Material mat_u4_v1(
        glm::vec3(1.0f, 1.0f, 1.0f),
        1.0f,
        0.0f,
        glm::vec3(0.0f, 0.0f, 0.0f),
        0.0f,
		1.5f,
		texture_u4_v1,
		-1
	);
    auto bedroom_u4_v1 = std::make_shared<Entity>(
		PROJECT_DIR "/meshes/bedroom/split_meshes/iscv2_iscv2_Material_u4_v1.obj",
		mat_u4_v1,
		M
	);
	scene->AddEntity(bedroom_u4_v1);

    //scene->Clear();
    Material mirror_mat(
        glm::vec3(0.99f, 0.99f, 0.99f),
        0.01f,
        1.0f,
        glm::vec3(0.0f, 0.0f, 0.0f),
        0.0f,
        1.5f,
        -1,
        -1
    );

    // 1. 定义平移矩阵 T
    glm::mat4 T = glm::translate(glm::mat4(1.0f), glm::vec3(26.65f, -32.55f, 27.9f));

    // 2. 定义缩放矩阵 S
    glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(7.85f, 0.1f, 10.7f));

    // 3. 按照 S * M * T 的顺序相乘
    // 注意：C++ 中矩阵乘法是从左到右结合的，但逻辑是从右向左应用到顶点
    glm::mat4 M_mirror = M * T * S;
    auto mirror = std::make_shared<Entity>(
        PROJECT_DIR "/meshes/cube.obj",
        mirror_mat,
        M_mirror
	);
	scene->AddEntity(mirror);

	int normal_sphere = scene->AddTexture(PROJECT_DIR "/meshes/sphere_normal.png");
    Material metalball_mat(
        glm::vec3(0.99f, 0.99f, 0.99f),
        0.2f,
        0.8f,
        glm::vec3(0.0f, 0.0f, 0.0f),
        0.0f,
        1.5f,
        -1,
        normal_sphere
    );
	T = glm::translate(glm::mat4(1.0f), glm::vec3(24.0f, 0.0f, 3.25f));
	S = glm::scale(glm::mat4(1.0f), glm::vec3(3.0f, 3.0f, 3.0f));
	glm::mat4 M_metalball = M * T * S;
    auto metalball = std::make_shared<Entity>(
        PROJECT_DIR "/meshes/sphere.obj",
        metalball_mat,
		M_metalball
	);
    scene->AddEntity(metalball);

    Material transball_mat(
        glm::vec3(0.99f, 0.99f, 0.99f),
        0.2f,
        0.1f,
        glm::vec3(0.0f, 0.0f, 0.0f),
        1.0f,
        1.5f,
        -1,
        -1
    );
    T = glm::translate(glm::mat4(1.0f), glm::vec3(17.0f, 2.0f, 3.25f));
    S = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 5.0f, 5.0f));
    glm::mat4 M_transball = M * T * S;
    auto transball = std::make_shared<Entity>(
        PROJECT_DIR "/meshes/cube.obj",
        transball_mat,
        M_transball
    );
    scene->AddEntity(transball);

    Material glass_mat(
        glm::vec3(0.99f, 0.99f, 0.99f),
        0.01f,
        0.35f,
        glm::vec3(0.0f, 0.0f, 0.0f),
        0.9f,
        1.5f,
        -1,
        -1
	);
	T = glm::translate(glm::mat4(1.0f), glm::vec3(49.55f, -21.25f, 15.42f));
	S = glm::scale(glm::mat4(1.0f), glm::vec3(0.1f, 4.05f, 12.85f));
	glm::mat4 M_glass = M * T * S;
    auto glass = std::make_shared<Entity>(
        PROJECT_DIR "/meshes/cube.obj",
		glass_mat,
		M_glass
	);
	scene->AddEntity(glass);

    Material triangle_mat(
        glm::vec3(0.9f, 0.9f, 0.9f),
        0.8f,
        0.0f,
        glm::vec3(0.0f, 0.0f, 0.0f),
        0.0f,
        1.5f,
        -1,
        -1,
        glm::vec3(0.2f, 0.2f, 0.2f)
    );
    T = glm::translate(glm::mat4(1.0f), glm::vec3(30.0f, 20.0f, 15.0f));
    S = glm::scale(glm::mat4(1.0f), glm::vec3(8.0f, 8.0f, 8.0f));
	glm::mat4 M_triangle = M * T * S;
    auto triangle = std::make_shared<Entity>(
        PROJECT_DIR "/meshes/triangle.obj",
        triangle_mat,
        M_triangle
	);
	scene->AddEntity(triangle);

    scene->BuildAccelerationStructures();
}

void SceneBuilder::BuildToonScene(Scene* scene) {
    scene->Clear();
    
    float box_size = 1.5f;
    
    // === Cornell Box 墙壁（使用卡通材质） ===
    
    // Floor (white, toon shading)
    Material floor_mat(glm::vec3(0.85f, 0.85f, 0.85f), 0.8f, 0.0f);
    floor_mat.use_toon = 1;
    scene->AddEntity(CreateCube(
        glm::vec3(0.0f, -box_size, 0.0f),
        glm::vec3(box_size, 0.01f, box_size),
        floor_mat
    ));
    
    // Ceiling (white, toon shading)
    Material ceiling_mat(glm::vec3(0.85f, 0.85f, 0.85f), 0.8f, 0.0f);
    ceiling_mat.use_toon = 1;
    scene->AddEntity(CreateCube(
        glm::vec3(0.0f, box_size, 0.0f),
        glm::vec3(box_size, 0.01f, box_size),
        ceiling_mat
    ));
    
    // Back wall (light blue, toon shading)
    Material back_mat(glm::vec3(0.6f, 0.7f, 0.9f), 0.9f, 0.0f);
    back_mat.use_toon = 1;
    scene->AddEntity(CreateCube(
        glm::vec3(0.0f, 0.0f, -box_size),
        glm::vec3(box_size, box_size, 0.01f),
        back_mat
    ));
    
    // Left wall (warm orange, toon shading)
    Material left_mat(glm::vec3(0.95f, 0.6f, 0.3f), 0.9f, 0.0f);
    left_mat.use_toon = 1;
    scene->AddEntity(CreateCube(
        glm::vec3(-box_size, 0.0f, 0.0f),
        glm::vec3(0.01f, box_size, box_size),
        left_mat
    ));
    
    // Right wall (cool purple, toon shading)
    Material right_mat(glm::vec3(0.6f, 0.4f, 0.85f), 0.9f, 0.0f);
    right_mat.use_toon = 1;
    scene->AddEntity(CreateCube(
        glm::vec3(box_size, 0.0f, 0.0f),
        glm::vec3(0.01f, box_size, box_size),
        right_mat
    ));
    
    // === 光源 ===
    scene->AddLight(CreateAreaLight(
        glm::vec3(0.0f, box_size - 0.1f, 0.0f),
        glm::vec3(40.0f, 40.0f, 40.0f),
        glm::vec3(0.6f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 0.6f)
    ));
    
    // // 侧面点光源（增加轮廓光效果）
    // scene->AddLight(CreatePointLight(
    //     glm::vec3(-1.0f, 0.5f, 0.5f),
    //     glm::vec3(15.0f, 10.0f, 5.0f)
    // ));
    
    // === 卡通球体1（左前，皮肤色） ===
    Material sphere1_mat(glm::vec3(0.95f, 0.75f, 0.65f), 0.2f, 0.1f);  // 皮肤色
    sphere1_mat.use_toon = 1;
    scene->AddEntity(CreateSphere(
        glm::vec3(-0.7f, -box_size + 0.4f, -0.2f),
        0.4f,
        sphere1_mat
    ));
    
    // === 卡通球体2（中间偏左，蓝色金属） ===
    Material sphere2_mat(glm::vec3(0.2f, 0.6f, 0.9f), 0.1f, 0.9f);  // 蓝色金属
    sphere2_mat.use_toon = 1;
    scene->AddEntity(CreateSphere(
        glm::vec3(-0.2f, -box_size + 0.35f, 0.1f),
        0.35f,
        sphere2_mat
    ));
    
    // === 卡通球体3（右侧，红色） ===
    Material sphere3_mat(glm::vec3(0.9f, 0.3f, 0.4f), 0.5f, 0.4f);  // 红色
    sphere3_mat.use_toon = 1;
    scene->AddEntity(CreateSphere(
        glm::vec3(0.6f, -box_size + 0.38f, -0.3f),
        0.38f,
        sphere3_mat
    ));
    
    // === 卡通球体4（后方偏左，绿色） ===
    Material sphere4_mat(glm::vec3(0.3f, 0.85f, 0.4f), 0.6f, 0.2f);
    sphere4_mat.use_toon = 1;
    scene->AddEntity(CreateSphere(
        glm::vec3(-0.5f, -box_size + 0.22f, -0.8f),
        0.22f,
        sphere4_mat
    ));
    
    // === 卡通球体5（后方偏右，黄色） ===
    Material sphere5_mat(glm::vec3(0.95f, 0.85f, 0.2f), 0.4f, 0.3f);
    sphere5_mat.use_toon = 1;
    scene->AddEntity(CreateSphere(
        glm::vec3(0.4f, -box_size + 0.26f, -0.7f),
        0.26f,
        sphere5_mat
    ));
    
    // === 卡通球体6（右前方，紫色） ===
    Material sphere6_mat(glm::vec3(0.75f, 0.3f, 0.85f), 0.3f, 0.5f);
    sphere6_mat.use_toon = 1;
    scene->AddEntity(CreateSphere(
        glm::vec3(0.8f, -box_size + 0.3f, 0.3f),
        0.3f,
        sphere6_mat
    ));
    
    // === 卡通球体7（中后方，青色） ===
    Material sphere7_mat(glm::vec3(0.2f, 0.9f, 0.8f), 0.5f, 0.2f);
    sphere7_mat.use_toon = 1;
    scene->AddEntity(CreateSphere(
        glm::vec3(0.0f, -box_size + 0.2f, -0.9f),
        0.2f,
        sphere7_mat
    ));
    
    scene->BuildAccelerationStructures();
}

void SceneBuilder::BuildToonBedroomScene(Scene* scene) {
    scene->Clear();
    
    // 光源设置
    scene->AddLight(CreateAreaLight(
        glm::vec3(0.0f, 3.0f, 0.0f),
        glm::vec3(60.0f, 60.0f, 60.0f),
        glm::vec3(2.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 2.0f)
    ));
    
    // 添加点光源增强照明
    scene->AddLight(CreatePointLight(
        glm::vec3(-2.0f, 2.5f, 1.0f),
        glm::vec3(50.0f, 40.0f, 30.0f)
    ));
    
    // 加载bedroom模型并应用卡通材质
    auto bedroom = std::make_shared<Entity>(
        PROJECT_DIR "/meshes/bedroom/iscv2.obj",
        glm::mat4(1.0f),
        false  // Load materials from MTL file
    );
    bedroom->LoadMesh(PROJECT_DIR "/meshes/bedroom/iscv2.obj");
    
    // 将所有材质改为卡通材质
    auto& materials = bedroom->GetMaterials();
    for (auto& mat : materials) {
        mat.use_toon = 1;  // 启用卡通着色
        // 根据材质颜色调整metallic来控制明暗级别
        float brightness = (mat.base_color.r + mat.base_color.g + mat.base_color.b) / 3.0f;
        if (brightness > 0.7f) {
            mat.metallic = 0.1f;  // 亮色用4级
        } else if (brightness > 0.4f) {
            mat.metallic = 0.5f;  // 中等用3级
        } else {
            mat.metallic = 0.8f;  // 暗色用2级
        }
    }
    
    scene->AddEntity(bedroom);
    
    grassland::LogInfo("Toon Bedroom loaded with {} materials (all set to toon shading)",
                       bedroom->GetMaterials().size());
    
    scene->BuildAccelerationStructures();
}

void SceneBuilder::BuildVolumeScene(Scene* scene) {
    scene->Clear();
    
    float box_size = 2.0f;
    BuildCornellBoxHelper(scene, box_size);
    
    // === 顶部面光源 ===
    float light_size = 1.0f;
    scene->AddLight(CreateAreaLight(
        glm::vec3(0.0f, box_size - 0.01f, 0.0f),
        glm::vec3(15.0f, 15.0f, 15.0f),
        glm::vec3(light_size, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, light_size)
    ));
    
    // === 发光雾球（左侧）- 主要展示 Emission（体发光） ===
    Material fog_warm;
    fog_warm.base_color = glm::vec3(0.0f);
    fog_warm.roughness = 0.0f;
    fog_warm.metallic = 0.0f;
    fog_warm.transmission = 1.0f;
    fog_warm.ior = glm::vec3(1.0f);
    fog_warm.volume_density = 0.5f;
    fog_warm.volume_emission = glm::vec3(3.0f, 1.5f, 0.5f);    // 橙红色发光
    fog_warm.volume_absorption = glm::vec3(0.01f, 0.01f, 0.01f);  // 极低吸收
    fog_warm.volume_scattering = glm::vec3(0.05f, 0.05f, 0.05f);   // 极低散射
    fog_warm.volume_anisotropy = 0.0f;
    fog_warm.texture_id = -1;
    fog_warm.normal_id = -1;
    
    scene->AddEntity(CreateSphere(
        glm::vec3(-0.8f, -box_size + 0.6f, 0.0f),
        0.6f,
        fog_warm
    ));
    
    // === 散射雾球（右侧）- 主要展示 Scattering（散射） ===
    Material fog_scatter;
    fog_scatter.base_color = glm::vec3(0.0f);
    fog_scatter.roughness = 0.0f;
    fog_scatter.metallic = 0.0f;
    fog_scatter.transmission = 1.0f;
    fog_scatter.ior = glm::vec3(1.0f);
    fog_scatter.volume_density = 0.6f;
    fog_scatter.volume_emission = glm::vec3(0.5f, 0.5f, 0.5f);    // 轻微白色发光，用于展示散射
    fog_scatter.volume_absorption = glm::vec3(0.01f, 0.01f, 0.01f);   // 极低吸收
    fog_scatter.volume_scattering = glm::vec3(2.0f, 2.0f, 2.0f);   // 高散射（无色）
    fog_scatter.volume_anisotropy = 0.0f;                  // 各向同性散射
    fog_scatter.texture_id = -1;
    fog_scatter.normal_id = -1;
    
    scene->AddEntity(CreateSphere(
        glm::vec3(0.8f, -box_size + 0.6f, 0.0f),
        0.6f,
        fog_scatter
    ));
    
    // === 吸收雾球（中上）- 主要展示 Absorption（吸收） ===
    Material fog_absorb;
    fog_absorb.base_color = glm::vec3(0.0f);
    fog_absorb.roughness = 0.0f;
    fog_absorb.metallic = 0.0f;
    fog_absorb.transmission = 1.0f;
    fog_absorb.ior = glm::vec3(1.0f);
    fog_absorb.volume_density = 0.6f;
    fog_absorb.volume_emission = glm::vec3(0.0f, 0.0f, 0.0f);  // 无发光
    fog_absorb.volume_absorption = glm::vec3(1.2f, 1.2f, 1.2f);  // 高吸收（无色）
    fog_absorb.volume_scattering = glm::vec3(0.05f, 0.05f, 0.05f);   // 极低散射
    fog_absorb.volume_anisotropy = 0.0f;
    fog_absorb.texture_id = -1;
    fog_absorb.normal_id = -1;
    
    scene->AddEntity(CreateCube(
        glm::vec3(0.0f, -box_size + 0.6f, -1.0f),
        glm::vec3(0.5f, 0.5f, 0.5f),
        fog_absorb
    ));
    
    // === 前景漫反射球体（用于展示体积光的遮挡效果） ===
    // scene->AddEntity(CreateSphere(
    //     glm::vec3(0.0f, -box_size + 0.4f, 1.3f),
    //     0.3f,
    //     Material(glm::vec3(0.9f, 0.9f, 0.9f), 0.8f, 0.0f)  // 白色漫反射
    // ));
    
    grassland::LogInfo("Volume Scene created with 3 emissive volumes");
    scene->BuildAccelerationStructures();
}