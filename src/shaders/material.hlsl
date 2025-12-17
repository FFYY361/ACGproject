#include "common.hlsli"
#include "random.hlsli"

// === PBR Helper Constants ===

// 1. Fresnel Schlick Approximation
// F0: 0度角的反射率。非金属通常是 0.04，金属则是 Albedo 颜色。
float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

// Fresnel Dielectric (精确计算介质的菲涅尔反射率)
float fresnelDielectric(float cosThetaI, float etaI, float etaT)
{
    cosThetaI = clamp(cosThetaI, -1.0, 1.0);
    
    // 判断是否从内部射出
    bool entering = cosThetaI > 0.0;
    if (!entering)
    {
        float temp = etaI;
        etaI = etaT;
        etaT = temp;
        cosThetaI = abs(cosThetaI);
    }
    
    // Snell's law
    float sinThetaI = sqrt(max(0.0, 1.0 - cosThetaI * cosThetaI));
    float sinThetaT = etaI / etaT * sinThetaI;
    
    // Total internal reflection
    if (sinThetaT >= 1.0)
        return 1.0;
    
    float cosThetaT = sqrt(max(0.0, 1.0 - sinThetaT * sinThetaT));
    
    float Rparl = ((etaT * cosThetaI) - (etaI * cosThetaT)) / ((etaT * cosThetaI) + (etaI * cosThetaT));
    float Rperp = ((etaI * cosThetaI) - (etaT * cosThetaT)) / ((etaI * cosThetaI) + (etaT * cosThetaT));
    
    return (Rparl * Rparl + Rperp * Rperp) / 2.0;
}

