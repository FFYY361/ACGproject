#ifndef TOON_HLSLI
#define TOON_HLSLI

#include "common.hlsli"
#include "random.hlsli"

// ===============================================================================================
// 卡通渲染着色器 (Toon/Cel Shading)
// ===============================================================================================
// 这个文件提供简单易用的卡通风格着色函数
// 主要特点：
//   1. 离散化的光照（2-4个明暗级别）
//   2. 简化的高光（类似硬边缘）
//   3. 可调节的风格参数
// ===============================================================================================

// --- 卡通着色参数结构体 ---
struct ToonParameters
{
    int levels;              // 明暗级别数量（通常2-4）
    float specular_size;     // 高光大小（0.0-1.0）
    float specular_smoothness; // 高光边缘平滑度
    float rim_amount;        // 边缘光强度
    float rim_threshold;     // 边缘光阈值
};

// --- 默认卡通参数 ---
ToonParameters GetDefaultToonParams()
{
    ToonParameters params;
    params.levels = 3;                    // 3级明暗（暗部、中间、亮部）
    params.specular_size = 0.05;         // 小而清晰的高光
    params.specular_smoothness = 0.01;   // 硬边缘高光
    params.rim_amount = 0.3;             // 适中的边缘光
    params.rim_threshold = 0.7;          // 边缘光阈值
    return params;
}

// ===============================================================================================
// 1. 离散化函数 - 将连续值分成N个级别
// ===============================================================================================
float Posterize(float value, int levels)
{
    // 将 [0,1] 的连续值分成 levels 个离散级别
    // 例如 levels=3: 0-0.33->0.166, 0.33-0.66->0.5, 0.66-1.0->0.833
    if (levels <= 1)
        return value;
    
    float step = 1.0 / float(levels);
    float quantized = floor(value / step) * step + step * 0.5;
    return clamp(quantized, 0.0, 1.0);
}

// 平滑的离散化（带一点点抗锯齿效果）
float PosterizeSmooth(float value, int levels, float smoothness)
{
    float step = 1.0 / float(levels);
    float quantized = floor(value / step) * step + step * 0.5;
    
    // 在级别边界处添加平滑过渡
    float fraction = frac(value / step);
    float edge = smoothstep(0.5 - smoothness, 0.5 + smoothness, fraction);
    
    return lerp(quantized - step * 0.5, quantized + step * 0.5, edge);
}

// ===============================================================================================
// 2. 卡通漫反射 - 离散化的兰伯特漫反射
// ===============================================================================================
float ToonDiffuse(float3 N, float3 L, int levels)
{
    float NdotL = max(0.0, dot(N, L));
    return Posterize(NdotL, levels);
}

// 带平滑过渡的卡通漫反射 - 更柔和的分级
float ToonDiffuseSmooth(float3 N, float3 L, int levels, float smoothness)
{
    float NdotL = max(0.0, dot(N, L));
    
    // 使用smoothstep创建更清晰的分级
    float step = 1.0 / float(levels);
    float level = floor(NdotL / step);
    float threshold = (level + 0.5) * step;
    
    return smoothstep(threshold - smoothness, threshold + smoothness, NdotL) * step + level * step;
}

// ===============================================================================================
// 3. 卡通高光 - 类似 Blinn-Phong 但带硬边缘
// ===============================================================================================
float ToonSpecular(float3 N, float3 V, float3 L, float size, float smoothness)
{
    float3 H = normalize(V + L);
    float NdotH = max(0.0, dot(N, H));
    
    // 使用 smoothstep 创建硬边缘高光
    // size 控制高光大小，smoothness 控制边缘硬度
    float specular = pow(NdotH, 1.0 / max(size, 0.001));
    return smoothstep(0.5 - smoothness, 0.5 + smoothness, specular);
}

// ===============================================================================================
// 4. 边缘光 (Rim Lighting) - 卡通风格的重要元素
// ===============================================================================================
float ToonRim(float3 N, float3 V, float amount, float threshold)
{
    // 边缘光：当视线与法线垂直时最强
    float rim = 1.0 - max(0.0, dot(N, V));
    
    // 创建硬边缘
    rim = smoothstep(threshold - 0.05, threshold + 0.05, rim);
    
    return rim * amount;
}

