#include "common.hlsli"
#include "random.hlsli"

// === PBR Helper Constants ===

// 1. Fresnel Schlick Approximation
// F0: 0度角的反射率。非金属通常是 0.04，金属则是 Albedo 颜色。
float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}


// 2. Normal Distribution Function (GGX / Trowbridge-Reitz)
// N: 宏观法线, H: 半程向量 (L+V), roughness: 粗糙度
float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / max(denom, 0.0000001); // 避免除以0
}

// 3. Geometry Function (Smith's Schlick-GGX)
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0; // 直接光照下的 k 值计算 (IBL下不同)

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / max(denom, 0.0000001);
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = GeometrySchlickGGX(NdotV, roughness);
    float ggx2 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

void GetKSandKD(float3 N, float3 V, float3 albedo, float roughness, float metallic, out float3 kS, out float3 kD)
{
    // 基础反射率 F0
    //float3 F0 = float3(0.04, 0.04, 0.04);
    float3 F0 = float3(0.00, 0.00, 0.00);
    F0 = lerp(F0, albedo, metallic); // 如果是金属，F0 就是 albedo
    // 计算 kS 和 kD
    kS = fresnelSchlick(max(dot(N, V), 0.0), F0);
    kD = float3(1.0, 1.0, 1.0) - kS;
    kD *= (1.0 - metallic); // 金属没有漫反射成分
}

float CastScaler(float3 x)
{
    return dot(float3(0.2126, 0.7152, 0.0722), x);
}

float GetSpecularChance(float3 N, float3 V, float3 albedo, float roughness, float metallic)
{
    float3 kS;
    float3 kD;
    GetKSandKD(N, V, albedo, roughness, metallic, kS, kD);
    float spec = CastScaler(kS);
    float diff = CastScaler(kD);
    return spec / (spec + diff);
}

// === Evaluation Function ===
// 计算 BRDF 的值，用于直接光照计算 (Direct Lighting)
float3 EvalPBR(float3 N, float3 V, float3 L, float3 albedo, float roughness, float metallic)
{
    float3 H = normalize(V + L);
    
    // 基础反射率 F0
    float3 kS;
    float3 kD;
    GetKSandKD(N, V, albedo, roughness, metallic, kS, kD);

    // Cook-Torrance BRDF 分项
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    float3 F = kS;
       
    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // +0.0001 防止除零
    float3 specular = numerator / denominator;

    // 最终 BRDF = Diffuse + Specular
    return (kD * albedo / PI + specular);
}

// 你现有的代码是分层采样的，所以总 PDF = P(spec) * PDF_spec + P(diff) * PDF_diff
float EvalBrdfPDF(float3 N, float3 V, float3 L, float3 albedo, float roughness, float metallic)
{
    if (dot(N, L) <= 0.0)
        return 0.0;

    float3 H = normalize(V + L);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float spec_chance = GetSpecularChance(N, V, albedo, roughness, metallic);
    
    // 2. Specular PDF (GGX)
    float D = DistributionGGX(N, H, roughness);
    // 注意：GGX 标准 PDF 是针对半程向量 H 的，转换到光线方向 L 需要除以 4*(V.H)
    float pdf_spec = (D * NdotH) / (4.0 * HdotV + 0.0001);
    
    // 3. Diffuse PDF (Cosine Weighted)
    float pdf_diff = NdotL / PI;

    // 4. 混合 PDF
    return spec_chance * pdf_spec + (1.0 - spec_chance) * pdf_diff;
}

// 生成 GGX 重要性采样的半程向量 H
float3 SampleGGX(float u1, float u2, float3 N, float roughness)
{
    float a = roughness * roughness;
    float phi = 2.0 * PI * u1;
    float cosTheta = sqrt((1.0 - u2) / (1.0 + (a * a - 1.0) * u2));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    
    // 球面坐标转笛卡尔坐标 (Tangent Space)
    float3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;
    
    // 将 H 从切线空间转换到世界空间
    // 构建简易的 TBN 矩阵
    float3 up = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 tangent = normalize(cross(up, N));
    float3 bitangent = cross(N, tangent);
    
    return normalize(tangent * H.x + bitangent * H.y + N * H.z);
}




// MIS
// [New Function] Power Heuristic
// 用于平衡两个 PDF 的权重。beta 通常取 2 (即 pdf 的平方)
float PowerHeuristic(float pdf_f, float pdf_g)
{
    float f2 = pdf_f * pdf_f;
    float g2 = pdf_g * pdf_g;
    return f2 / (max(f2 + g2, 0.00001));
}