// Refract function (折射方向计算)
// I: 入射方向（指向表面）, N: 法线, eta: 折射率比值 (n1/n2)
// 返回折射方向，如果全内反射返回零向量
float3 refract(float3 I, float3 N, float eta)
{
    float cosi = dot(N, I);
    float k = 1.0 - eta * eta * (1.0 - cosi * cosi);
    
    if (k < 0.0)
    {
        return float3(0.0, 0.0, 0.0); // Total internal reflection
    }
    
    return eta * I - (eta * cosi + sqrt(k)) * N;
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
    float NdotV = abs(dot(N, V));
    float NdotL = abs(dot(N, L));
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

float Luminance(float3 x)
{
    return dot(float3(0.2126, 0.7152, 0.0722), x);
}

// 构建局部坐标系 (Orthonormal Basis)
void BuildOrthonormalBasis(float3 N, out float3 T, out float3 B)
{
    float3 up = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    T = normalize(cross(up, N));
    B = cross(N, T);
}

// 采样余弦加权半球 (用于漫反射)
float3 SampleCosineWeightedHemisphere(float u1, float u2, float3 N)
{
    float3 T, B;
    BuildOrthonormalBasis(N, T, B);
    float r = sqrt(u1);
    float phi = 2.0 * PI * u2;
    float x = r * cos(phi);
    float y = r * sin(phi);
    float z = sqrt(max(0.0, 1.0 - u1));
    return x * T + y * B + z * N;
}

/*
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
*/
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





// ========== BSDF Implementation ==========
// BSDF = BRDF (reflection) + BTDF (transmission)

// 采样BSDF：返回出射方向、PDF、以及是否是透射
struct BSDFSample
{
    float3 direction;  // 出射方向
    float pdf;         // PDF
    float3 bsdf;     // BSDF*cos/pdf
    bool isTransmission; // 是否是透射
};

float3 calcFresnel(float ior, float NdotV, float3 albedo, float metallic)
{
    // 1. 计算菲涅尔项作为镜面反射权重的参考
    // 对于电介质，F0 = ((1-ior)/(1+ior))^2
    float f0_dielectric = pow((1.0 - ior) / (1.0 + ior), 2.0);
    // 金属的F0即为albedo，电介质根据metallic混合
    float3 F0 = lerp(float3(f0_dielectric, f0_dielectric, f0_dielectric), albedo, metallic);
    
    // 计算近似的菲涅尔反射强度
    float3 fresnelWeight = fresnelSchlick(NdotV, F0);
    return fresnelWeight;
}

// 计算各Lobe的选择概率
void GetLobeProbabilities(float3 V, float3 N, float3 albedo, float metallic,
                          float transmission, float ior,
                          out float pDiffuse, out float pSpec, out float pTrans)
{
    // 1. 计算菲涅尔项作为镜面反射权重的参考
    // 对于电介质，F0 = ((1-ior)/(1+ior))^2
    float fresnelWeight = Luminance(calcFresnel(ior, abs(dot(N, V)), albedo, metallic));
    
    // 2. 能量权重 (Energy Weights)
    // 金属没有漫反射和透射
    float wSpec = fresnelWeight;
    float wTrans = (1.0 - metallic) * transmission * (1.0 - fresnelWeight);
    float wDiff = (1.0 - metallic) * (1.0 - transmission) * (1.0 - fresnelWeight);
    //wTrans = 0;
    
    // 稍微调整 specular 权重以保证金属完全反射
    wSpec = lerp(wSpec, 1.0, metallic);

    // 3. 归一化为概率 (PDF Sum = 1)
    float sum = wSpec + wTrans + wDiff;
    if (sum > 1e-6)
    {
        pSpec = wSpec / sum;
        pTrans = wTrans / sum;
        pDiffuse = wDiff / sum;
    }
    else
    {
        // 异常情况兜底 (如全黑)
        pSpec = 1.0;
        pTrans = 0.0;
        pDiffuse = 0.0;
    }
}

BSDFSample CalcBSDF(float3 N, float3 V, float3 L, float3 albedo, float roughness, float metallic,
                    float transmission, float ior, inout uint seed)
{
    L = normalize(L);
    BSDFSample result;
    result.direction = L;
    float NdotV = dot(N, V);
    float NdotL = dot(N, L);
    
    
    // 判断是反射还是透射
    bool reflecting = (NdotV * NdotL) > 0.0;
    result.isTransmission = !reflecting;

    // 准备概率
    float pDiff, pSpec, pTrans;
    GetLobeProbabilities(V, N, albedo, metallic, transmission, ior, pDiff, pSpec, pTrans);
    
    // 准备微表面常用变量

    // --- 1. 漫反射贡献 ---
    
    float3 H;
    float3 F;
    float eta = (NdotV > 0.0) ? (1.0 / ior) : ior;
    if (reflecting)
    {
        H = normalize(V + L);
        F = calcFresnel(ior, max(dot(H, V), 0.0), albedo, metallic);
    }
    else
    {
        H = normalize(eta * V + L);
        F = calcFresnel(ior, abs(dot(V, H)), albedo, metallic);
    }
    
    
    float3 diffBSDF = float3(0.0, 0.0, 0.0);
    float diffPDF = 0.0;
    if (reflecting && pDiff > 0.0)
    {
        float3 H = normalize(V + L);
        // Lambertian
        diffBSDF = albedo / PI;
        diffPDF = abs(NdotL) / PI; // Cosine weighted PDF
    }

    // --- 2. 镜面反射贡献 (BRDF) ---
    float3 specBSDF = float3(0.0, 0.0, 0.0);
    float specPDF = 0.0;
    if (reflecting && pSpec > 0.0)
    {
        float3 H = normalize(V + L);
        float G = GeometrySmith(N, V, L, roughness);
        float D = DistributionGGX(N, H, roughness);
        
        float3 F = calcFresnel(ior, max(dot(H, V), 0.0), albedo, metallic);
        
        // Cook-Torrance
        float denom = 4.0 * abs(NdotV) * abs(NdotL) + 1e-5;
        specBSDF = (D * G * F) / denom;
        //specBSDF = D;
        
        // PDF conversion: PDF_H -> PDF_L
        // PDF_L = D(H) * (N.H) / (4 * (V.H))
        float HdotV = max(0.0, dot(H, V));
        float NdotH = max(0.0, dot(N, H));
        specPDF = D * NdotH / (4.0 * HdotV + 1e-5);
    }

    // --- 3. 镜面透射贡献 (BTDF) ---
    float3 transBSDF = float3(0.0, 0.0, 0.0);
    float transPDF = 0.0;
    if (!reflecting && pTrans > 0.0)
    {
        // 处理折射率
        if (dot(H, N) < 0.0)
            H = -H; // 统一朝向
        float NdotH = abs(dot(N, H));
        float VdotH = abs(dot(V, H));
        float LdotH = abs(dot(L, H));
       

        float D = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        
        // F 计算 (透射部分用 1-F)
        
        float sqrtDenom = eta * dot(V, H) + dot(L, H);
        float3 numer = (1.0 - F) * D * G * VdotH * LdotH;
        float denom = (abs(NdotL) * abs(NdotV) + 1e-5) * (sqrtDenom * sqrtDenom + 1e-5);
        transBSDF = albedo * numer / denom;

        // PDF conversion for Refraction
        // PDF_L = PDF_H * |jacobian|
        // Jacobian = eta^2 * (L.H) / (V.H + eta * L.H)^2
        float jacobian = (LdotH) / (sqrtDenom * sqrtDenom + 1e-5);
        transPDF = (D * NdotH) * jacobian;
    }
    
    
    result.bsdf = pDiff * diffBSDF + pSpec * specBSDF + pTrans * transBSDF;
    //result.bsdf = diffBSDF + specBSDF + transBSDF;
    result.pdf = pDiff * diffPDF + pSpec * specPDF + pTrans * transPDF;
    
    // 防止除零
    if (result.pdf < 1e-6)
        result.pdf = 0.0;

    return result;
}

BSDFSample SampleBSDF(float3 N, float3 V, float3 albedo, float roughness, float metallic,
                      float transmission, float ior, inout uint seed)
{
    float u1 = next_rand(seed);
    float u2 = next_rand(seed);
    float u_lobe = next_rand(seed); // 用于选择 Lobe 的随机数

    // 1. 获取选择概率
    float pDiff, pSpec, pTrans;
    GetLobeProbabilities(V, N, albedo, metallic, transmission, ior, pDiff, pSpec, pTrans);
    
    float3 L = float3(0, 0, 1);
    bool sampledTransmission = false;

    // 2. 根据概率选择采样策略
    if (u_lobe < pSpec)
    {
        // --- 采样 Specular BRDF ---
        float3 H = SampleGGX(u1, u2, N, roughness);
        L = reflect(-V, H);
        sampledTransmission = false;
    }
    else if (u_lobe < pSpec + pTrans)
    {
        // --- 采样 Specular BTDF ---
        float3 H = SampleGGX(u1, u2, N, roughness);
        
        float eta = (dot(V, N) > 0.0) ? (1.0 / ior) : ior;
        float3 refrDir;
        // refract 函数：refract(I, N, eta). I是指向表面的，即 -V
        // 需要自己处理全内反射 (TIR)
        float w = dot(V, H);
        float k = 1.0 - eta * eta * (1.0 - w * w);
        if (k < 0.0)
        {
            // 全内反射：退化为反射
            L = float3(0.0, 0.0, 0.0);
            sampledTransmission = false;
        }
        else
        {
            float sign = (dot(N, V) > 0.0) ? 1.0 : -1.0;
            L = (eta * w - sign * sqrt(k)) * H - eta * V;
            sampledTransmission = true;
        }
        L = normalize(L);
    }
    else
    {
        // --- 采样 Diffuse ---
        float3 N_eff = dot(N, V) < 0.0 ? -N : N; // 保证N朝向V所在半球)
        L = SampleCosineWeightedHemisphere(u1, u2, N_eff);
        sampledTransmission = false;
    }

    // 3. 这里的关键点：
    // 虽然我们要么选了漫反射，要么选了镜面，
    // 但返回的 PDF 和 BSDF 必须包含*所有*Lobe对该方向 L 的贡献 (MIS)。
    // 比如：即使我在粗糙表面选了漫反射方向，高光波瓣也可能覆盖到这个方向。
    
    if (length(L) < 0.01)
    {
        BSDFSample result;
        result.bsdf = float3(0, 0, 0);
        result.pdf = 0.0;
        return result;
    }
    BSDFSample result = CalcBSDF(N, V, L, albedo, roughness, metallic, transmission, ior, seed);
    
    // 修正透射标记
    result.isTransmission = sampledTransmission;
    //result.bsdf = L;
    
    return result;
}







/*
// 统一的BSDF采样函数
BSDFSample SampleBSDF(float3 N, float3 V, float3 albedo, float roughness, float metallic, 
                      float transmission, float ior, inout uint seed)
{
    BSDFSample result;
    result.isTransmission = false;
    
    // 如果是透射材质
    if (transmission > 0.5)
    {
        // 判断光线方向
        float cosTheta = dot(V, N);
        bool entering = cosTheta > 0.0;
        
        float eta_ratio;
        float3 outward_normal;
        
        if (entering)
        {
            eta_ratio = 1.0 / ior;
            outward_normal = N;
        }
        else
        {
            eta_ratio = ior;
            outward_normal = -N;
            cosTheta = -cosTheta;
        }
        
        // 计算菲涅尔反射率
        float F = fresnelDielectric(cosTheta, entering ? 1.0 : ior, entering ? ior : 1.0);
        
        // 根据菲涅尔概率决定反射还是折射
        if (next_rand(seed) < F)
        {
            // 反射
            result.direction = reflect(-V, outward_normal);
            result.pdf = 1.0; // Delta分布，PDF形式上为1（已经通过随机采样处理概率）
            result.weight = albedo; // 玻璃颜色（通常是白色）
            result.isTransmission = false;
        }
        else
        {
            // 折射
            float3 incident = -V;
            result.direction = refract(incident, outward_normal, eta_ratio);
            
            // 检查全内反射
            if (length(result.direction) < 0.001)
            {
                result.direction = reflect(-V, outward_normal);
                result.pdf = 1.0;
                result.weight = albedo;
                result.isTransmission = false;
            }
            else
            {
                result.pdf = 1.0; // Delta分布
                // 对于折射，需要考虑立体角的变化（非对称传输）
                // weight = albedo * (eta_t^2 / eta_i^2) 用于辐射度传输
                // 但在大多数实现中，为了简化，直接使用albedo
                result.weight = albedo;
                result.isTransmission = true;
            }
        }
    }
    else
    {
        // 标准BRDF采样
        float spec_chance = GetSpecularChance(N, V, albedo, roughness, metallic);
        
        float3 L;
        if (next_rand(seed) < spec_chance)
        {
            // Specular
            float3 H = SampleGGX(next_rand(seed), next_rand(seed), N, roughness);
            L = reflect(-V, H);
        }
        else
        {
            // Diffuse
            L = GetCosineWeightedSample(N, seed);
        }
        
        result.direction = L;
        result.pdf = EvalBrdfPDF(N, V, L, albedo, roughness, metallic);
        
        if (result.pdf > 0.0001)
        {
            float3 brdf = EvalPBR(N, V, L, albedo, roughness, metallic);
            result.weight = brdf * max(dot(N, L), 0.0) / result.pdf;
        }
        else
        {
            result.weight = float3(0, 0, 0);
        }
        result.isTransmission = false;
    }
    
    return result;
}
*/
/*
// 评估BSDF：给定入射和出射方向，计算BSDF值和PDF
float3 EvalBSDF(float3 N, float3 V, float3 L, float3 albedo, float roughness, 
                float metallic, float transmission, float ior, out float pdf)
{
    // 透射材质：只评估反射部分（用于直接光照）
    // 折射部分是delta分布，只能通过采样获得
    if (transmission > 0.5)
    {
        // 检查L是否在正确的半球（反射侧）
        float NdotL = dot(N, L);
        float NdotV = dot(N, V);
        
        if (NdotL * NdotV <= 0.0)
        {
            // L和V在不同侧，这是透射，无法评估
            pdf = 0.0;
            return float3(0, 0, 0);
        }
        
        // 在同一侧，评估反射BRDF（菲涅尔加权）
        bool entering = NdotV > 0.0;
        float cosTheta = abs(NdotV);
        float F = fresnelDielectric(cosTheta, entering ? 1.0 : ior, entering ? ior : 1.0);
        
        // 返回完美镜面反射的BRDF（delta函数的近似）
        // 实际应该用delta函数，这里返回0因为直接光照无法命中delta分布
        pdf = 0.0;
        return float3(0, 0, 0);
    }
    
    // 标准BRDF
    pdf = EvalBrdfPDF(N, V, L, albedo, roughness, metallic);
    return EvalPBR(N, V, L, albedo, roughness, metallic);
}
*/