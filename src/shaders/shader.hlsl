
// 1. 引入其他模块
#include "common.hlsli"
#include "random.hlsli"
#include "bsdf.hlsli"


void GetLightInfo(
    in Light light, in float3 P, in float3 light_pos,
    out float3 intensity, out float pdf_solid, out float3 dir, out float dist
)
{
    if (light.type == 0)
    {
        // 点光源
        float3 toLight = light.position - P;
        dist = length(toLight);
        dir = normalize(toLight);
        intensity = light.color;
        float pdf = PI;
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

float3 MipMapSample(int idx, float2 uv, float width, float3 ray_dir, float3 normal, float uv_scale)
{
    // 1. 获取基础信息
    TextureInfo info = texture_infos[idx];
    int text_idx = info.idx;
    Texture2D tex = textures[NonUniformResourceIndex(text_idx)];
    uint twidth, theight, numlevel;
    tex.GetDimensions(0, twidth, theight, numlevel);
    
    float resolution = (float) max(twidth, theight);

    // 2. 修正倾斜拉伸 (Anisotropy)
    // 当光线斜射表面时，足迹（Footprint）会变大
    // cos_theta 越小（越趋于掠射），footprint 越大，mip 层级越高（越模糊）
    float cos_theta = abs(dot(ray_dir, normal));
    float spread_width = width / max(cos_theta, 0.0001f);

    // 3. 计算 Mip Level
    // 注意：此处假设你的 width 是在世界空间定义的
    // 你需要一个 texels_per_unit (每单位世界坐标对应多少纹理像素)
    // 或者是简单的 uv_to_world 比例
    float uv_per_world_unit = uv_scale; // 这个值通常需要从顶点/材质数据中预计算
    float mip_level = log2(spread_width * uv_per_world_unit * resolution);
    
    //return mip_level;
    // 4. 边界处理
    mip_level = clamp(mip_level, 0.0, (float) (numlevel - 1));
    text_idx += (int) mip_level;
    tex = textures[NonUniformResourceIndex(text_idx)];
    //texture = textures[NonUniformResourceIndex(text_idx)];
    return tex.SampleLevel(textureSampler, uv, 0).rgb;
}

// float3 MipMapSample(int idx, float2 uv, float width, float3 ray_dir, float3 normal)
// {
//     int base_idx = texture_infos[idx].idx; // index of mip level 0 in textures_ array
//     // Query base level dimensions
//     uint twidth, theight, numlevel;
//     textures[NonUniformResourceIndex(base_idx)].GetDimensions(0, twidth, theight, numlevel);
//     uint resolution = max(twidth, theight);
//     float area_uv = 1.0; // 假设整个纹理覆盖面积为1
//     float area_pos = width * width; // 近似为正方形区域
//     // float area_uv = 1.0; // assume whole texture covers area 1
//     // float area_pos = max(width * width, 1e-6); // avoid div by zero
//     float texel_size = sqrt(area_uv / area_pos);
//     float mip_level = log2(max(1.0, (float)resolution * texel_size)) - log2(abs(dot(ray_dir, normal)) + EPS);
//     mip_level = clamp(mip_level, 0.0, (float) (texture_infos[idx].mipLevels - 1));
//     int mip = (int) floor(mip_level + 0.5);
//     int final_idx = base_idx + mip;
//     Texture2D texture = textures[NonUniformResourceIndex(final_idx)];
//     return texture.SampleLevel(textureSampler, uv, 0).rgb;
// }


float3 get_target_direction(float2 pixel_center)
{
    uint2 dims = float2(DispatchRaysDimensions().xy);
    float2 uv = pixel_center / float2(dims);
    uv.y = 1.0 - uv.y;
    uv = uv * 2.0 - 1.0;
    float4 target = mul(camera_info.screen_to_camera, float4(uv, 1, 1));
    return target.xyz;
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
    
    // 3. 采样光线时间（用于 motion blur）并插值相机变换
    float time = lerp(camera_info.shutterOpen, camera_info.shutterClose, next_rand(seed));
    float alpha = 0.0;
    if (abs(camera_info.shutterClose - camera_info.shutterOpen) > 1e-6)
        alpha = (time - camera_info.shutterOpen) / (camera_info.shutterClose - camera_info.shutterOpen);
    float4x4 cam_to_world_t = lerp(camera_info.camera_to_world_prev, camera_info.camera_to_world, alpha);

    // 4. 生成初始光线（使用插值后的相机变换）
    float3 cam_origin = mul(cam_to_world_t, float4(0, 0, 0, 1)).xyz;
    float4 target = mul(camera_info.screen_to_camera, float4(d, 1, 1));
    float3 pinhole_dir = normalize(mul(cam_to_world_t, float4(target.xyz, 0)).xyz);

    // Thin-lens DOF: sample lens if aperture > 0
    float aperture = camera_info.aperture;
    float focusDist = camera_info.focusDist;
    float3 cam_right = mul(cam_to_world_t, float4(1, 0, 0, 0)).xyz;
    float3 cam_up = mul(cam_to_world_t, float4(0, 1, 0, 0)).xyz;

    float3 origin;
    float3 direction_world;
    if (aperture > 1e-6) {
        // Focus point on focal plane
        float3 p_focus = cam_origin + pinhole_dir * focusDist;
        // Sample disk using concentric mapping
        float u1 = next_rand(seed);
        float u2 = next_rand(seed);
        float2 u = 2.0 * float2(u1, u2) - 1.0;
        float2 disk;
        if (u.x == 0 && u.y == 0) {
            disk = float2(0,0);
        } else {
            float2 d;
            float r, theta;
            if (abs(u.x) > abs(u.y)) {
                r = u.x;
                theta = (PI/4.0) * (u.y / u.x);
            } else {
                r = u.y;
                theta = (PI/2.0) - (PI/4.0) * (u.x / u.y);
            }
            d = r * float2(cos(theta), sin(theta));
            disk = d;
        }
        float3 lens_offset = cam_right * (disk.x * aperture) + cam_up * (disk.y * aperture);
        origin = cam_origin + lens_offset;
        direction_world = normalize(p_focus - origin);
    } else {
        origin = cam_origin;
        direction_world = pinhole_dir;
    }

    float t_min = 0.001;
    float t_max = 10000.0;
    
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = normalize(direction_world);
    ray.TMin = t_min;
    ray.TMax = t_max;
    
    float3 throughput = float3(1.0, 1.0, 1.0);
    float3 radiance = float3(0.0, 0.0, 0.0);
    

    float last_bsdf_pdf = -1.0;
    float3 last_hit_pos = float3(0.0, 0.0, 0.0);
    
    RayPayload payload;
    payload.instance_id = -1; // 初始为无效 ID
    payload.cal_emission = true;
    payload.time = time;
    
    // === 路径追踪主循环 ===
    // 限制最大反弹次数，玻璃球需要更多次数以支持多次折射
    for (int bounce = 0; bounce < 8; bounce++)
    {
        
        payload.hit = false;
        
        
        
        payload.dPdX = 0.0;
        payload.dPdY = 0.0;
        
        
        float3 d = target.xyz;
        float3 x = float3(1, 0, 0);
        float3 y = float3(0, 1, 0);
        
        float3 target_x = get_target_direction(pixel_center + float2(1.0, 0.0));
        float3 target_y = get_target_direction(pixel_center + float2(0.0, 1.0));
        float3 right = target_x - d;
        float3 up = target_y - d;
        payload.dDdX = (pow(length(d), 2) * right - (dot(d, right)) * right) / (pow(length(d), 3) + EPS);
        payload.dDdY = (pow(length(d), 2) * up - (dot(d, up)) * up) / (pow(length(d), 3) + EPS);
        
        payload.dDdX = mul(camera_info.camera_to_world, float4(payload.dDdX, 0)).xyz;
        payload.dDdY = mul(camera_info.camera_to_world, float4(payload.dDdY, 0)).xyz;
        
        float fovY = radians(60.0f);

        
        payload.angle = (2 * tan(fovY / 2.0f)) / dims.y; // 2 arctan(0.5/F)
        payload.width = 0.0;
        
        float h, w;
        uint n;
        
        // 发射光线
        payload.isShadowRay = false;
        TraceRay(as, RAY_FLAG_NONE, 0xFF, 0, 1, 0, ray, payload);
        
        if (bounce == 0)
            entity_id_output[pixel_coords] = payload.hit ? (int) payload.instance_id : -1;
        

        // --- Case 1: Miss (击中天空/背景) ---
        if (!payload.hit)
        {
            //radiance = 0.0;
            radiance += throughput * payload.material.base_color; // 如果 Miss Shader 里设置了 albedo
            break; // 光线逃逸，结束路径
        }

        // --- Case 2: Hit (击中物体) ---
        
        //radiance = payload.material.base_color;
        //break;
        float3 P = payload.position;
        float3 N = payload.normal;
        float3 V = -ray.Direction;
        
        // (可选) 累加自发光 emission
        // 如果击中的是非常强的光源，通常我们就停止路径追踪了，因为光线已经“找到家”了
        if (length(payload.material.emission) > 0.01)
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
                // 标准BRDF路径需要MIS权重
            if (last_bsdf_pdf < 0)
                mis_indirect = 1.0f;
            else
                mis_indirect = PowerHeuristic(last_bsdf_pdf, light_pdf_solid);
            
            //radiance = 0;
            radiance += mis_indirect * throughput * payload.material.emission;
            //radiance = float3(10, 0, 0);
            //radiance = float3(0, 0, 0); // 遇到自发光直接结束
            break;
        }
        // === Next Event Estimation (直接光照) ===
        float3 directLightContrib = float3(0.0, 0.0, 0.0);
        for (int i = 0; i < lightinfo.num_light; ++i)
        {
            Light light = lights[i];
            
            // 采样光源位置
            float3 light_pos = light.position;
            if (light.type == 1) // 面光源
            {
                light_pos += light.u * (next_rand(seed) - 0.5) + light.v * (next_rand(seed) - 0.5);
            }
            
            // 计算光源信息
            float3 L_dir, L_intensity;
            float dist, light_pdf_solid;
            GetLightInfo(light, P, light_pos, L_intensity, light_pdf_solid, L_dir, dist);

            // 阴影射线测试
            RayDesc shadowRay;
            shadowRay.Origin = P + L_dir * 0.001;
            shadowRay.Direction = L_dir;
            shadowRay.TMin = 0.001;
            shadowRay.TMax = dist - 0.1;
            RayPayload shadowPayload;
            shadowPayload.time = payload.time;
            shadowPayload.hit = true;
            shadowPayload.isShadowRay = true;
            shadowPayload.shadow = 1.0f; // 初始光强为1.0
            TraceRay(as, RAY_FLAG_NONE,
                 0xFF, 0, 1, 0, shadowRay, shadowPayload);
            
            if (length(shadowPayload.shadow) > 0.001)
            {
                // --- 3.3 计算 BSDF 贡献 ---
                float NdotL = dot(N, L_dir);
                
                // 检查光线和法线是否在同一半球（透射材质不能通过 NEE 直接照明）
                if (NdotL > 0.0)
                {
                    // 使用新接口评估 BSDF
                    float3 bsdf_value;
                    float bsdf_pdf;
                    EvalBSDFForNEE(V, L_dir, N, payload.material, bsdf_value, bsdf_pdf);
                    
                    float mis_direct = 1.0;
                    if (bsdf_pdf > 0.0001)
                        mis_direct = PowerHeuristic(light_pdf_solid, bsdf_pdf);
                    
                    directLightContrib += shadowPayload.shadow * mis_direct * throughput * bsdf_value * L_intensity * NdotL / light_pdf_solid;
                }
            }
        }
        //if (bounce == 1)
            //radiance += directLightContrib;
        //break;
        radiance += directLightContrib;
        //break;
        
        // === 使用统一的BSDF采样 ===
        BSDFSample bsdf_sample;
        float angle = payload.angle;
        float width = payload.width;
        bool success = SampleBSDF(V, N, payload.material, seed, bsdf_sample, payload);
        payload.angle = angle;
        payload.width = width;
        
        
        
        
        
        
        
        
        //radiance = bsdf_sample.bsdf; 
        //break;
        
        // 检查采样是否有效
        if (!success || bsdf_sample.pdf < 0.0001 || length(bsdf_sample.bsdf) < 0.0001)
        {
            
            break; // 采样失败或能量为0
        }
        /*
        if (bounce == 0)
        {
            radiance = bsdf_sample.bsdf / bsdf_sample.pdf;
            break;
        }
        */
        // 更新throughput
        throughput *= bsdf_sample.bsdf * abs(dot(N, bsdf_sample.direction)) / bsdf_sample.pdf;
        
        // 更新光线
        float3 next_dir = normalize(bsdf_sample.direction);
        ray.Origin = P + next_dir * 0.001; // 沿着新方向偏移
        ray.Direction = next_dir;
        
        // 保存PDF用于MIS
        last_bsdf_pdf = bsdf_sample.pdf;
        last_hit_pos = P;
        
        
        
        
        

        // --- 俄罗斯轮盘赌 (Russian Roulette) ---
        // 随着反弹次数增加，throughput 会变小。如果太小，就随机终止，避免浪费计算。
        if (bounce > 4)
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
  
    payload.hit = false;
    if (payload.isShadowRay)
        return; // 阴影射线不需要天空颜色
    
    
    float3 topColor = float3(0.2, 0.2, 0.6);
    float3 bottomColor = float3(0.1, 0.1, 0.4);
    float t = 0.5 * (normalize(WorldRayDirection()).y + 1.0);
    Material mat;
    mat.base_color = lerp(bottomColor, topColor, t);
    payload.material = mat;
  
  
    payload.instance_id = 0xFFFFFFFF; // Invalid ID for miss
}

[shader("anyhit")]
void AnyHitMain(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    // 1. 获取当前点的 Alpha 值（示例暂用硬编码，之后需采样纹理）
    // 假设 alpha = 0.0 是全透明，1.0 是全不透明
    uint instance_idx = InstanceID();
  
  
    // 取得三角形索引和顶点位置
    uint prim = PrimitiveIndex();
  
    // 获取entity info来查找正确的材质
    EntityInfo entity_info = entity_infos[instance_idx];
    
    // 计算该entity内部的三角形索引，然后加上materialIdBufferOffset得到全局索引
    uint triangle_global_idx = entity_info.materialIdBufferOffset + prim;
    uint material_id = material_ids[triangle_global_idx];
    
    // 加载该三角形的材质
    Material mat = materials[material_id];
    float3 alpha = mat.alpha;
    // 2. 针对阴影射线 (Shadow Ray)
    // 只有当你的渲染器逻辑里，用 payload.shadow 来标记剩余光强时才这样做
    if (payload.isShadowRay)
    {
        // 核心逻辑：既然是半透明，光线必须“穿过去”才能计算后面的遮挡
        payload.shadow *= (1.0f - alpha);

        if (length(payload.shadow) < 0.001f)
        {
            // 如果光强已经衰减到几乎没有了，接受这次碰撞（停止搜索）
            AcceptHitAndEndSearch();
            return;
        }
        else
        {
            // 还有残余光强，忽略这次碰撞，让光线继续飞向下一个物体
            IgnoreHit();
            return;
        }
    }
    else
    {
        // 3. 对于相机射线 (Primary Ray)
        // 如果碰到全透明的地方，必须 IgnoreHit，否则你会看到一个黑色的几何体表面
        if (max(alpha[0], max(alpha[1], alpha[2])) < 0.01f)
        {
            IgnoreHit();
            return;
        }
        else
        {
            //AcceptHitAndEndSearch();
            //IgnoreHit();
            return;
        }
    }
}

[shader("closesthit")] void ClosestHitMain(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    
    
    payload.hit = true;
    
    if (payload.isShadowRay)
    {
        // 阴影射线只需要标记为击中，然后在 AnyHit 里处理透明度
        return;
    }

    // 获取实例/材质索引
    uint instance_idx = InstanceID();
    payload.instance_id = instance_idx;
  
    // barycentrics: attr.barycentrics 是 float2 (u,v)，第三分量为 1-u-v
    float2 bary2 = attr.barycentrics;
    float3 b = float3(1.0 - bary2.x - bary2.y, bary2.x, bary2.y);
  
    // 取得三角形索引和顶点位置
    uint prim = PrimitiveIndex();
  
    // 获取entity info来查找正确的材质
    EntityInfo entity_info = entity_infos[instance_idx];
    // Interpolate instance transform based on ray time for motion blur
    float alpha_inst = 0.0;
    if (abs(camera_info.shutterClose - camera_info.shutterOpen) > 1e-6)
        alpha_inst = (payload.time - camera_info.shutterOpen) / (camera_info.shutterClose - camera_info.shutterOpen);
    alpha_inst = clamp(alpha_inst, 0.0, 1.0);
    float4x4 objectToWorld_t = lerp(entity_info.objectToWorldPrev, entity_info.objectToWorld, alpha_inst);
    float4x4 worldToObject_t = lerp(entity_info.worldToObjectPrev, entity_info.worldToObject, alpha_inst);
    
    // 计算该entity内部的三角形索引，然后加上materialIdBufferOffset得到全局索引
    uint triangle_global_idx = entity_info.materialIdBufferOffset + prim;
    uint material_id = material_ids[triangle_global_idx];
    
    // 加载该三角形的材质
    Material mat = materials[material_id];

    // ---------- 强制漫反射计算 ----------
    // 1. 获取世界空间法线
    float3 world_normal = normalize(float3(0, 0, 0)); // TODO: 替换为真实三角形法线
    
    Vertex v0 = vertices[indices[entity_info.indexBufferOffset + prim * 3 + 0] + entity_info.vertexBufferOffset];
    Vertex v1 = vertices[indices[entity_info.indexBufferOffset + prim * 3 + 1] + entity_info.vertexBufferOffset];
    Vertex v2 = vertices[indices[entity_info.indexBufferOffset + prim * 3 + 2] + entity_info.vertexBufferOffset];
    // 计算击中点的世界坐标 (用于下一条光线的起点)
    float3 world_pos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    
    // 插值UV坐标
    float2 uv = b.x * v0.uv + b.y * v1.uv + b.z * v2.uv;
    uv = float2(uv.x, 1.0 - uv.y); // V坐标翻转以匹配纹理采样约定)
    // 光线微分
    float sign = (dot(WorldRayDirection(), world_normal)) > 0 ? 1.0 : -1.0;
    float3 dTdX = -(dot(world_normal, (payload.dPdX + RayTCurrent() * payload.dDdX))) / (dot(WorldRayDirection(), world_normal) + sign * EPS);
    float3 dPdX = payload.dPdX + RayTCurrent() * payload.dDdX + dTdX * WorldRayDirection();
    float3 dDdX = payload.dDdX;
    
    float3 dTdY = -(dot(world_normal, (payload.dPdY + RayTCurrent() * payload.dDdY))) / (dot(WorldRayDirection(), world_normal) + sign * EPS);
    float3 dPdY = payload.dPdY + RayTCurrent() * payload.dDdY + dTdY * WorldRayDirection();
    float3 dDdY = payload.dDdY;
    
    payload.dPdX = dPdX;
    payload.dDdX = dDdX;
    payload.dPdY = dPdY;
    payload.dDdY = dDdY;
    
    payload.width = payload.width + tan(payload.angle) * RayTCurrent();
    
    /*
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
    }
    */
        // 如果顶点法线全为零，则使用面法线
    /*
    float3 edge1 = v1.pos - v0.pos;
    float3 edge2 = v2.pos - v0.pos;
    float3 face_normal = cross(edge1, edge2);
    world_normal = normalize(mul((float3x3) entity_info.objectToWorld, face_normal));
    */
    
    float3 face_normal_ = normalize(b.x * v0.normal + b.y * v1.normal + b.z * v2.normal);
    float3 world_normal_ = normalize(mul((float3x3) objectToWorld_t, face_normal_));
    
    // calc uv scale
    float area_uv = length(cross(float3(v1.uv - v0.uv, 0), float3(v2.uv - v0.uv, 0))) * 0.5;
    float area_3d = length(cross(v1.pos - v0.pos, v2.pos - v0.pos)) * 0.5;
    float uv_scale = sqrt(area_uv / area_3d);
    
    
    
    float3 face_normal = float3(0, 0, 0);
    if (mat.normal_id >= 0)
    {
        float3 normal = MipMapSample(mat.normal_id, uv, payload.width, WorldRayDirection(), world_normal_, uv_scale);
        normal = normal * 2.0 - 1.0; // 从 [0,1] 映射到 [-1,1]
        //normal.y = -normal.y; // 纹理空间到对象空间的 Y 轴翻转
        face_normal = normalize(normal);
    }
    else if (length(v0.normal) > 0.9 && length(v1.normal) > 0.9 && length(v2.normal) > 0.9)
    {
        // 使用插值后的顶点法线
        face_normal = normalize(b.x * v0.normal + b.y * v1.normal + b.z * v2.normal);
    }
    else
    {
        face_normal = normalize(cross(v1.pos - v0.pos, v2.pos - v0.pos));
    }
    
    world_normal = normalize(mul((float3x3) objectToWorld_t, face_normal));
    
    // 确保法线朝向光线入射侧（对于透射材质很重要）
    if (dot(world_normal, WorldRayDirection()) > 0.0)
    {
        world_normal = -world_normal;
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
    
    
    
    
    
    // === 将数据写入 Payload ===
    payload.position = world_pos;
    payload.normal = world_normal; // 确保法线归一化
    payload.debug = world_normal;
    // 如果有纹理，使用纹理颜色；否则使用材质基础色
    if (mat.texture_id >= 0) {
        mat.base_color = MipMapSample(mat.texture_id, uv, payload.width, WorldRayDirection(), world_normal, uv_scale);
    }
    
    payload.material = mat;
}
