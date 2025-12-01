
// 1. 引入其他模块
#include "common.hlsli"
#include "random.hlsli"

[shader("raygeneration")] void RayGenMain()
{
    
    // 1. 初始化随机数种子 (像素坐标 + 累计帧数)
    uint2 pixel_coords = DispatchRaysIndex().xy;
    uint2 dims = DispatchRaysDimensions().xy;
    uint seed = init_rand(pixel_coords.x + pixel_coords.y * dims.x, accumulated_samples[pixel_coords], 16);
    
    // 2. 也是 Anti-aliasing 的一步：在像素内抖动坐标
    float2 pixel_center = (float2) pixel_coords + float2(next_rand(seed), next_rand(seed));
    //float2 pixel_center = (float2) pixel_coords + float2(0.5, 0.5); // 不抖动版本
    float2 uv = pixel_center / float2(dims);
    uv.y = 1.0 - uv.y;
    float2 d = uv * 2.0 - 1.0;
    
    /*
    float2 pixel_center = (float2)DispatchRaysIndex() + float2(0.5, 0.5);
    float2 uv = pixel_center / float2(DispatchRaysDimensions().xy);
    uv.y = 1.0 - uv.y;
    float2 d = uv * 2.0 - 1.0;
    */
    
    // 3. 生成初始光线
    float4 origin = mul(camera_info.camera_to_world, float4(0, 0, 0, 1));
    float4 target = mul(camera_info.screen_to_camera, float4(d, 1, 1));
    float4 direction = mul(camera_info.camera_to_world, float4(target.xyz, 0));

    float t_min = 0.001;
    float t_max = 10000.0;
    
    RayDesc ray;
    ray.Origin = origin.xyz;
    ray.Direction = normalize(direction.xyz);
    ray.TMin = t_min;
    ray.TMax = t_max;
    
    float3 throughput = float3(1.0, 1.0, 1.0);
    float3 radiance = float3(0.0, 0.0, 0.0);
    
    RayPayload payload;
    payload.instance_id = -1; // 初始为无效 ID
    payload.cal_emission = true;
    
    // === 路径追踪主循环 ===
    // 限制最大反弹次数，例如 3 或 5
    for (int bounce = 0; bounce < 5; bounce++)
    {
        payload.hit = false;
        
        // 发射光线
        TraceRay(as, RAY_FLAG_NONE, 0xFF, 0, 1, 0, ray, payload);
        
        if (bounce == 0)
            entity_id_output[pixel_coords] = payload.hit ? (int) payload.instance_id : -1;

        // --- Case 1: Miss (击中天空/背景) ---
        if (!payload.hit)
        {
            radiance += throughput * payload.albedo; // 如果 Miss Shader 里设置了 albedo
            break; // 光线逃逸，结束路径
        }

        // --- Case 2: Hit (击中物体) ---
        
        // (可选) 累加自发光 emission
        radiance += throughput * payload.emission;
        // 如果击中的是非常强的光源，通常我们就停止路径追踪了，因为光线已经“找到家”了
        if (length(payload.emission) > 1.0)
        {
            break;
        }

        // 准备下一次反弹的数据
        float3 P = payload.position;
        float3 N = payload.normal;
        float3 V = -ray.Direction; // 视线方向
        
        float3 directLightContrib = float3(0, 0, 0);
        
        
        // 遍历光源 (这里为了演示只写一个循环，实际可能只随机采一个)
        for (int i = 0; i < lightinfo.num_light; ++i)
        {
            Light light = lights[i];
            float3 L_dir;
            float dist;
            float3 L_intensity;
            float light_pdf;

        // --- 3.1 光源采样 ---
            if (light.type == 0)
            {
                float3 toLight = light.position - P;
                dist = length(toLight);
                L_dir = normalize(toLight);
                L_intensity = light.color / (dist * dist);
                light_pdf = 1.0;
            }
            else
            { // AREA LIGHT
            // 简单的均匀采样
                float r1 = next_rand(seed);
                float r2 = next_rand(seed);
            // 假设 Light 是个平面 Quad，定义在 position, u, v
                float3 samplePos = light.position + light.u * (r1 - 0.5) + light.v * (r2 - 0.5);
                float3 toLight = samplePos - P;
                dist = length(toLight);
                L_dir = normalize(toLight);
            
            // 几何项 G = cos(theta_light) / dist^2
                float3 lightNormal = normalize(cross(light.u, light.v)); // 确保 Light 结构体里有这个或能计算
                float cosLight = max(dot(-L_dir, lightNormal), 0.0);
            
                L_intensity = light.color * cosLight / (dist * dist);
                light_pdf = 1.0 / light.area;
            }

        // --- 3.2 阴影射线 (Shadow Ray) ---
            RayDesc shadowRay;
            shadowRay.Origin = P + N * 0.001; // Bias
            shadowRay.Direction = L_dir;
            shadowRay.TMin = 0.001;
            shadowRay.TMax = dist - 0.1; // 防止打到光源自己

        // 使用一个新的 Payload 或者简单的 bool
            RayPayload shadowPayload;
            shadowPayload.hit = true; // 默认被遮挡
        
        // 关键 Flag: SKIP_CLOSEST_HIT_SHADER | ACCEPT_FIRST_HIT_AND_END_SEARCH
        // 这里的 instanceMask 和 hitGroupIndex 可能需要根据你的 Host 代码调整
        // 假设 Shadow Miss Shader 在 index 1 (需要在 Host 设置 Shader Table)
        // 如果没有单独的 Shadow Miss，就用通用的，并在 Miss Shader 里设置 hit = false
            TraceRay(as, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
                 0xFF, 0, 1, 0, shadowRay, shadowPayload); // Miss shader index 设为 0 (通用) 或 1 (专用)

        // 假设 Miss Shader 会把 shadowPayload.hit 设为 false
            if (!shadowPayload.hit)
            {
            // --- 3.3 计算 BRDF 贡献 ---
                float NdotL = max(dot(N, L_dir), 0.0);
            
            // 简单的 Diffuse BRDF = albedo / PI
            // Result = Throughput * BRDF * L_incoming * NdotL / PDF
            // 这里 throughput 是路径目前的衰减，不要更新 throughput，而是直接加到 radiance
            
                float3 brdf = payload.albedo / PI;
            
            // 注意：如果是金属(Metallic)，Diffuse 几乎为 0，这里简化处理
                brdf *= (1.0 - payload.metallic);
            
                directLightContrib += throughput * brdf * L_intensity * NdotL / light_pdf;
            }

        }
        radiance += directLightContrib;
        //radiance = float3(lightinfo.num_light, 0, 0); // 测试用，显示光源数量)
        float3 next_dir;
        
        // === 材质逻辑 (BSDF) ===
        // 这里根据 Roughness 和 Metallic 简单区分 Diffuse 和 Specular
        
        float probability_specular = payload.metallic; // 简单的混合概率
        // 如果想更物理，应该使用 Fresnel 计算 specular 概率
        
        if (next_rand(seed) < probability_specular)
        {
            // --- Specular (镜面反射) ---
            // 完美镜面反射: reflect(I, N). I 是入射光方向 (ray.Direction)
            // 如果要支持粗糙度，需要对反射方向进行 Importance Sampling (如 GGX)
            // 简单起见，这里先做完美反射：
            next_dir = reflect(ray.Direction, N);
            
            // 镜面反射颜色通常是 base_color (对于金属)
            throughput *= payload.albedo;
            payload.cal_emission = true; // 下一次计算自发光
        }
        else
        {
            // --- Diffuse (漫反射) ---
            // 使用余弦加权采样
            next_dir = GetCosineWeightedSample(N, seed);
            
            // Lambertian BRDF: Albedo / PI
            // PDF: cos(theta) / PI
            // Lighting equation term: cos(theta)
            // Throughput update = (Albedo / PI) * cos(theta) / (cos(theta) / PI) = Albedo
            throughput *= payload.albedo;
            payload.cal_emission = false; // 下一次不计算自发光
        }
        

        // --- 俄罗斯轮盘赌 (Russian Roulette) ---
        // 随着反弹次数增加，throughput 会变小。如果太小，就随机终止，避免浪费计算。
        if (bounce > 2)
        {
            float p = max(throughput.x, max(throughput.y, throughput.z));
            if (next_rand(seed) > p)
                break; // 终止
            throughput /= p; // 能量补偿
        }

        // --- 更新光线 ---
        ray.Origin = payload.position + N * 0.001; // 偏移起点防止自遮挡 (Shadow Acne)
        ray.Direction = next_dir;
    }
    
    
    
    /*
    TraceRay(as, RAY_FLAG_NONE, 0xFF, 0, 1, 0, ray, payload);

    // uint2 pixel_coords = DispatchRaysIndex().xy;
  
    // Write to immediate output (for camera movement mode)
    output[pixel_coords] = float4(payload.color, 1);
  
  
    // Accumulate color for progressive rendering (when camera is stationary)
    float4 prev_color = accumulated_color[pixel_coords];
    int prev_samples = accumulated_samples[pixel_coords];
  
    accumulated_color[pixel_coords] = prev_color + float4(payload.color, 1);
    accumulated_samples[pixel_coords] = prev_samples + 1;
    */
    
    
    // Write entity ID to the ID buffer
    // If no hit, write -1; otherwise write the instance ID
    
    // 如果没有 NaN (安全检查)
    if (any(isnan(radiance)))
        radiance = float3(0, 0, 0);

    output[pixel_coords] = float4(radiance, 1.0);

    // 累积逻辑
    float4 prev_color = accumulated_color[pixel_coords];
    int prev_samples = accumulated_samples[pixel_coords];
    
    // 如果相机移动了，通常 CPU 端会重置 accumulated_samples 为 0
    // 简单的平均算法： NewAverage = (OldSum + NewSample) / N
    if (prev_samples == 0)
        accumulated_color[pixel_coords] = float4(radiance, 1.0);
    else
        accumulated_color[pixel_coords] = prev_color + float4(radiance, 1.0);
    accumulated_samples[pixel_coords] = prev_samples + 1;
    
    // 写入 entity ID (仅针对第一次 Hit，需要单独处理或在 bounce=0 时写入)
    // 可以在 bounce loop 里加个 if (bounce == 0) entity_id_output[...] = ...
}

