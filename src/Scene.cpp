#include "Scene.h"
#include "glm/gtc/matrix_transform.hpp"

Scene::Scene(grassland::graphics::Core* core)
    : core_(core) {
}

Scene::~Scene() {
    Clear();
}

void Scene::AddEntity(std::shared_ptr<Entity> entity) {
    if (!entity || !entity->IsValid()) {
        grassland::LogError("Cannot add invalid entity to scene");
        return;
    }

    // Build BLAS for the entity
    entity->BuildBLAS(core_);
    
    entities_.push_back(entity);
    grassland::LogInfo("Added entity to scene (total: {})", entities_.size());
}

void Scene::AddLight(std::shared_ptr<Light> light) {
    if (!light) {
        grassland::LogError("Cannot add invalid light to scene");
        return;
    }
    lights_.push_back(*light);
    const float scale = 0.09f;
    glm::vec3 pos = light->position;
    glm::mat4 T = glm::translate(glm::mat4(1.0f), pos);
    if (light->type == 0) { // Point light
        glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(scale));
        auto point_light = std::make_shared<Entity>(
            PROJECT_DIR "/meshes/sphere.obj",
            Material(glm::vec3(1.0f, 1.0f, 1.0f), 0.0f, 0.0f, light->color),
		    T*S
        );
        AddEntity(point_light);
	}
    if (light->type == 1) { // Area light

		glm::vec3 u = light->u * 0.5f;
		glm::vec3 v = light->v * 0.5f;
        glm::vec3 normal_dir = glm::normalize(glm::cross(u, v));
        glm::vec3 w = scale * normal_dir; 
        glm::mat4 R(1.0f);
        R[0] = glm::vec4(u, 0.0f);
        R[1] = glm::vec4(v, 0.0f);
        R[2] = glm::vec4(w, 0.0f);


        auto area_light = std::make_shared<Entity>(
            PROJECT_DIR "/meshes/cube.obj",
			Material(glm::vec3(0.0f, 0.0f, 0.0f), 0.0f, 0.0f, light->color),
			T * R
		);
        AddEntity(area_light);
	}
    grassland::LogInfo("Added light to scene (total: {})", lights_.size());
}

void Scene::Clear() {
    entities_.clear();
    tlas_.reset();
    materials_buffer_.reset();
	vertices_buffer_.reset();
	indices_buffer_.reset();
	entity_info_buffer_.reset();
	light_info_buffer_.reset();
    lights_.clear();
	grassland::LogInfo("Cleared scene");
}

void Scene::BuildAccelerationStructures() {
    if (entities_.empty()) {
        grassland::LogWarning("No entities to build acceleration structures");
        return;
    }

    // Create TLAS instances from all entities
    std::vector<grassland::graphics::RayTracingInstance> instances;
    instances.reserve(entities_.size());

    for (size_t i = 0; i < entities_.size(); ++i) {
        auto& entity = entities_[i];
        if (entity->GetBLAS()) {
            // Create instance with entity's transform
            // instanceCustomIndex is used to index into materials buffer
            // Convert mat4 to mat4x3 (drop the last row which is always [0,0,0,1] for affine transforms)
            glm::mat4x3 transform_3x4 = glm::mat4x3(entity->GetTransform());
            
            auto instance = entity->GetBLAS()->MakeInstance(
                transform_3x4,
                static_cast<uint32_t>(i),  // instanceCustomIndex for material lookup
                0xFF,                       // instanceMask
                0,                          // instanceShaderBindingTableRecordOffset
                grassland::graphics::RAYTRACING_INSTANCE_FLAG_NONE
            );
            instances.push_back(instance);
        }
    }

    // Build TLAS
    core_->CreateTopLevelAccelerationStructure(instances, &tlas_);
    grassland::LogInfo("Built TLAS with {} instances", instances.size());

    // Update materials buffer
    //UpdateMaterialsBuffer();
	UpdateBuffers();
}

