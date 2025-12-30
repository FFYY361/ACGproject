#ifndef COMMON_HLSLI
#define COMMON_HLSLI

// static const float PI = 3.14159265359f;
// static const float EPS = 1e-5f;
#include "random.hlsli"

// 定义光源类型
#define LIGHT_TYPE_POINT 0
#define LIGHT_TYPE_AREA  1

// ==================== 结构体定义 ====================
struct CameraInfo
{
    float4x4 screen_to_camera;
    float4x4 camera_to_world;
    float4x4 camera_to_world_prev; // previous frame/camera transform for motion blur
    float shutterOpen; // shutter open time (e.g., 0.0)
    float shutterClose; // shutter close time (e.g., 0.02)
    float aperture; // lens radius (world units)
    float focusDist; // focus distance for thin-lens model
};

struct HoverInfo
{
    int hovered_entity_id;
};

struct LightInfo
{
    int num_light;
};

struct Material
{
    float3 base_color;
    float3 emission;
    float3 alpha;
    float roughness;
    float metallic;
    float transmission; // 0 = opaque, 1 = fully transparent
    float3 ior; // Index of refraction for R/G/B (色散)
    int texture_id; // -1 = no texture, >= 0 = texture index
    int normal_id;
    int use_vertex_color; // 0 = use base_color/texture, 1 = use vertex colors
    int use_toon; // 0 = PBR shading, 1 = Toon/Cel shading
    
    // Volume properties
    float3 volume_emission; // Volumetric emission (e.g., glowing fog)
    float3 volume_absorption; // Absorption coefficient (sigma_a)
    float3 volume_scattering; // Scattering coefficient (sigma_s)
    float volume_density; // Density multiplier
    float volume_anisotropy; // Phase function anisotropy parameter g [-1,1]
};

// Volume rendering helper: compute transmittance
float3 VolumeTransmittance(float3 sigma_t, float distance)
{
    return exp(-sigma_t * distance);
}

// Henyey-Greenstein Phase Function
// g: anisotropy parameter [-1, 1]
//    g = 0: isotropic scattering (uniform in all directions)
//    g > 0: forward scattering (more likely to continue in same direction)
//    g < 0: backward scattering (more likely to scatter backwards)
// cosTheta: dot(wi, wo) where wi is incoming direction, wo is outgoing direction
float HenyeyGreensteinPhase(float g, float cosTheta)
{
    float denom = 1.0 + g * g - 2.0 * g * cosTheta;
    return (1.0 / (4.0 * PI)) * (1.0 - g * g) / (denom * sqrt(denom));
}

// Sample direction from Henyey-Greenstein phase function
// Returns sampled direction in world space
// wi: incident direction (normalized)
// g: anisotropy parameter
// u1, u2: random numbers in [0,1]
float3 SampleHenyeyGreenstein(float3 wi, float g, float u1, float u2)
{
    float cosTheta;
    
    if (abs(g) < 0.001) {
        // Isotropic case
        cosTheta = 1.0 - 2.0 * u1;
    } else {
        // Anisotropic case
        float sqr = (1.0 - g * g) / (1.0 - g + 2.0 * g * u1);
        cosTheta = (1.0 + g * g - sqr * sqr) / (2.0 * g);
    }
    
    float sinTheta = sqrt(max(0.0, 1.0 - cosTheta * cosTheta));
    float phi = 2.0 * PI * u2;
    
    // Build coordinate system around wi
    float3 w = wi;
    float3 up = abs(w.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 u = normalize(cross(up, w));
    float3 v = cross(w, u);
    
    // Sample direction in local coordinates, then transform to world
    return sinTheta * cos(phi) * u + sinTheta * sin(phi) * v + cosTheta * w;
}

struct EntityInfo
{
    float4x4 objectToWorld;
    float4x4 worldToObject;
    float4x4 objectToWorldPrev;
    float4x4 worldToObjectPrev;
    uint vertexBufferOffset;
    uint indexBufferOffset;
    uint materialOffset;
    uint materialIdBufferOffset;  // Offset into the material ID buffer
    uint numMaterials;             // Number of materials for this entity
};

struct Vertex
{
    float3 pos;
    float3 normal;
    float2 uv;  // Texture coordinates
};

struct TextureInfo
{
    int idx;
    int mipLevels;
};

// 重构后的 RayPayload (如上一条建议所述)
struct RayPayload
{
    bool hit; // 是否击中
    bool isShadowRay; // 是否为阴影射线
    float3 shadow; // 阴影衰减系数，若-1表示这不是阴影射线
    bool cal_emission; // 是否计算自发光
    uint instance_id; // 击中的物体ID
    
    // G-Buffer 数据 (将由 ClosestHit 填充)
    float3 position; // 世界坐标位置
    float3 normal; // 世界坐标法线
    Material material; // 击中点的材质信息
    float hit_distance; // Distance to hit point (for volume rendering)
    float3 debug;
    float angle; // 光锥角度
    float width; // 光锥宽度
    float3 dPdX;
    float3 dPdY;
    float3 dDdX;
    float3 dDdY;
    float time; // ray time for motion blur
    int wavelength_channel; // 0=R, 1=G, 2=B for dispersion, -1=not set
};

struct Light
{
    float3 position; // Point light position or Area light center
    float3 color; // Light intensity/color (e.g., float3(10, 10, 10))
    
    // Area light specifics (例如是一个矩形或者圆盘)
    float3 u; // Area light edge vector U
    float3 v; // Area light edge vector V
    float area; // Surface area
    uint type; // 0: Point, 1: Area
};

// ==================== 全局资源绑定 ====================
// 把寄存器绑定放在这里，所有 shader 都能看到
RaytracingAccelerationStructure as : register(t0, space0);
RWTexture2D<float4> output : register(u0, space1);
ConstantBuffer<CameraInfo> camera_info : register(b0, space2);
StructuredBuffer<Material> materials : register(t0, space3);
ConstantBuffer<HoverInfo> hover_info : register(b0, space4);
RWTexture2D<int> entity_id_output : register(u0, space5);
RWTexture2D<float4> accumulated_color : register(u0, space6);
RWTexture2D<int> accumulated_samples : register(u0, space7);
StructuredBuffer<EntityInfo> entity_infos : register(t0, space8);
StructuredBuffer<Vertex> vertices : register(t0, space9);
StructuredBuffer<uint> indices : register(t0, space10);
ConstantBuffer<LightInfo> lightinfo : register(b0, space11);
StructuredBuffer<Light> lights : register(t0, space12);
Texture2D textures[] : register(t0, space13);  // Bindless texture array
SamplerState textureSampler : register(s0, space14);
StructuredBuffer<TextureInfo> texture_infos : register(t0, space15);
StructuredBuffer<uint> material_ids : register(t0, space16);  // Per-triangle material IDs



Texture2D textures_real[];



float PowerHeuristic(float pdf_f, float pdf_g)
{
    float f2 = pdf_f * pdf_f;
    float g2 = pdf_g * pdf_g;
    return f2 / (max(f2 + g2, 0.00001));
}

#endif
