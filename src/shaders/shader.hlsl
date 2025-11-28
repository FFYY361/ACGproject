
struct CameraInfo {
  float4x4 screen_to_camera;
  float4x4 camera_to_world;
};

struct HoverInfo {
  int hovered_entity_id;
};

struct Material
{
    float3 base_color;
    float roughness;
    float metallic;
};

struct EntityInfo
{
    float4x4 objectToWorld;
    float4x4 worldToObject;
    uint vertexBufferOffset; // 指向全局 array 中的第几个顶点 buffer
    uint indexBufferOffset; // 指向全局 array 中的第几个索引 buffer
    uint materialOffset;
};

struct Vertex
{
    float3 pos;
    float3 normal;
};

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

struct RayPayload {
    float3 color;
    bool hit;
    uint instance_id;
};

[shader("raygeneration")] void RayGenMain() {
    float2 pixel_center = (float2)DispatchRaysIndex() + float2(0.5, 0.5);
    float2 uv = pixel_center / float2(DispatchRaysDimensions().xy);
    uv.y = 1.0 - uv.y;
    float2 d = uv * 2.0 - 1.0;
    float4 origin = mul(camera_info.camera_to_world, float4(0, 0, 0, 1));
    float4 target = mul(camera_info.screen_to_camera, float4(d, 1, 1));
    float4 direction = mul(camera_info.camera_to_world, float4(target.xyz, 0));

    float t_min = 0.001;
    float t_max = 10000.0;

    RayPayload payload;
    payload.color = float3(0, 0, 0);
    payload.hit = false;
    payload.instance_id = 0;

    RayDesc ray;
    ray.Origin = origin.xyz;
    ray.Direction = normalize(direction.xyz);
    ray.TMin = t_min;
    ray.TMax = t_max;

    TraceRay(as, RAY_FLAG_NONE, 0xFF, 0, 1, 0, ray, payload);

    uint2 pixel_coords = DispatchRaysIndex().xy;
  
    // Write to immediate output (for camera movement mode)
    output[pixel_coords] = float4(payload.color, 1);
  
    // Write entity ID to the ID buffer
    // If no hit, write -1; otherwise write the instance ID
    entity_id_output[pixel_coords] = payload.hit ? (int)payload.instance_id : -1;
  
    // Accumulate color for progressive rendering (when camera is stationary)
    float4 prev_color = accumulated_color[pixel_coords];
    int prev_samples = accumulated_samples[pixel_coords];
  
    accumulated_color[pixel_coords] = prev_color + float4(payload.color, 1);
    accumulated_samples[pixel_coords] = prev_samples + 1;
}

[shader("miss")] void MissMain(inout RayPayload payload) {
    // Sky gradient
    //  float t = 0.5 * (normalize(WorldRayDirection()).y + 1.0);
    //  payload.color = lerp(float3(1.0, 1.0, 1.0), float3(0.5, 0.7, 1.0), t);
  
  
    float3 topColor = float3(1.0, 1.0, 1.0);
    float3 bottomColor = float3(1.0, 1.0, 1.0);
    float t = 0.5 * (normalize(WorldRayDirection()).y + 1.0);
    payload.color = lerp(topColor, bottomColor, t);
  
  
    payload.hit = false;
    payload.instance_id = 0xFFFFFFFF; // Invalid ID for miss
    }
    /*
    [shader("closesthit")] void ClosestHitMain(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr) {
    payload.hit = true;
  
    // Get material index from instance
    uint material_idx = InstanceID();
    payload.instance_id = material_idx;
  
    // Load material
    Material mat = materials[material_idx];
  
    // Simple diffuse lighting
    float3 world_normal = normalize(float3(0, 1, 0)); // Placeholder, should compute from geometry
    float3 light_dir = normalize(float3(1, 1, 1));
    float ndotl = max(0.0, dot(world_normal, light_dir));
  
    // Apply material color (NO hover highlighting here - done in post-process)
    float3 diffuse = mat.base_color;
  
    payload.color = diffuse;
}
*/

[shader("closesthit")]
void ClosestHitMain(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    payload.hit = true;

    // 获取实例/材质索引
    uint instance_idx = InstanceID();
    payload.instance_id = instance_idx;
  
    // barycentrics: attr.barycentrics 是 float2 (u,v)，第三分量为 1-u-v
    float2 bary2 = attr.barycentrics;
    float3 b = float3(1.0 - bary2.x - bary2.y, bary2.x, bary2.y);
  
    // 取得三角形索引和顶点位置
    uint prim = PrimitiveIndex();
    //payload.instance_id = prim;
  
    //printf("Hit triangle index: %d", prim, "\n");

    // 加载材质
    Material mat = materials[instance_idx];

    // ---------- 强制漫反射计算 ----------
    // 1. 获取世界空间法线
    float3 world_normal = normalize(float3(0, 0, 0)); // TODO: 替换为真实三角形法线
    
    
    EntityInfo entity_info = entity_infos[instance_idx];
    Vertex v0 = vertices[indices[entity_info.indexBufferOffset + prim * 3 + 0] + entity_info.vertexBufferOffset];
    Vertex v1 = vertices[indices[entity_info.indexBufferOffset + prim * 3 + 1] + entity_info.vertexBufferOffset];
    Vertex v2 = vertices[indices[entity_info.indexBufferOffset + prim * 3 + 2] + entity_info.vertexBufferOffset];
    
    if ((v0.normal.x == 0 && v0.normal.y == 0 && v0.normal.z == 0) ||
        (v1.normal.x == 0 && v1.normal.y == 0 && v1.normal.z == 0) ||
        (v2.normal.x == 0 && v2.normal.y == 0 && v2.normal.z == 0)) {
        // 如果顶点法线全为零，则使用面法线
        float3 edge1 = v1.pos - v0.pos;
        float3 edge2 = v2.pos - v0.pos;
        float3 face_normal = cross(edge1, edge2);
        world_normal = normalize(mul((float3x3) entity_info.objectToWorld, face_normal));
    } else {
        // 使用插值后的顶点法线
        float3 face_nromal = normalize(b.x * v0.normal + b.y * v1.normal + b.z * v2.normal);
        world_normal = normalize(mul((float3x3) entity_info.objectToWorld, face_nromal));
    }
  

    // 2. 平行光方向 (0, 0, -1)
    float3 light_dir = normalize(float3(0, -1, 0));

    // 3. 计算 Lambert 漫反射系数 N·L
    float ndotl = max(dot(world_normal, -light_dir), 0.0);
    // 注意这里取负号，因为光照方向是从光源指向表面

    // 4. 乘以材质基色得到最终漫反射颜色
    float3 diffuse = mat.base_color * ndotl;
    //float3 diffuse = world_normal;

    // 输出到 RayPayload
    payload.color = diffuse;
}