void Scene::UpdateInstances() {
    if (!tlas_ || entities_.empty()) {
        return;
    }

    // Recreate instances with updated transforms
    std::vector<grassland::graphics::RayTracingInstance> instances;
    instances.reserve(entities_.size());

    for (size_t i = 0; i < entities_.size(); ++i) {
        auto& entity = entities_[i];
        if (entity->GetBLAS()) {
            // Convert mat4 to mat4x3
            glm::mat4x3 transform_3x4 = glm::mat4x3(entity->GetTransform());
            
            auto instance = entity->GetBLAS()->MakeInstance(
                transform_3x4,
                static_cast<uint32_t>(i),
                0xFF,
                0,
                grassland::graphics::RAYTRACING_INSTANCE_FLAG_NONE
            );
            instances.push_back(instance);
        }
    }

    // Update TLAS
    tlas_->UpdateInstances(instances);
}

//void Scene::UpdateMaterialsBuffer() {
//    if (entities_.empty()) {
//        return;
//    }
//
//    // Collect all materials
//    std::vector<Material> materials;
//    materials.reserve(entities_.size());
//
//    for (const auto& entity : entities_) {
//        materials.push_back(entity->GetMaterial());
//    }
//
//    // Create/update materials buffer
//    size_t buffer_size = materials.size() * sizeof(Material);
//    
//    if (!materials_buffer_) {
//        core_->CreateBuffer(buffer_size, 
//                          grassland::graphics::BUFFER_TYPE_DYNAMIC, 
//                          &materials_buffer_);
//    }
//    
//    materials_buffer_->UploadData(materials.data(), buffer_size);
//    grassland::LogInfo("Updated materials buffer with {} materials", materials.size());
//}


