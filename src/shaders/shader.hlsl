
// 1. 引入其他模块
#include "common.hlsli"
#include "random.hlsli"
#include "material.hlsl"

void GetLightInfo(
    in Light light, in float3 P, in float3 light_pos,
    out float3 intensity, out float pdf_solid, out float3 dir, out float dist)
{
    if (light.type == 0)
    {
        // 点光源
        float3 toLight = light.position - P;
        dist = length(toLight);
        dir = normalize(toLight);
        intensity = light.color;
        float pdf = 1.0;
        pdf_solid = pdf * (dist * dist); // Dirac delta
    }
    else
    {
        // 面光源
        // 简单均匀采样
        float3 toLight = light_pos - P;
        dist = length(toLight);
        dir = normalize(toLight);
        float3 lightNormal = normalize(cross(light.u, light.v));
        float cosLight = max(dot(-normalize(toLight), lightNormal), 0.0);
        
        intensity = light.color;
        float pdf = 1.0 / light.area;
        pdf_solid = pdf * (dist * dist) / max(cosLight, 0.001);
    }
}






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
    

    float last_brdf_pdf = -1.0;
    float3 last_hit_pos = float3(0.0, 0.0, 0.0);
    
    RayPayload payload;
    payload.instance_id = -1; // 初始为无效 ID
    payload.cal_emission = true;
    
    // === 路径追踪主循环 ===
    // 限制最大反弹次数，玻璃球需要更多次数以支持多次折射
    for (int bounce = 0; bounce < 8; bounce++)
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
        
        float3 P = payload.position;
        float3 N = payload.normal;
        float3 V = -ray.Direction; // 视线方向
        
        // (可选) 累加自发光 emission
        // 如果击中的是非常强的光源，通常我们就停止路径追踪了，因为光线已经“找到家”了
        if (length(payload.emission) > 0.01)
        {
            // 我们需要计算 MIS 权重： w_b = p_b^2 / (p_b^2 + p_l^2)
            // p_b 是上一轮决定射向这里的 BSDF PDF (我们需要在上一轮循环结束前存下来)
            // p_l 是如果用 NEE 采样这个点，得到的 PDF (Solid Angle)
            
            Light light = lights[payload.instance_id]; // 光源必定先于entity创建，因此instance_id可用来索引lights数组
            float mis_indirect = 1.0f;
            float3 L_dir;
            float dist;
            float3 L_intensity;
            float light_pdf_solid = 0.0;
            
            /*if (light.type != 0)
            {
            }*/
            GetLightInfo(light, last_hit_pos, payload.position, L_intensity, light_pdf_solid, L_dir, dist);
            
            // 对于透射材质（last_brdf_pdf == 1.0），直接累加emission不需要MIS
            if (last_brdf_pdf == 1.0)
            {
                radiance += throughput * payload.emission;
            }
            else
            {
                // 标准BRDF路径需要MIS权重
                if (last_brdf_pdf < 0)
                    mis_indirect = 1.0f;
                else
                    mis_indirect = PowerHeuristic(last_brdf_pdf, light_pdf_solid);
            
                radiance += throughput * payload.emission * mis_indirect;
            }
            //radiance = float3(0, 0, 0); // 遇到自发光直接结束
            break;
        }
        //radiance = payload.metallic;
        
        float3 directLightContrib = float3(0, 0, 0);
        
        // 透射材质也需要直接光照（用于表面反射部分）
        // 遍历光源进行Next Event Estimation
        for (int i = 0; i < lightinfo.num_light; ++i)
        {
            Light light = lights[i];
            float3 L_dir;
            float dist;
            float3 L_intensity;
            float light_pdf_solid = 1.0;
            
            // --- 3.1 计算光源信息 ---
            float3 light_pos = light.position;
            if (light.type == 1)
            {
                // 面光源，随机采样光源表面位置
                float r1 = next_rand(seed);
                float r2 = next_rand(seed);
                light_pos = light.position + light.u * (r1 - 0.5) + light.v * (r2 - 0.5);
            }
            GetLightInfo(light, P, light_pos, L_intensity, light_pdf_solid, L_dir, dist);

            // --- 3.2 阴影射线 (Shadow Ray) ---
            RayDesc shadowRay;
            shadowRay.Origin = P + N * 0.001;
            shadowRay.Direction = L_dir;
            shadowRay.TMin = 0.001;
            shadowRay.TMax = dist - 0.1;
            RayPayload shadowPayload;
            shadowPayload.hit = true;
            TraceRay(as, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
                 0xFF, 0, 1, 0, shadowRay, shadowPayload);
            
            if (!shadowPayload.hit)
            {
                // --- 3.3 计算 BSDF 贡献 ---
                float NdotL = max(dot(N, L_dir), 0.0);
                float bsdf_pdf;
                float3 bsdf = EvalBSDF(N, V, L_dir, payload.albedo, payload.roughness, 
                                      payload.metallic, payload.transmission, payload.ior, bsdf_pdf);
                float mis_direct = 1.0;
                
                if (bsdf_pdf > 0.0001)
                    mis_direct = PowerHeuristic(light_pdf_solid, bsdf_pdf);
                
                directLightContrib += mis_direct * throughput * bsdf * L_intensity * NdotL / light_pdf_solid;
            }
        }
        radiance += directLightContrib;
        
        // === 使用统一的BSDF采样 ===
        BSDFSample bsdf_sample = SampleBSDF(N, V, payload.albedo, payload.roughness, 
                                            payload.metallic, payload.transmission, 
                                            payload.ior, seed);
        
        // 检查采样是否有效
        if (bsdf_sample.pdf < 0.0001 || length(bsdf_sample.weight) < 0.0001)
        {
            break; // 采样失败或能量为0
        }
        
        // 更新throughput
        throughput *= bsdf_sample.weight;
        
        // 更新光线
        float3 next_dir = normalize(bsdf_sample.direction);
        ray.Origin = P + next_dir * 0.001; // 沿着新方向偏移
        ray.Direction = next_dir;
        
        // 保存PDF用于MIS
        if (bsdf_sample.isTransmission)
        {
            last_brdf_pdf = 1.0; // 透射使用特殊标记
        }
        else
        {
            last_brdf_pdf = bsdf_sample.pdf;
        }
        last_hit_pos = P;

        // --- 俄罗斯轮盘赌 (Russian Roulette) ---
        // 随着反弹次数增加，throughput 会变小。如果太小，就随机终止，避免浪费计算。
        if (bounce > 2)
        {
            float p = max(throughput.x, max(throughput.y, throughput.z));
            if (next_rand(seed) > p)
                break; // 终止
            throughput /= p; // 能量补偿
        }
    } // 路径追踪主循环结束
    
    
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
    
    // 插值UV坐标
    float2 uv = b.x * v0.uv + b.y * v1.uv + b.z * v2.uv;
    
    // === 将数据写入 Payload ===
    payload.position = world_pos;
    payload.normal = world_normal; // 确保法线归一化
    
    // 如果有纹理，使用纹理颜色；否则使用材质基础色
    if (mat.texture_id >= 0) {
        payload.albedo = textures[NonUniformResourceIndex(mat.texture_id)].SampleLevel(textureSampler, uv, 0).rgb;
    } else {
        payload.albedo = mat.base_color;
    }
    
    payload.roughness = mat.roughness;
    payload.metallic = mat.metallic;
    payload.transmission = mat.transmission;
    payload.ior = mat.ior;
    // 始终传递自发光，无论光线来自何处
    payload.emission = mat.emission;
}
