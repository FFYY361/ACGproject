#pragma once
#include "long_march.h"
#include "Entity.h"
#include "Material.h"
#include <vector>
#include <memory>

struct EntityInfo {
    glm::mat4 objectToWorld;
    glm::mat4 worldToObject;
    glm::mat4 objectToWorldPrev; // Added for previous frame transform
    glm::mat4 worldToObjectPrev;  // Added for previous frame transform
    uint32_t vertexBufferOffset;
    uint32_t indexBufferOffset;
    uint32_t materialOffset;
    uint32_t materialIdBufferOffset;  // Offset into the material ID buffer
    uint32_t numMaterials;             // Number of materials for this entity
};

struct TextureInfo {
    int idx;
	int mipLevels;
    TextureInfo(int idx, int mipLevels) : idx(idx), mipLevels(mipLevels) {};
};

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
};

struct Light
{
    glm::vec3 position; // Point light position or Area light center
    glm::vec3 color; // Light intensity/color (e.g., float3(10, 10, 10))

    // Area light specifics
    glm::vec3 u; // Area light edge vector U
    glm::vec3 v; // Area light edge vector V
    float area; // Surface area
    uint32_t type; // 0: Point, 1: Area

	Light() : type(0), position(0.0f), color(1.0f), u(0.0f), v(0.0f), area(0.0f) {}
    Light(bool type, const glm::vec3& position, const glm::vec3& color,
          const glm::vec3& u = glm::vec3(0.0f), const glm::vec3& v = glm::vec3(0.0f), float area = 0.0f)
		: type(type), position(position), color(color), u(u), v(v), area(area) {
	}
};

struct LightInfo
{
    int num_light;
    
	LightInfo() : num_light(0) {}
};

// Scene manages a collection of entities and builds the TLAS
class Scene {
public:
    Scene(grassland::graphics::Core* core);
    ~Scene();

    // Add an entity to the scene
    void AddEntity(std::shared_ptr<Entity> entity);
    void AddLight(std::shared_ptr<Light> light);

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
	grassland::graphics::Buffer* GetLightsBuffer() const { return lights_buffer_.get(); }
	grassland::graphics::Buffer* GetLightInfoBuffer() const { return light_info_buffer_.get(); }
	grassland::graphics::Buffer* GetTextureInfoBuffer() const { return texture_info_buffer_.get(); }
	grassland::graphics::Buffer* GetMaterialIdBuffer() const { return material_id_buffer_.get(); }

    // Get all entities
    const std::vector<std::shared_ptr<Entity>>& GetEntities() const { return entities_; }

    // Get number of entities
    size_t GetEntityCount() const { return entities_.size(); }

private:
    //void UpdateMaterialsBuffer();
	void UpdateBuffers(); // Update all buffers, including materials, vertex, index, entity info

    grassland::graphics::Core* core_;
    std::vector<std::shared_ptr<Entity>> entities_;
	std::vector<Light> lights_;
	LightInfo light_info_;
    std::unique_ptr<grassland::graphics::AccelerationStructure> tlas_;
    std::unique_ptr<grassland::graphics::Buffer> materials_buffer_;
    std::unique_ptr<grassland::graphics::Buffer> vertices_buffer_;
    std::unique_ptr<grassland::graphics::Buffer> indices_buffer_;
    std::unique_ptr<grassland::graphics::Buffer> entity_info_buffer_;
	std::unique_ptr<grassland::graphics::Buffer> light_info_buffer_;
	std::unique_ptr<grassland::graphics::Buffer> lights_buffer_;
	std::unique_ptr<grassland::graphics::Buffer> material_id_buffer_;  // Per-triangle material IDs
    
    // Texture management
    std::vector<std::unique_ptr<grassland::graphics::Image>> textures_;
    std::unique_ptr<grassland::graphics::Sampler> texture_sampler_;
    int num_texture_;
    std::vector<TextureInfo> texture_infos_;
    std::unique_ptr<grassland::graphics::Buffer> texture_info_buffer_;

public:
    // Texture management
    int AddTexture(const std::string& file_path);
    int CreateProceduralTexture(int width, int height, const std::function<glm::vec4(float, float)>& generator);
    grassland::graphics::Image* GetTexture(int index) const;
    size_t GetTextureCount() const { return textures_.size(); }
    grassland::graphics::Sampler* GetTextureSampler() const { return texture_sampler_.get(); }

private:
};

