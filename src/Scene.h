#pragma once
#include "long_march.h"
#include "Entity.h"
#include "Material.h"
#include <vector>
#include <memory>

struct EntityInfo {
    glm::mat4 objectToWorld;
    glm::mat4 worldToObject;
    uint32_t vertexBufferOffset;
    uint32_t indexBufferOffset;
    uint32_t materialOffset;
};

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
};

// Scene manages a collection of entities and builds the TLAS
class Scene {
public:
    Scene(grassland::graphics::Core* core);
    ~Scene();

    // Add an entity to the scene
    void AddEntity(std::shared_ptr<Entity> entity);

    // Remove all entities
    void Clear();

    // Build/rebuild the TLAS from all entities
    void BuildAccelerationStructures();

    // Update TLAS instances (e.g., for animation)
    void UpdateInstances();

    // Get the TLAS for rendering
    grassland::graphics::AccelerationStructure* GetTLAS() const { return tlas_.get(); }

    // Get buffer for all entities
    grassland::graphics::Buffer* GetMaterialsBuffer() const { return materials_buffer_.get(); }
	grassland::graphics::Buffer* GetVerticesBuffer() const { return vertices_buffer_.get(); }
	grassland::graphics::Buffer* GetIndicesBuffer() const { return indices_buffer_.get(); }
	grassland::graphics::Buffer* GetEntityInfoBuffer() const { return entity_info_buffer_.get(); }

    // Get all entities
    const std::vector<std::shared_ptr<Entity>>& GetEntities() const { return entities_; }

    // Get number of entities
    size_t GetEntityCount() const { return entities_.size(); }

private:
    //void UpdateMaterialsBuffer();
	void UpdateBuffers(); // Update all buffers, including materials, vertex, index, entity info

    grassland::graphics::Core* core_;
    std::vector<std::shared_ptr<Entity>> entities_;
    std::unique_ptr<grassland::graphics::AccelerationStructure> tlas_;
    std::unique_ptr<grassland::graphics::Buffer> materials_buffer_;
    std::unique_ptr<grassland::graphics::Buffer> vertices_buffer_;
    std::unique_ptr<grassland::graphics::Buffer> indices_buffer_;
    std::unique_ptr<grassland::graphics::Buffer> entity_info_buffer_;

};