// ===============================================================================================
// 5. 完整的卡通着色函数 - 组合所有效果
// ===============================================================================================
// 简单版本：只计算一个方向光的卡通着色
float3 ToonShading_Simple(
    float3 baseColor,        // 材质基础颜色
    float3 N,                // 表面法线
    float3 V,                // 视线方向
    float3 L,                // 光源方向
    float3 lightColor,       // 光源颜色
    ToonParameters params    // 卡通参数
)
{
    // 1. 离散化的漫反射
    float diffuse = ToonDiffuseSmooth(N, L, params.levels, 0.05);
    float3 diffuseColor = baseColor * lightColor * diffuse;
    
    // 2. 卡通高光（通常是白色或接近白色）
    float specular = ToonSpecular(N, V, L, params.specular_size, params.specular_smoothness);
    float3 specularColor = lightColor * specular;
    
    // 3. 边缘光（通常带一点颜色）
    float rim = ToonRim(N, V, params.rim_amount, params.rim_threshold);
    float3 rimColor = baseColor * rim;
    
    // 组合所有贡献
    return diffuseColor + specularColor + rimColor;
}

// ===============================================================================================
// 6. 多光源卡通着色 - 用于光线追踪场景
// ===============================================================================================
// 这个函数设计为在你的路径追踪器中替换 PBR 着色
float3 ToonShading_MultiLight(
    float3 baseColor,
    float3 N,
    float3 V,
    float3 P,                // 着色点位置
    ToonParameters params,
    // 以下参数从你的着色器传入
    int numLights,
    StructuredBuffer<Light> lights
)
{
    float3 result = float3(0, 0, 0);
    
    // 卡通风格的环境光（底色）
    float3 ambient = baseColor * 0.15;
    result += ambient;
    
    // 遍历所有光源
    for (int i = 0; i < numLights; i++)
    {
        Light light = lights[i];
        float3 L;
        float3 lightColor = light.color;
        
        // 计算光源方向
        if (light.type == LIGHT_TYPE_POINT)
        {
            L = normalize(light.position - P);
            // 点光源距离衰减
            float dist = length(light.position - P);
            lightColor /= (dist * dist + 1.0);
        }
        else // LIGHT_TYPE_AREA
        {
            L = normalize(light.position - P);
            // 面光源归一化强度
            lightColor *= 0.015;
        }
        
        // 计算卡通分级漫反射
        float NdotL = max(0.0, dot(N, L));
        float diffuse;
        
        // 根据级别数创建清晰的明暗分级
        if (params.levels == 2) {
            // 2级：暗/亮
            diffuse = step(0.5, NdotL);
        } else if (params.levels == 3) {
            // 3级：暗/中/亮
            diffuse = floor(NdotL * 3.0) / 2.0;
        } else {
            // 4级：暗/中暗/中亮/亮
            diffuse = floor(NdotL * 4.0) / 3.0;
        }
        
        // 卡通高光
        float specular = ToonSpecular(N, V, L, params.specular_size, params.specular_smoothness);
        
        // 组合光照
        result += baseColor * lightColor * (diffuse * 0.8 + 0.2);
        result += lightColor * specular * 0.3;
    }
    
    // 边缘光（Rim lighting）
    float rim = ToonRim(N, V, params.rim_amount, params.rim_threshold);
    result += baseColor * rim * 0.2;
    
    return result;
}

// ===============================================================================================
// 7. 材质特定的卡通着色 - 从 Material 结构体读取参数
// ===============================================================================================
float3 EvaluateToonMaterial(
    Material mat,
    float3 N,
    float3 V,
    float3 P,
    int numLights,
    StructuredBuffer<Light> lights
)
{
    // 获取基础颜色
    float3 baseColor = mat.base_color;
    
    // 根据材质参数调整卡通效果
    ToonParameters params = GetDefaultToonParams();
    
    // 可以用 roughness 控制明暗级别
    // roughness 低 -> 更多级别（更细腻）
    // roughness 高 -> 更少级别（更平坦）
    if (mat.roughness < 0.3)
        params.levels = 4;
    else if (mat.roughness < 0.6)
        params.levels = 3;
    else
        params.levels = 2;
    
    // 用 metallic 控制高光强度
    params.specular_size = lerp(0.05, 0.15, 1.0 - mat.metallic);
    
    // 调用多光源着色
    return ToonShading_MultiLight(baseColor, N, V, P, params, numLights, lights);
}

// ===============================================================================================
// 8. 描边辅助函数（用于后处理或几何着色器）
// ===============================================================================================
// 基于法线和深度的边缘检测
bool IsOutlineEdge(float3 N, float depth, float3 N_neighbor, float depth_neighbor, float threshold)
{
    // 法线差异检测
    float normalDiff = dot(N, N_neighbor);
    if (normalDiff < threshold)
        return true;
    
    // 深度差异检测
    float depthDiff = abs(depth - depth_neighbor);
    if (depthDiff > 0.1)
        return true;
    
    return false;
}

#endif // TOON_HLSLI
