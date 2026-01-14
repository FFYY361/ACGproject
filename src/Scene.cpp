#include "Scene.h"
#include "glm/gtc/matrix_transform.hpp"

#include "stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

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
    if (lights_.size() < entities_.size()) {
		grassland::LogError("MUST add lights before adding entities to the scene, now have {} lights and {} entities",
            lights_.size(), entities_.size());
    }
    lights_.push_back(*light);
    glm::vec3 pos = light->position;
    if (light->type == 0) { // Point light
        const float scale = 0.09f;
        glm::mat4 T = glm::translate(glm::mat4(1.0f), pos);
        glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(scale));
        auto point_light = std::make_shared<Entity>(
            PROJECT_DIR "/meshes/sphere.obj",
            Material(glm::vec3(0.0f, 0.0f, 0.0f), 0.0f, 0.0f, light->color, 0.0f, glm::vec3(1.0f)),
            T*S
        );
        AddEntity(point_light);
	}
    if (light->type == 1) { // Area light
        const float scale = 0.009f;
		glm::vec3 u = light->u * 0.5f;
		glm::vec3 v = light->v * 0.5f;
        glm::vec3 normal_dir = glm::normalize(glm::cross(u, v));
        glm::vec3 w = scale * normal_dir; 
        glm::mat4 R(1.0f);
        R[0] = glm::vec4(u, 0.0f);
        R[1] = glm::vec4(v, 0.0f);
        R[2] = glm::vec4(w, 0.0f);
        glm::mat4 T = glm::translate(glm::mat4(1.0f), pos - scale * w);


        auto area_light = std::make_shared<Entity>(
            PROJECT_DIR "/meshes/cube.obj",
            Material(glm::vec3(0.0f, 0.0f, 0.0f), 0.0f, 0.0f, light->color, 0.0f, glm::vec3(1.0f)),
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
	material_id_buffer_.reset();
    lights_.clear();

    // Clear texture resources and metadata to avoid stale bindings after scene switch
    textures_.clear();
    texture_infos_.clear();
    texture_info_buffer_.reset();
    texture_sampler_.reset();
    num_texture_ = 0;

    // Clear environment map resources
    environment_map_.reset();
    has_environment_map_ = false;
    environment_intensity_ = 1.0f;
    
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
    if (!tlas_ || entities_.empty() || !entity_info_buffer_) {
        return;
    }

    // Update TLAS instances with new transforms
    std::vector<grassland::graphics::RayTracingInstance> instances;
    instances.reserve(entities_.size());

    for (size_t i = 0; i < entities_.size(); ++i) {
        auto& entity = entities_[i];
        if (entity->GetBLAS()) {
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

    tlas_->UpdateInstances(instances);
    
    // Read back current entity_infos, update only transforms, then re-upload
    size_t entity_info_size = entities_.size() * sizeof(EntityInfo);
    std::vector<EntityInfo> entity_infos(entities_.size());
    
    // Read current buffer (to preserve offsets and other data)
    entity_info_buffer_->DownloadData(entity_infos.data(), entity_info_size);
    
    // Update only transform matrices
    for (size_t i = 0; i < entities_.size(); ++i) {
        entity_infos[i].objectToWorld = entities_[i]->GetTransform();
        entity_infos[i].worldToObject = glm::inverse(entities_[i]->GetTransform());
        entity_infos[i].objectToWorldPrev = entities_[i]->GetPreviousTransform();
        entity_infos[i].worldToObjectPrev = glm::inverse(entities_[i]->GetPreviousTransform());
    }
    
    // Re-upload updated entity_infos
    entity_info_buffer_->UploadData(entity_infos.data(), entity_info_size);
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
    std::vector<uint32_t> material_ids;  // Per-triangle material IDs

    uint32_t vertex_buffer_offset = 0;
    uint32_t index_buffer_offset = 0;
    uint32_t material_offset = 0;
    uint32_t material_id_buffer_offset = 0;
    
    size_t entity_idx = 0;
    for (const auto& entity : entities_) {
        EntityInfo info{};
        info.vertexBufferOffset = vertex_buffer_offset;
        info.indexBufferOffset = index_buffer_offset;
        info.materialOffset = material_offset;
        info.materialIdBufferOffset = material_id_buffer_offset;
        info.objectToWorld = entity->GetTransform();
        info.worldToObject = glm::inverse(entity->GetTransform());
        info.objectToWorldPrev = entity->GetPreviousTransform();
        info.worldToObjectPrev = glm::inverse(entity->GetPreviousTransform());
        
        // Debug log for motion blur cubes (last two entities in procedural scene)
        size_t total_entities = entities_.size();
        if (entity_idx >= total_entities - 2) {
            glm::vec3 cur_pos = glm::vec3(info.objectToWorld[3]);
            glm::vec3 prev_pos = glm::vec3(info.objectToWorldPrev[3]);
            grassland::LogInfo("UpdateBuffers - Entity {}: cur_pos=({:.2f},{:.2f},{:.2f}), prev_pos=({:.2f},{:.2f},{:.2f})",
                entity_idx, cur_pos.x, cur_pos.y, cur_pos.z, prev_pos.x, prev_pos.y, prev_pos.z);
        }

        // Collect materials from entity (may have multiple)
        const auto& entity_materials = entity->GetMaterials();
        info.numMaterials = static_cast<uint32_t>(entity_materials.size());
        
        for (const auto& mat : entity_materials) {
            materials.push_back(mat);
        }
        
        entity_infos.push_back(info);

        // Append vertex data
        const auto& mesh = entity->GetMesh();
        uint32_t num_vertices = mesh.NumVertices();
        uint32_t num_indices = mesh.NumIndices();
        uint32_t num_triangles = num_indices / 3;
        
        for (uint32_t v = 0; v < num_vertices; ++v) {
            Vertex vert{};
            grassland::Vector3<float> pos = mesh.Positions()[v];
            vert.pos = glm::vec3(pos[0], pos[1], pos[2]);
            grassland::Vector3<float> norm = mesh.Normals() ? mesh.Normals()[v] : grassland::Vector3<float>{ 0.0f, 0.0f, 0.0f };
            vert.normal = glm::vec3(norm[0], norm[1], norm[2]);
            // Add UV coordinates
            if (mesh.TexCoords()) {
                grassland::Vector2<float> uv = mesh.TexCoords()[v];
                vert.uv = glm::vec2(uv[0], uv[1]);
            } else {
                vert.uv = glm::vec2(0.0f, 0.0f);
            }
            vertices.push_back(vert);
        }
        
        // Append index data
        for (uint32_t idx = 0; idx < num_indices; ++idx) {
            indices.push_back(mesh.Indices()[idx]);
        }
        
        // Append material IDs (per triangle)
        // Read from entity's BuildMaterialIdBuffer result
        const auto& entity_material_ids = entity->GetMesh(); // TODO: need getter for material_ids_
        
        // For now, manually build material IDs from submeshes
        const auto& submeshes = entity->GetSubMeshes();
        
        // If no submeshes, create a default one (this shouldn't happen, but safety check)
        if (submeshes.empty()) {
            grassland::LogWarning("Entity {} has no submeshes, using default", entity_idx);
            for (uint32_t tri = 0; tri < num_triangles; ++tri) {
                material_ids.push_back(material_offset);
            }
        } else {
            for (const auto& submesh : submeshes) {
                uint32_t submesh_triangles = submesh.index_count / 3;
                uint32_t global_material_id = material_offset + submesh.material_index;
                
                for (uint32_t tri = 0; tri < submesh_triangles; ++tri) {
                    material_ids.push_back(global_material_id);
                }
            }
        }
        
        vertex_buffer_offset += num_vertices;
        index_buffer_offset += num_indices;
        material_offset += static_cast<uint32_t>(entity_materials.size());
        material_id_buffer_offset += num_triangles;
        entity_idx++;
    }

    EntityInfo debug_info = entity_infos[0];

    // Diagnostic mapping: print each entity -> submesh -> local material -> global material -> texture_id
    // for (size_t ei = 0; ei < entities_.size(); ++ei) {
    //     const auto& info = entity_infos[ei];
    //     const auto& ent = entities_[ei];
    //     const auto& submeshes = ent->GetSubMeshes();
    //     for (size_t si = 0; si < submeshes.size(); ++si) {
    //         uint32_t local_mat = submeshes[si].material_index;
    //         uint32_t global_mat = info.materialOffset + local_mat;
    //         int tex_id = -1;
    //         if (global_mat < materials.size()) tex_id = materials[global_mat].texture_id;
    //         grassland::LogInfo("Entity {} SubMesh {}: local_mat={}, global_mat={}, texture_id={}", ei, si, local_mat, global_mat, tex_id);
    //     }
    // }



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

    // Create/update material ID buffer
    size_t material_id_buffer_size = material_ids.size() * sizeof(uint32_t);
    if (!material_id_buffer_) {
        core_->CreateBuffer(material_id_buffer_size,
                          grassland::graphics::BUFFER_TYPE_DYNAMIC,
                          &material_id_buffer_);
    }
    material_id_buffer_->UploadData(material_ids.data(), material_id_buffer_size);
    grassland::LogInfo("Updated material ID buffer with {} triangle IDs", material_ids.size());

	// Create/update texturesinfo buffer
    if (texture_infos_.size() == 0) {
		TextureInfo dummy_tex_info(0, 0);
		texture_infos_.push_back(dummy_tex_info);
    }
	size_t texture_info_buffer_size = texture_infos_.size() * sizeof(TextureInfo);
    if (!texture_info_buffer_) {
        core_->CreateBuffer(texture_info_buffer_size,
                          grassland::graphics::BUFFER_TYPE_DYNAMIC,
                          &texture_info_buffer_);
	}
	texture_info_buffer_->UploadData(texture_infos_.data(), texture_info_buffer_size);
	grassland::LogInfo("Updated texture info buffer with {} texture infos, each of size {}", texture_infos_.size(), sizeof(TextureInfo));

    // Diagnostic: print mapping between texture_infos and actual images
    // grassland::LogInfo("Total images in textures_ (all mips combined): {}", textures_.size());
    // for (size_t i = 0; i < texture_infos_.size(); ++i) {
    //     int base = texture_infos_[i].idx;
    //     int mips = texture_infos_[i].mipLevels;
    //     grassland::LogInfo("TextureInfo[{}]: base_idx={}, mipLevels={}, occupies [{} .. {}]", i, base, mips, base, base + mips - 1);
    // }


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
    
    // Create texture sampler if not exists
    if (!texture_sampler_) {
        //grassland::graphics::SamplerInfo sampler_info(
        //    grassland::graphics::FilterMode::FILTER_MODE_LINEAR,   // min_filter: 缩小使用线性插值
        //    grassland::graphics::FilterMode::FILTER_MODE_LINEAR,   // mag_filter: 放大使用线性插值
        //    grassland::graphics::FilterMode::FILTER_MODE_LINEAR,   // mip_filter: 核心设置！层级之间也进行线性插值
        //    grassland::graphics::AddressMode::ADDRESS_MODE_REPEAT,  // address_u
        //    grassland::graphics::AddressMode::ADDRESS_MODE_REPEAT,  // address_v
        //    grassland::graphics::AddressMode::ADDRESS_MODE_REPEAT   // address_w
        //); 
        grassland::graphics::SamplerInfo sampler_info;
        // Use default sampler settings (linear filtering, repeat wrapping)
        core_->CreateSampler(sampler_info, &texture_sampler_);
        grassland::LogInfo("Created texture sampler");
    }
    
    // Ensure at least one texture exists (required for shader binding)
    if (textures_.empty()) {
        grassland::LogInfo("No textures in scene, creating dummy 1x1 white texture");
        CreateProceduralTexture(1, 1, [](float, float) { 
            return glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); 
        });
    }
}

int Scene::AddTexture(const std::string& file_path) {
    // 使用stb_image加载图片
    int width, height, channels;
    unsigned char* data = stbi_load(file_path.c_str(), &width, &height, &channels, 4); // 强制RGBA 4通道
    
    if (!data) {
        return -1;
        grassland::LogError("Failed to load texture: {}", file_path);
        // grassland::LogWarning("Creating checkerboard texture as fallback for {}", file_path);
        // 加载失败，创建棋盘格纹理作为后备
        auto checkerboard = [](float u, float v) -> glm::vec4 {
            int checker_size = 8;
            int x = static_cast<int>(u * checker_size);
            int y = static_cast<int>(v * checker_size);
            bool is_white = ((x + y) % 2) == 0;
            float c = is_white ? 0.9f : 0.1f;
            return glm::vec4(c, c, c, 1.0f);
        };
        return CreateProceduralTexture(256, 256, checkerboard);
    }
    
    grassland::LogInfo("Loaded image: {} ({}x{}, {} channels)", file_path, width, height, channels);


    int miplevels = static_cast<int>(std::floor(std::log2(std::max(width, height)))) + 1;
    int curW = width;
    int curH = height;
    unsigned char* curData = data; // 当前操作的指针
	TextureInfo tex_info(textures_.size(), miplevels);
	texture_infos_.push_back(tex_info);

    for (int level = 0; level < miplevels; ++level) {

		char filename[64];
	    snprintf(filename, sizeof(filename), "mipmap_level%d.png", level);
        stbi_write_png(filename, curW, curH, 4, curData, curW * 4);
        // 1. 创建并上传 GPU 纹理
        std::unique_ptr<grassland::graphics::Image> texture;
        core_->CreateImage(curW, curH, grassland::graphics::IMAGE_FORMAT_R8G8B8A8_UNORM, &texture);
        texture->UploadData(curData);
        textures_.push_back(std::move(texture));

        // 如果是最后一层，不需要再计算下一层
        if (level == miplevels - 1) break;

        // 2. 准备下一层数据
        int nextW = std::max(1, curW / 2);
        int nextH = std::max(1, curH / 2);
        unsigned char* nextData = (unsigned char*)malloc(nextW * nextH * 4);

        stbir_resize_uint8_linear(curData, curW, curH, 0,
            nextData, nextW, nextH, 0, STBIR_RGBA);

        // 3. 关键：释放当前层的 CPU 内存（如果是第一层用 stbi，否则用 free）
        if (level == 0) {
            stbi_image_free(curData);
        }
        else {
            free(curData);
        }

        // 4. 迭代
        curData = nextData;
        curW = nextW;
        curH = nextH;
    }

    // 循环结束后，curData 指向的是最后一层 Mipmap 的内存，需释放
    // 注意：如果 miplevels 为 1，则此处 curData 就是 data，已在上面 stbi_image_free 过了
    if (miplevels > 1) {
        free(curData);
    }
    else
    {
        stbi_image_free(curData);
    }

    num_texture_++;
    return num_texture_ - 1;

}

int Scene::CreateProceduralTexture(int width, int height, 
                                   const std::function<glm::vec4(float, float)>& generator) {
    std::vector<uint8_t> pixels(width * height * 4);
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float u = static_cast<float>(x) / width;
            float v = static_cast<float>(y) / height;
            
            glm::vec4 color = generator(u, v);
            int idx = (y * width + x) * 4;
            pixels[idx + 0] = static_cast<uint8_t>(glm::clamp(color.r * 255.0f, 0.0f, 255.0f));
            pixels[idx + 1] = static_cast<uint8_t>(glm::clamp(color.g * 255.0f, 0.0f, 255.0f));
            pixels[idx + 2] = static_cast<uint8_t>(glm::clamp(color.b * 255.0f, 0.0f, 255.0f));
            pixels[idx + 3] = static_cast<uint8_t>(glm::clamp(color.a * 255.0f, 0.0f, 255.0f));
        }
    }
    
    std::unique_ptr<grassland::graphics::Image> texture;
    core_->CreateImage(width, height,
                      grassland::graphics::IMAGE_FORMAT_R8G8B8A8_UNORM,
                      &texture);
    
    // Upload pixel data to the image
    texture->UploadData(pixels.data());
    
    int index = static_cast<int>(textures_.size());
    textures_.push_back(std::move(texture));
    
    grassland::LogInfo("Created procedural texture (index {}, {}x{})", index, width, height);
    return index;
}

grassland::graphics::Image* Scene::GetTexture(int index) const {
    if (index < 0 || index >= static_cast<int>(textures_.size())) {
        return nullptr;
    }
    return textures_[index].get();
}

bool Scene::LoadEnvironmentMap(const std::string& file_path) {
    // Load HDR environment map (expects .hdr format)
    int width, height, channels;
    
    // Use stbi_loadf for HDR images to preserve float data
    float* data = stbi_loadf(file_path.c_str(), &width, &height, &channels, 4);
    if (!data) {
        grassland::LogError("Failed to load environment map: {}", file_path);
        has_environment_map_ = false;
        return false;
    }
    
    grassland::LogInfo("Loaded environment map: {} ({}x{}, {} channels)", 
                      file_path, width, height, channels);
    
    // Create HDR texture with R32G32B32A32_SFLOAT format
    core_->CreateImage(width, height,
                      grassland::graphics::IMAGE_FORMAT_R32G32B32A32_SFLOAT,
                      &environment_map_);
    
    // Upload float data
    environment_map_->UploadData(data);
    
    stbi_image_free(data);
    
    has_environment_map_ = true;
    grassland::LogInfo("Environment map loaded successfully");
    
    return true;
}
