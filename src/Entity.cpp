#include "Entity.h"
#include <filesystem>

// For MTL parsing (implementation already in grassland_math library)
#include "tiny_obj_loader.h"

Entity::Entity(const std::string& obj_file_path, 
               const Material& material,
               const glm::mat4& transform)
    : material_(material)
    , transform_(transform)
    , prev_transform_(transform)
    , mesh_loaded_(false) {
    
    LoadMesh(obj_file_path);
    
    // Initialize materials_ with the provided material (for single-material entities)
    if (materials_.empty()) {
        materials_.push_back(material_);
    }
}

// Constructor for multi-material loading
Entity::Entity(const std::string& obj_file_path,
               const glm::mat4& transform,
               bool load_materials_from_mtl)
    : material_(Material())
    , transform_(transform)
    , prev_transform_(transform)
    , mesh_loaded_(false) {
    
    if (load_materials_from_mtl) {
        LoadMeshWithMaterials(obj_file_path);
    } else {
        LoadMesh(obj_file_path);
    }
    
    // Ensure at least one material exists
    if (materials_.empty()) {
        materials_.push_back(material_);
    }
}

Entity::~Entity() {
    blas_.reset();
    index_buffer_.reset();
    vertex_buffer_.reset();
    uv_buffer_.reset();
    material_id_buffer_.reset();
}

bool Entity::LoadMesh(const std::string& obj_file_path) {
    // Try to load the OBJ file
    std::string full_path = grassland::FindAssetFile(obj_file_path);
    
    if (mesh_.LoadObjFile(full_path) != 0) {
        grassland::LogError("Failed to load mesh from: {}", obj_file_path);
        mesh_loaded_ = false;
        return false;
    }

    grassland::LogInfo("Successfully loaded mesh: {} ({} vertices, {} indices)", 
                       obj_file_path, mesh_.NumVertices(), mesh_.NumIndices());
    
    // If no submeshes defined, create a default one covering the entire mesh
    if (submeshes_.empty()) {
        SubMesh default_submesh;
        default_submesh.index_start = 0;
        default_submesh.index_count = mesh_.NumIndices();
        default_submesh.material_index = 0;
        submeshes_.push_back(default_submesh);
        grassland::LogInfo("Created default submesh (single material)");
    }
    
    mesh_loaded_ = true;
    return true;
}

void Entity::BuildMaterialIdBuffer() {
    // Build per-triangle material IDs from submesh definitions
    size_t num_triangles = mesh_.NumIndices() / 3;
    material_ids_.clear();
    material_ids_.resize(num_triangles, 0);
    
    for (const auto& submesh : submeshes_) {
        uint32_t start_triangle = submesh.index_start / 3;
        uint32_t num_submesh_triangles = submesh.index_count / 3;
        
        for (uint32_t i = 0; i < num_submesh_triangles; ++i) {
            material_ids_[start_triangle + i] = submesh.material_index;
        }
    }
    
    grassland::LogInfo("Built material ID buffer: {} triangles, {} submeshes", 
                       num_triangles, submeshes_.size());
}