[shader("miss")] void MissMain(inout RayPayload payload) {
    // Sky gradient
    //  float t = 0.5 * (normalize(WorldRayDirection()).y + 1.0);
    //  payload.color = lerp(float3(1.0, 1.0, 1.0), float3(0.5, 0.7, 1.0), t);
  
    float3 topColor = float3(0.2, 0.2, 0.2);
    float3 bottomColor = float3(0.1, 0.1, 0.1);
    float t = 0.5 * (normalize(WorldRayDirection()).y + 1.0);
    payload.albedo = lerp(topColor, bottomColor, t);
  
  
    payload.hit = false;
    payload.instance_id = 0xFFFFFFFF; // Invalid ID for miss
}

[shader("closesthit")] void ClosestHitMain(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
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
    // payload.instance_id = prim;
  
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
  
    /*
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
    */
    
    // 计算击中点的世界坐标 (用于下一条光线的起点)
    float3 world_pos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    
    // === 将数据写入 Payload ===
    payload.position = world_pos;
    payload.normal = world_normal; // 确保法线归一化
    payload.albedo = mat.base_color;
    payload.roughness = mat.roughness;
    payload.metallic = mat.metallic;
    //payload.emission = float3(0, 0, 0); // 暂时设为0，除非你的材质结构体里有 emission
    if (payload.cal_emission)
        payload.emission = mat.emission;
}