void Scene::UpdateBuffers() {
    if (entities_.empty()) {
        return;
    }
	// Collect all materials, entity infos
    std::vector<Material> materials;
	materials.reserve(entities_.size());
	std::vector<EntityInfo> entity_infos;
	entity_infos.reserve(entities_.size());

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    uint32_t vertex_buffer_offset = 0;
    uint32_t index_buffer_offset = 0;
    uint32_t material_offset = 0;
    for (const auto& entity : entities_) {
        EntityInfo info{};
        info.vertexBufferOffset = vertex_buffer_offset;
        info.indexBufferOffset = index_buffer_offset;
        info.materialOffset = material_offset;
        info.objectToWorld = entity->GetTransform();
        info.worldToObject = glm::inverse(entity->GetTransform());


        materials.push_back(entity->GetMaterial());
        entity_infos.push_back(info);

        // Append vertex data
        const auto& mesh = entity->GetMesh();
        uint32_t num_vertices = mesh.NumVertices();
        uint32_t num_indices = mesh.NumIndices();
        for (uint32_t v = 0; v < num_vertices; ++v) {
            Vertex vert{};
            grassland::Vector3<float> pos = mesh.Positions()[v];
            //const auto pos = mesh.Positions()[v];
            vert.pos = glm::vec3(pos[0], pos[1], pos[2]);
            grassland::Vector3<float> norm = mesh.Normals() ? mesh.Normals()[v] : grassland::Vector3<float>{ 0.0f, 0.0f, 0.0f };
            vert.normal = glm::vec3(norm[0], norm[1], norm[2]);
            //vert.normal = mesh.Normals() ? mesh.Normals()[v] : glm::vec3(0.0f, 0.0f, 0.0f);
            vertices.push_back(vert);
        }
        // Append index data (with offset)
        for (uint32_t idx = 0; idx < num_indices; ++idx) {
            indices.push_back(mesh.Indices()[idx]);
        }
        vertex_buffer_offset += num_vertices;
        index_buffer_offset += num_indices;
    }

    EntityInfo debug_info = entity_infos[0];/*
    grassland::LogInfo("First entity info: vertexOffset={}, indexOffset={}, materialOffset={}, objectToWorld={}",
        debug_info.vertexBufferOffset,
        debug_info.indexBufferOffset,
		debug_info.materialOffset);*/
    grassland::LogInfo("objectToWorld matrix first row: [{}, {}, {}, {}]",
        debug_info.objectToWorld[0][0],
        debug_info.objectToWorld[0][1],
        debug_info.objectToWorld[0][2],
		debug_info.objectToWorld[0][3]);
    grassland::LogInfo("objectToWorld matrix second row: [{}, {}, {}, {}]",
        debug_info.objectToWorld[1][0],
        debug_info.objectToWorld[1][1],
        debug_info.objectToWorld[1][2],
        debug_info.objectToWorld[1][3]);
    grassland::LogInfo("objectToWorld matrix third row: [{}, {}, {}, {}]",
        debug_info.objectToWorld[2][0],
        debug_info.objectToWorld[2][1],
        debug_info.objectToWorld[2][2],
		debug_info.objectToWorld[2][3]);
    grassland::LogInfo("objectToWorld matrix fourth row: [{}, {}, {}, {}]",
        debug_info.objectToWorld[3][0],
        debug_info.objectToWorld[3][1],
		debug_info.objectToWorld[3][2],
		debug_info.objectToWorld[3][3]);



	// Create/update materials buffer
	size_t materials_buffer_size = materials.size() * sizeof(Material);
    if (!materials_buffer_) {
        core_->CreateBuffer(materials_buffer_size,
                          grassland::graphics::BUFFER_TYPE_DYNAMIC,
                          &materials_buffer_);
	}
    materials_buffer_->UploadData(materials.data(), materials_buffer_size);
	grassland::LogInfo("Updated materials buffer with {} materials", materials.size());

	// Create/update info buffer
	size_t entity_info_buffer_size = entity_infos.size() * sizeof(EntityInfo);
    if (!entity_info_buffer_) {
        core_->CreateBuffer(entity_info_buffer_size,
                          grassland::graphics::BUFFER_TYPE_DYNAMIC,
                          &entity_info_buffer_);
    }
	entity_info_buffer_->UploadData(entity_infos.data(), entity_info_buffer_size);
    grassland::LogInfo("Updated entity info buffer with {} entries, each of size {}", entity_infos.size(), sizeof(EntityInfo));

    // Create/update vertex buffer
    size_t vertices_buffer_size = vertices.size() * sizeof(Vertex);
    if (!vertices_buffer_) {
        core_->CreateBuffer(vertices_buffer_size,
                          grassland::graphics::BUFFER_TYPE_DYNAMIC,
                          &vertices_buffer_);
    }
    vertices_buffer_->UploadData(vertices.data(), vertices_buffer_size);
    grassland::LogInfo("Updated vertex buffer with {} vertices, each of size {}", vertices.size(), sizeof(Vertex));

    // Create/update index buffer
    size_t indices_buffer_size = indices.size() * sizeof(uint32_t);
    if (!indices_buffer_) {
        core_->CreateBuffer(indices_buffer_size,
                          grassland::graphics::BUFFER_TYPE_DYNAMIC,
                          &indices_buffer_);
    }
    indices_buffer_->UploadData(indices.data(), indices_buffer_size);
	grassland::LogInfo("Updated index buffer with {} indices, each of size {}", indices.size(), sizeof(uint32_t));



	light_info_.num_light = static_cast<int>(lights_.size());
    if (light_info_.num_light == 0)
    {
        Light dummy_light;
        lights_.push_back(dummy_light);
    }
	size_t light_info_buffer_size = sizeof(LightInfo);
	size_t lights_buffer_size = lights_.size() * sizeof(Light);

    if (!lights_buffer_) {
        core_->CreateBuffer(lights_buffer_size,
                          grassland::graphics::BUFFER_TYPE_DYNAMIC,
                          &lights_buffer_);
	}
	lights_buffer_->UploadData(lights_.data(), lights_buffer_size);
	grassland::LogInfo("Updated lights buffer with {} lights, each of size {}", lights_.size(), sizeof(Light));

    if (!light_info_buffer_) {
        core_->CreateBuffer(light_info_buffer_size,
                          grassland::graphics::BUFFER_TYPE_DYNAMIC,
                          &light_info_buffer_);
	}
	light_info_buffer_->UploadData(&light_info_, light_info_buffer_size);
	grassland::LogInfo("The type of first light: {}", lights_[0].type);

}
