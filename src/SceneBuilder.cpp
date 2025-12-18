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
    //auto light1 = std::make_shared<Light>(
    //    0,
    //    glm::vec3(1.5f, 3.0f, 2.0f),
    //    glm::vec3(50.0f, 50.0f, 50.0f)
    //);
    //auto light1 = std::make_shared<Light>(
    //    0,
    //    glm::vec3(0.0f, 5.0f, -1.1f),
    //    glm::vec3(15.0f, 15.0f, 15.0f)
    //);
    //scene->AddLight(light1);
    /*auto light2 = std::make_shared<Light>(
        0,
        glm::vec3(1.5f, 0.5f, -2.0f),
        glm::vec3(20.0f, 20.0f, 20.0f)
    );*/
    //scene->AddLight(light2);

    //auto light = std::make_shared<Light>(
    //    1,
    //    glm::vec3(0.0f, 2.0f, -1.5f),
    //    glm::vec3(15.0f, 15.0f, 15.0f),
    //    glm::vec3(0.0f, 0.0f, -1.0f),
    //    glm::vec3(1.0f, 0.0f, 0.0f),
    //    1.0f
    //);
    //scene->AddLight(light);


    // Ground plane
    //scene->AddEntity(CreateGroundPlane());
    
    // Yellow sphere
    //scene->AddEntity(CreateSphere(
    //    glm::vec3(0.0f, 0.5f, -2.0f),
    //    0.5f,
    //    Material(glm::vec3(1.0f, 1.0f, 0.0f), 1.0f, 0.0f)
    //));

    //// Metallic white sphere
    //scene->AddEntity(CreateSphere(
    //    glm::vec3(2.0f, 0.5f, 0.0f),
    //    0.5f,
    //    Material(glm::vec3(1.0f, 1.0f, 1.0f), 0.2f, 1.0f, glm::vec3(0.0f), 0.0f, 1.1f)
    //));
    
    //// Blue cube
  //  scene->AddEntity(CreateCube(
  //      glm::vec3(0.0f, 0.5f, 0.0f),
		//glm::vec3(1.0f, 1.0f, 1.0f),
  //      Material(glm::vec3(1.0f, 1.0f, 1.0f), 0.5f, 0.3f, glm::vec3(0.0f), 0.95f, 1.01f)
  //  ));

  //  
  //  scene->BuildAccelerationStructures();



    scene->Clear();

    float box_size = 1.5f;

    // === Cornell Box 标准6面墙（与Box3相同） ===

    // === 顶部面光源 ===
    float light_size = 0.7f;
    scene->AddLight(CreateAreaLight(
        glm::vec3(0.0f, box_size-0.1f, -0.4f),
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
    scene->AddEntity(CreateGroundPlane(
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

	int idx_text = scene->AddTexture(PROJECT_DIR "/meshes/cube_normal.png");
    grassland::LogInfo("Added texture with index: {}", idx_text);


    // === 场景物体2：透明玻璃立方体（前方，旋转45度） ===
    scene->AddEntity(std::make_shared<Entity>(
        PROJECT_DIR "/meshes/cube.obj",
        Material(glm::vec3(1.0f, 1.0f, 1.0f), 0.8f, 0.0f, glm::vec3(0.0f), 0.00f, 1.5f, idx_text, -1),  // 透明玻璃 IOR=1.5
        glm::scale(
            glm::rotate(
                glm::translate(glm::mat4(1.0f), glm::vec3(0.5f, -box_size + 0.6f, 0.4f)),
                glm::radians(45.0f),
                glm::vec3(0.0f, 1.0f, 0.0f)  // 绕Y轴旋转
            ),
            glm::vec3(0.5f, 0.5f, 0.5f)
        )
    ));


    //scene->AddEntity(CreateCube(
    //    glm::vec3(0.5f, -box_size + 0.6f, 0.4f),
    //    glm::vec3(0.5f, 0.5f, 0.5f),
    //    Material(glm::vec3(1.0f, 1.0f, 1.0f), 0.4f, 0.0f, glm::vec3(0.0f), 0.00f, 1.01f, idx_text)
    //));


    scene->BuildAccelerationStructures();
}

void SceneBuilder::BuildCornellBox(Scene* scene) {
    scene->Clear();
    
    float box_size = 2.0f;
    float wall_thickness = 0.1f;

    // Point light in the top corner
    auto light = std::make_shared<Light>(
        0,  // type: 0 = Point light
        glm::vec3(box_size * 0.8f, box_size * 0.9f, -box_size * 0.8f),  // 右上后角落
        glm::vec3(10.0f, 10.0f, 10.0f)
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

void SceneBuilder::BuildCornellBox3(Scene* scene) {
    scene->Clear();
    
    float box_size = 2.0f;
    
    // === Cornell Box 标准6面墙 ===
    
    // Floor (white)
    scene->AddEntity(CreateGroundPlane(
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
        Material(glm::vec3(0.25f, 0.25f, 0.75f), 0.9f, 0.0f)
    ));
    
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
    
    // === Cornell Box 标准6面墙（与Box3相同） ===
    
    // Floor (white)
    scene->AddEntity(CreateGroundPlane(
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
        Material(glm::vec3(0.25f, 0.25f, 0.75f), 0.9f, 0.0f)
    ));
    
    // === 顶部面光源 ===
    float light_size = 0.7f;
    scene->AddLight(CreateAreaLight(
        glm::vec3(0.0f, box_size - 0.01f, 0.0f),
        glm::vec3(24.0f, 24.0f, 24.0f),
        glm::vec3(light_size, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, light_size)
    ));
    
    // === 场景物体1：镜面球（左后） ===
    scene->AddEntity(CreateSphere(
        glm::vec3(-0.7f, -box_size + 0.5f, -0.5f),
        0.5f,
        Material(glm::vec3(0.95f, 0.95f, 0.95f), 0.02f, 1.0f)  // 高度镜面金属
    ));
    
    // === 场景物体2：透明玻璃立方体（前方，旋转45度） ===
    scene->AddEntity(std::make_shared<Entity>(
        PROJECT_DIR "/meshes/cube.obj",
        Material(glm::vec3(1.0f, 1.0f, 1.0f), 0.0f, 0.0f, glm::vec3(0.0f), 1.0f, 1.3f),  // 透明玻璃 IOR=1.5
        glm::scale(
            glm::rotate(
                glm::translate(glm::mat4(1.0f), glm::vec3(0.5f, -box_size + 0.5f, 0.4f)),
                glm::radians(45.0f),
                glm::vec3(0.0f, 1.0f, 0.0f)  // 绕Y轴旋转
            ),
            glm::vec3(0.5f, 0.5f, 0.5f)
        )
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

void SceneBuilder::BuildProceduralScene(Scene* scene) {
    scene->Clear();
    
    float box_size = 2.0f;
    
    // === 加载纹理 ===
    int mygo_texture_id = scene->AddTexture(PROJECT_DIR "/meshes/textures/texture_mygo.jpeg");
    int thu_texture_id = scene->AddTexture(PROJECT_DIR "/meshes/textures/texture_thu.png");
    
    // === 顶部圆盘面光源 ===
    float disk_radius = 0.6f;
    scene->AddLight(CreateAreaLight(
        glm::vec3(0.0f, box_size - 0.01f, 0.0f),
        glm::vec3(30.0f, 30.0f, 30.0f),
        glm::vec3(disk_radius, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, disk_radius)
    ));
    
    // === Cornell Box 6面墙 ===
    
    // Floor (white)
    scene->AddEntity(CreateGroundPlane(
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
        Material(glm::vec3(0.25f, 0.25f, 0.75f), 0.9f, 0.0f)
    ));
    
    // === 场景物体：两个带纹理的立方体（旋转让正面朝向相机） ===
    
    // 左侧立方体：MyGO纹理
    scene->AddEntity(std::make_shared<Entity>(
        PROJECT_DIR "/meshes/cube_uv.obj",
        Material(glm::vec3(1.0f, 1.0f, 1.0f), 0.8f, 0.0f, glm::vec3(0.0f), 0.0f, 1.0f, mygo_texture_id),
        glm::scale(
            glm::translate(glm::mat4(1.0f), glm::vec3(-0.7f, -box_size + 0.6f, 0.0f)),
            glm::vec3(0.6f, 0.6f, 0.6f)
        )
    ));
    
    // 右侧立方体：清华纹理
    scene->AddEntity(std::make_shared<Entity>(
        PROJECT_DIR "/meshes/cube_uv.obj",
        Material(glm::vec3(1.0f, 1.0f, 1.0f), 0.8f, 0.0f, glm::vec3(0.0f), 0.0f, 1.0f, thu_texture_id),
        glm::scale(
            glm::translate(glm::mat4(1.0f), glm::vec3(0.7f, -box_size + 0.6f, 0.0f)),
            glm::vec3(0.6f, 0.6f, 0.6f)
        )
    ));
    
    scene->BuildAccelerationStructures();
}

void SceneBuilder::BuildBedroomScene(Scene* scene) {
    scene->Clear();
    
    // // === 加载bedroom纹理 ===
    // int tex1 = scene->AddTexture(PROJECT_DIR "/meshes/bedroom/iscv2_u1_v1.jpg");
    // int tex2 = scene->AddTexture(PROJECT_DIR "/meshes/bedroom/iscv2_u1_v2.jpg");
    // int tex3 = scene->AddTexture(PROJECT_DIR "/meshes/bedroom/iscv2_u2_v1.jpg");
    // int tex4 = scene->AddTexture(PROJECT_DIR "/meshes/bedroom/iscv2_u2_v2.jpg");
    // int tex5 = scene->AddTexture(PROJECT_DIR "/meshes/bedroom/iscv2_u2_v4.jpg");
    // int tex6 = scene->AddTexture(PROJECT_DIR "/meshes/bedroom/iscv2_u3_v1.jpg");
    // int tex7 = scene->AddTexture(PROJECT_DIR "/meshes/bedroom/iscv2_u4_v1.jpg");
    
    // === 加载bedroom模型 ===
    // 主要的bedroom mesh
    scene->AddEntity(std::make_shared<Entity>(
        PROJECT_DIR "/meshes/bedroom/iscv2.obj",
        Material(glm::vec3(1.0f, 1.0f, 1.0f), 0.8f, 0.0f, glm::vec3(0.0f), 0.0f, 1.0f),
        glm::scale(
            glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)),
            glm::vec3(1.0f, 1.0f, 1.0f)
        )
    ));
    
    // // === 添加一些装饰物体 ===
    
    // // 镜面金属球（装饰品）
    // scene->AddEntity(CreateSphere(
    //     glm::vec3(1.5f, 0.5f, 1.0f),
    //     0.3f,
    //     Material(glm::vec3(0.95f, 0.95f, 0.95f), 0.05f, 1.0f)
    // ));
    
    // // 玻璃球（装饰品）
    // scene->AddEntity(CreateSphere(
    //     glm::vec3(-1.5f, 0.5f, 1.0f),
    //     0.3f,
    //     Material(glm::vec3(1.0f, 1.0f, 1.0f), 0.0f, 0.0f, glm::vec3(0.0f), 1.0f, 1.5f)
    // ));
    
    scene->BuildAccelerationStructures();
}