bool Entity::LoadMeshWithMaterials(const std::string& obj_file_path) {
    std::string full_path = grassland::FindAssetFile(obj_file_path);
    
    // First load the mesh normally
    if (mesh_.LoadObjFile(full_path) != 0) {
        grassland::LogError("Failed to load mesh from: {}", obj_file_path);
        mesh_loaded_ = false;
        return false;
    }
    
    // Now use tinyobjloader to parse materials
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> obj_materials;
    std::string warn, err;
    
    std::filesystem::path path(full_path);
    std::string mtl_base_dir = path.parent_path().string();
    
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &obj_materials, &warn, &err, 
                                full_path.c_str(), mtl_base_dir.c_str());
    
    if (!warn.empty()) {
        grassland::LogWarning("TinyObjLoader: {}", warn);
    }
    
    if (!err.empty()) {
        grassland::LogError("TinyObjLoader: {}", err);
    }
    
    if (!ret) {
        grassland::LogError("Failed to parse OBJ materials");
        // Fall back to single material
        submeshes_.push_back(SubMesh(0, mesh_.NumIndices(), 0));
        materials_.push_back(Material());
        mesh_loaded_ = true;
        return false;
    }
    
    grassland::LogInfo("Loaded {} materials from MTL file", obj_materials.size());
    
    // debug
    // Verify mesh and shapes consistency
    size_t total_tinyobj_indices = 0;
    for (const auto& shape : shapes) {
        total_tinyobj_indices += shape.mesh.indices.size();
    }
    grassland::LogInfo("Mesh indices: grassland={}, tinyobj={}", 
                      mesh_.NumIndices(), total_tinyobj_indices);
    
    if (mesh_.NumIndices() != total_tinyobj_indices) {
        grassland::LogError("Index count mismatch between grassland and tinyobj loaders!");
    }
    
    // Convert tinyobj materials to our Material format
    materials_.clear();
    for (const auto& mat : obj_materials) {
        Material our_mat;
        our_mat.base_color = glm::vec3(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);
        our_mat.roughness = 1.0f - mat.shininess / 100.0f;  // Approximate conversion
        our_mat.metallic = mat.metallic;
        our_mat.emission = glm::vec3(mat.emission[0], mat.emission[1], mat.emission[2]);
        our_mat.ior = glm::vec3(mat.ior);  // Convert float to vec3 (no dispersion by default)
        our_mat.transmission = mat.dissolve < 0.99f ? (1.0f - mat.dissolve) : 0.0f;
        our_mat.texture_id = -1;  // TODO: Load textures from mat.diffuse_texname
        our_mat.normal_id = -1;
        our_mat.use_vertex_color = 0;
        
        materials_.push_back(our_mat);
        grassland::LogInfo("  Material: {} (diffuse: {}, {}, {})", 
                          mat.name, mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);
    }
    
    // If no materials, create a default one
    if (materials_.empty()) {
        materials_.push_back(Material());
        grassland::LogInfo("No materials found, using default");
    }
    
    // Build submeshes from shapes and material IDs
    submeshes_.clear();
    uint32_t index_offset = 0;
    
    for (const auto& shape : shapes) {
        // Group consecutive faces with same material into submeshes
        if (shape.mesh.material_ids.empty()) {
            // No material IDs, treat as single submesh
            SubMesh submesh;
            submesh.index_start = index_offset;
            submesh.index_count = shape.mesh.indices.size();
            submesh.material_index = 0;
            submeshes_.push_back(submesh);
            index_offset += submesh.index_count;
        } else {
            // Build submeshes based on material changes
            int current_mat_id = -1;
            SubMesh current_submesh;
            
            for (size_t face_idx = 0; face_idx < shape.mesh.material_ids.size(); ++face_idx) {
                int mat_id = shape.mesh.material_ids[face_idx];
                
                // Clamp material ID to valid range
                if (mat_id < 0 || mat_id >= static_cast<int>(materials_.size())) {
                    mat_id = 0;
                }
                
                if (mat_id != current_mat_id) {
                    // Save previous submesh if it has content
                    if (current_submesh.index_count > 0) {
                        submeshes_.push_back(current_submesh);
                    }
                    
                    // Start new submesh
                    current_submesh.index_start = index_offset;
                    current_submesh.index_count = 0;
                    current_submesh.material_index = mat_id;
                    current_mat_id = mat_id;
                }
                
                // Each face is a triangle (3 indices)
                current_submesh.index_count += 3;
                index_offset += 3;
            }
            
            // Don't forget the last submesh
            if (current_submesh.index_count > 0) {
                submeshes_.push_back(current_submesh);
            }
        }
    }
    
    grassland::LogInfo("Created {} submeshes from OBJ file", submeshes_.size());
    
    // Debug: Print each submesh
    for (size_t i = 0; i < submeshes_.size(); ++i) {
        const auto& sm = submeshes_[i];
        grassland::LogInfo("  SubMesh {}: start={}, count={}, mat_idx={}, triangles={}", 
                          i, sm.index_start, sm.index_count, sm.material_index, sm.index_count/3);
    }
    
    // If no submeshes created, make a default one
    if (submeshes_.empty()) {
        submeshes_.push_back(SubMesh(0, mesh_.NumIndices(), 0));
    }
    
    mesh_loaded_ = true;
    return true;
}

