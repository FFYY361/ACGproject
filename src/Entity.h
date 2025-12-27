#pragma once
#include "long_march.h"
#include "Material.h"

// SubMesh represents a portion of mesh with a specific material
struct SubMesh {
    uint32_t index_start;    // Starting index in the index buffer
    uint32_t index_count;    // Number of indices for this submesh
    uint32_t material_index; // Index into the entity's material array
    
    SubMesh() : index_start(0), index_count(0), material_index(0) {}
    SubMesh(uint32_t start, uint32_t count, uint32_t mat_idx)
        : index_start(start), index_count(count), material_index(mat_idx) {}
};

// Entity represents a mesh instance with a material and transform
class Entity {
public:
    Entity(const std::string& obj_file_path, 
           const Material& material = Material(),
           const glm::mat4& transform = glm::mat4(1.0f));
    
    // Constructor for loading OBJ with MTL materials
    Entity(const std::string& obj_file_path,
           const glm::mat4& transform,
           bool load_materials_from_mtl);

    ~Entity();

    // Load mesh from OBJ file
    bool LoadMesh(const std::string& obj_file_path);
    
    // Load mesh and materials from OBJ + MTL files
    bool LoadMeshWithMaterials(const std::string& obj_file_path);

    // Getters
    grassland::graphics::Buffer* GetVertexBuffer() const { return vertex_buffer_.get(); }
    grassland::graphics::Buffer* GetIndexBuffer() const { return index_buffer_.get(); }
    grassland::graphics::Buffer* GetUVBuffer() const { return uv_buffer_.get(); }
    grassland::graphics::Buffer* GetMaterialIdBuffer() const { return material_id_buffer_.get(); }
    const Material& GetMaterial() const { return material_; }  // Returns first material for backward compatibility
    const std::vector<Material>& GetMaterials() const { return materials_; }
    const std::vector<SubMesh>& GetSubMeshes() const { return submeshes_; }
    const glm::mat4& GetTransform() const { return transform_; }
    grassland::graphics::AccelerationStructure* GetBLAS() const { return blas_.get(); }
    bool HasMultipleMaterials() const { return materials_.size() > 1; }
    size_t GetTriangleCount() const { return mesh_.NumIndices() / 3; }

	grassland::Mesh<float>& GetMesh() { return mesh_; }

    // Setters
    void SetMaterial(const Material& material) { material_ = material; }
    void SetTransform(const glm::mat4& transform) { transform_ = transform; }

    // Create BLAS for this entity's mesh
    void BuildBLAS(grassland::graphics::Core* core);

    // Check if mesh is loaded
    bool IsValid() const { return mesh_loaded_; }

private:
    void BuildMaterialIdBuffer();  // Build per-triangle material IDs from submeshes
    
    grassland::Mesh<float> mesh_;
    Material material_;  // Primary material for backward compatibility
    std::vector<Material> materials_;  // All materials (for multi-material meshes)
    std::vector<SubMesh> submeshes_;  // Submesh definitions
    std::vector<uint32_t> material_ids_;  // Material ID per triangle (generated from submeshes)
    glm::mat4 transform_;

    std::unique_ptr<grassland::graphics::Buffer> vertex_buffer_;
    std::unique_ptr<grassland::graphics::Buffer> index_buffer_;
    std::unique_ptr<grassland::graphics::Buffer> uv_buffer_;
    std::unique_ptr<grassland::graphics::Buffer> material_id_buffer_;  // Per-triangle material IDs
    std::unique_ptr<grassland::graphics::AccelerationStructure> blas_;

    bool mesh_loaded_;
};

