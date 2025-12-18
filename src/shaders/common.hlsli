#ifndef COMMON_HLSLI
#define COMMON_HLSLI

// 定义光源类型
#define LIGHT_TYPE_POINT 0
#define LIGHT_TYPE_AREA  1

// ==================== 结构体定义 ====================
struct CameraInfo
{
    float4x4 screen_to_camera;
    float4x4 camera_to_world;
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
    float roughness;
    float metallic;
    float transmission;  // 0 = opaque, 1 = fully transparent
    float3 emission; // 后面做光源时会用到
    float ior;           // Index of refraction
    int texture_id;      // -1 = no texture, >= 0 = texture array index
    int material_id;
};

struct EntityInfo
{
    float4x4 objectToWorld;
    float4x4 worldToObject;
    uint vertexBufferOffset;
    uint indexBufferOffset;
    uint materialOffset;
};

struct Vertex
{
    float3 pos;
    float3 normal;
    float2 uv;  // Texture coordinates
};

// 重构后的 RayPayload (如上一条建议所述)
struct RayPayload
{
    bool hit; // 是否击中
    bool cal_emission; // 是否计算自发光
    uint instance_id; // 击中的物体ID
    
    // G-Buffer 数据 (将由 ClosestHit 填充)
    float3 position; // 世界坐标位置
    float3 normal; // 世界坐标法线
    Material material; // 击中点的材质信息
    float3 debug;
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



float PowerHeuristic(float pdf_f, float pdf_g)
{
    float f2 = pdf_f * pdf_f;
    float g2 = pdf_g * pdf_g;
    return f2 / (max(f2 + g2, 0.00001));
}

#endif
