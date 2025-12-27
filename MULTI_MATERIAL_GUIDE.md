# 多材质支持使用指南

## 概述

现在Entity类完全支持从OBJ+MTL文件加载多材质模型！

## 架构说明

### SubMesh 结构
```cpp
struct SubMesh {
    uint32_t index_start;     // 在索引缓冲区中的起始位置
    uint32_t index_count;     // 该子网格的索引数量
    uint32_t material_index;  // 材质索引
};
```

### 数据流
```
OBJ/MTL文件 → Entity::LoadMeshWithMaterials() 
→ materials_[] + submeshes_[]
→ BuildMaterialIdBuffer() 生成 material_ids_[]
→ Scene::UpdateBuffers() 上传到GPU
→ 着色器通过 material_ids[triangle_idx] 查找材质
```

## 使用方法

### 1. 单材质模式（保持向后兼容）
```cpp
auto entity = std::make_shared<Entity>(
    PROJECT_DIR "/meshes/sphere.obj",
    Material(glm::vec3(1.0f, 0.0f, 0.0f), 0.5f, 0.0f),  // 红色漫反射
    glm::mat4(1.0f)
);
scene->AddEntity(entity);
```

### 2. 多材质模式（自动加载MTL）
```cpp
// 方法1：直接构造
auto bedroom = std::make_shared<Entity>(
    PROJECT_DIR "/meshes/bedroom/iscv2.obj",
    glm::mat4(1.0f),
    true  // 第三个参数为true表示加载MTL材质
);
scene->AddEntity(bedroom);

// 方法2：使用辅助函数
auto model = SceneBuilder::CreateMultiMaterialEntity(
    PROJECT_DIR "/meshes/bedroom/iscv2.obj",
    glm::translate(glm::mat4(1.0f), glm::vec3(0, 1, 0))
);
scene->AddEntity(model);
```

### 3. 在SceneBuilder中使用
```cpp
void SceneBuilder::BuildBedroomScene(Scene* scene) {
    scene->Clear();
    
    // 添加光源
    scene->AddLight(CreateAreaLight(
        glm::vec3(0.0f, 3.0f, 0.0f),
        glm::vec3(50.0f, 50.0f, 50.0f),
        glm::vec3(2.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 2.0f)
    ));
    
    // 加载多材质模型
    auto bedroom = CreateMultiMaterialEntity(
        PROJECT_DIR "/meshes/bedroom/iscv2.obj"
    );
    scene->AddEntity(bedroom);
    
    scene->BuildAccelerationStructures();
}
```

## MTL 材质转换

TinyObjLoader材质 → 我们的Material结构：

| MTL属性 | Material属性 | 转换规则 |
|---------|-------------|---------|
| Kd (diffuse) | base_color | 直接映射 RGB |
| Ns (shininess) | roughness | `1.0 - Ns/100.0` |
| Pm (metallic) | metallic | 直接映射 |
| Ke (emission) | emission | 直接映射 RGB |
| Ni (IOR) | ior | 直接映射 |
| d (dissolve) | transmission | `1.0 - d` (当d<0.99) |
| map_Kd | texture_id | TODO: 未来支持 |

## 调试信息

加载时会输出：
```
[INFO] Loaded 7 materials from MTL file
[INFO]   Material: iscv2_Material_u1_v1 (diffuse: 1, 1, 1)
[INFO]   Material: iscv2_Material_u2_v1 (diffuse: 1, 1, 1)
[INFO] Created 15 submeshes from OBJ file
[INFO] Built material ID buffer: 383513 triangles, 15 submeshes
[INFO] Bedroom loaded with 7 materials and 15 submeshes
```

## 着色器端

着色器自动通过以下方式获取正确材质：
```hlsl
// 获取全局三角形索引
uint triangle_global_idx = (entity_info.indexBufferOffset / 3) + prim;

// 查找该三角形的材质ID
uint material_id = material_ids[triangle_global_idx];

// 加载材质
Material mat = materials[material_id];
```

## 性能考虑

- **内存**：每个三角形需要额外4字节（material ID）
- **查找**：O(1) 材质查找，无性能损失
- **兼容性**：单材质模型仍然高效（所有material_id=0）

## 示例场景

```cpp
// 在 main.cpp 或你的场景选择逻辑中：
SceneBuilder::BuildBedroomScene(scene.get());
```

现在你可以加载任何带.mtl文件的.obj模型！