void Entity::BuildBLAS(grassland::graphics::Core* core) {
    if (!mesh_loaded_) {
        grassland::LogError("Cannot build BLAS: mesh not loaded");
        return;
    }

    // Create vertex buffer
    size_t vertex_buffer_size = mesh_.NumVertices() * sizeof(glm::vec3);
    core->CreateBuffer(vertex_buffer_size, 
                      grassland::graphics::BUFFER_TYPE_DYNAMIC, 
                      &vertex_buffer_);
    vertex_buffer_->UploadData(mesh_.Positions(), vertex_buffer_size);

    // Create index buffer
    size_t index_buffer_size = mesh_.NumIndices() * sizeof(uint32_t);
    core->CreateBuffer(index_buffer_size, 
                      grassland::graphics::BUFFER_TYPE_DYNAMIC, 
                      &index_buffer_);
    index_buffer_->UploadData(mesh_.Indices(), index_buffer_size);

    // Create UV buffer (if mesh has UVs)
    if (mesh_.TexCoords() && mesh_.NumVertices() > 0) {
        size_t uv_buffer_size = mesh_.NumVertices() * sizeof(glm::vec2);
        core->CreateBuffer(uv_buffer_size, 
                          grassland::graphics::BUFFER_TYPE_DYNAMIC, 
                          &uv_buffer_);
        uv_buffer_->UploadData(mesh_.TexCoords(), uv_buffer_size);
        grassland::LogInfo("Created UV buffer with {} coordinates", mesh_.NumVertices());
    } else {
        // Create default UV buffer (all zeros)
        std::vector<glm::vec2> default_uvs(mesh_.NumVertices(), glm::vec2(0.0f, 0.0f));
        size_t uv_buffer_size = default_uvs.size() * sizeof(glm::vec2);
        core->CreateBuffer(uv_buffer_size, 
                          grassland::graphics::BUFFER_TYPE_DYNAMIC, 
                          &uv_buffer_);
        uv_buffer_->UploadData(default_uvs.data(), uv_buffer_size);
        grassland::LogInfo("Created default UV buffer (no UVs in mesh)");
    }

    // Build material ID buffer from submesh definitions
    BuildMaterialIdBuffer();
    
    // Create material ID buffer (per triangle)
    size_t material_id_buffer_size = material_ids_.size() * sizeof(uint32_t);
    core->CreateBuffer(material_id_buffer_size,
                      grassland::graphics::BUFFER_TYPE_DYNAMIC,
                      &material_id_buffer_);
    material_id_buffer_->UploadData(material_ids_.data(), material_id_buffer_size);
    grassland::LogInfo("Created material ID buffer with {} triangle IDs from {} submeshes", 
                       material_ids_.size(), submeshes_.size());

    // Build BLAS
    core->CreateBottomLevelAccelerationStructure(
        vertex_buffer_.get(), 
        index_buffer_.get(), 
        sizeof(glm::vec3), 
        &blas_);

    grassland::LogInfo("Built BLAS for entity with {} materials", materials_.size());
}

void Entity::SetTransform(const glm::mat4& transform) {
    prev_transform_ = transform_;  // 保存上一帧的变换
    transform_ = transform;
}

void Entity::SetPreviousTransform(const glm::mat4& prev_transform) {
    prev_transform_ = prev_transform;
}

void Entity::SetTransformNoPrev(const glm::mat4& transform) {
    transform_ = transform;
}

