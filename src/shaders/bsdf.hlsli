#include "common.hlsli"
#include "random.hlsli"

















// ===============================================================================================
// 0. 结构体定义 (Required)
// ===============================================================================================
struct BSDFSample
{
    float3 direction;     // 采样的出射方向 L
    float pdf;            // 该方向的总 PDF（考虑所有 lobe）
    float3 bsdf;          // BSDF 的值 f(v,l)（不含 cos）
    bool isTransmission;  // 是否是透射
};

// ===============================================================================================
// 1. 辅助数学函数 (GGX, Smith, Fresnel)
// ===============================================================================================

// 辅助：计算平方
float Square(float x)
{
    return x * x;
}

// GGX Normal Distribution Function (NDF)
// Trowbridge-Reitz GGX
float D_GGX(float NdotH, float alpha)
{
    float a2 = alpha * alpha;
    float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom + EPS);
}

// Smith Geometry Function (Height-Correlated)
// 比分离的 Smith G 更准确，能量更守恒

float G1_GGX(float NdotV, float alpha)
{
    return 2.0 * NdotV / (NdotV + sqrt(alpha * alpha + (1.0 - alpha * alpha) * NdotV * NdotV) + EPS);
}



// V: View, L: Light, alpha: roughness^2
float G_Smith(float NdotV, float NdotL, float alpha)
{
    
    NdotV = max(NdotV, EPS);
    NdotL = max(NdotL, EPS);
    float a2 = alpha * alpha;
    float lambdaV = (-1.0 + sqrt(1.0 + a2 * (1.0 - NdotV * NdotV) / (NdotV * NdotV))) * 0.5;
    float lambdaL = (-1.0 + sqrt(1.0 + a2 * (1.0 - NdotL * NdotL) / (NdotL * NdotL))) * 0.5;
    float G = 1.0 / (1.0 + lambdaV + lambdaL);
    return G;
}

// Fresnel Schlick
// F0: 垂直入射反射率, F90: 掠射角反射率(通常为1)
float3 F_Schlick(float CosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(max(0.0, 1.0 - CosTheta), 5.0);
}

float3 BurleyDiffuseTerm(float3 N, float3 V, float3 L, float roughness)
{
    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));
    
    float3 H = normalize(L + V);
    float HdotV = saturate(dot(H, V));
    
    float Fd90 = 0.5 + 2.0 * roughness * HdotV * HdotV;
    float FL = pow(1.0 - NdotL, 5.0);
    float FV = pow(1.0 - NdotV, 5.0);
    
    float diffuseTerm = (1 + (Fd90 - 1) * FL) * (1 + (Fd90 - 1) * FV);
    
    return diffuseTerm;
}

// 完整的 Fresnel Dielectric (绝缘体精确菲涅尔)
// 用于处理折射和反射的精确比例
float F_Dielectric(float CosTheta, float eta)
{
    float sinThetaT2 = eta * eta * (1.0 - CosTheta * CosTheta);
    // 全内反射 (Total Internal Reflection)
    if (sinThetaT2 > 1.0)
        return 1.0;

    float cosThetaT = sqrt(max(0.0, 1.0 - sinThetaT2));
    
    float r_parl = ((eta * CosTheta) - cosThetaT) / ((eta * CosTheta) + cosThetaT);
    float r_perp = ((CosTheta) - (eta * cosThetaT)) / ((CosTheta) + (eta * cosThetaT));
    
    return 0.5 * (r_parl * r_parl + r_perp * r_perp);
}

bool refract(float3 I, float3 N, float eta, out float3 O)
{
    float cosI = dot(-I, N);
    float sinT2 = eta * eta * (1.0 - cosI * cosI);
    if (sinT2 > 1.0)
    {
        O = float3(0, 0, 0);
        return true; // TIR
    }
    float cosT = sqrt(1.0 - sinT2);
    float sign = (cosI > 0.0) ? 1.0 : -1.0;
    O = eta * I + (eta * cosI - sign * cosT) * N;
    return false; // No TIR
}




void BuildOrthonormalBasis(float3 N, out float3 T, out float3 B)
{
    float3 up = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    T = normalize(cross(up, N));
    B = cross(N, T);
}

// 采样余弦加权半球 (用于漫反射)
float3 SampleCosineWeightedHemisphere(float3 N, inout uint seed)
{
    float u1 = next_rand(seed);
    float u2 = next_rand(seed);
    float3 T, B;
    BuildOrthonormalBasis(N, T, B);
    float r = sqrt(u1);
    float phi = 2.0 * PI * u2;
    float x = r * cos(phi);
    float y = r * sin(phi);
    float z = sqrt(max(0.0, 1.0 - u1));
    return x * T + y * B + z * N;
}



// 采样 GGX 微表面法线 H
// 返回: 本地坐标系下的 H (切线空间) 或者 世界空间下的 H (取决于传入的 N)
// 这里假设传入的 N 是世界空间，我们构建 TBN 进行采样
float3 SampleGGX(float3 N, float alpha, inout uint seed)
{
    float u1 = next_rand(seed);
    float u2 = next_rand(seed);
    
    float a = alpha;
    
    float phi = 2.0 * PI * u1;
    float cosTheta = sqrt((1.0 - u2) / (1.0 + (a * a - 1.0) * u2));
    float sinTheta = sqrt(max(0.0, 1.0 - cosTheta * cosTheta));
    
    float3 H_Local = float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
    
    float3 Tangent, Bitangent;
    BuildOrthonormalBasis(N, Tangent, Bitangent);
    
    return normalize(Tangent * H_Local.x + Bitangent * H_Local.y + N * H_Local.z);
}

float3 SampleGGXVNDF(float3 V, float3 N, float alpha, inout uint seed)
{
    if (dot(N, V) < 0.0)
    {
        // 保证 V 在 N 的同一半球
        V = -V;
    }
    
    
    // 1. 构建正交基
    float3 T, B;
    BuildOrthonormalBasis(N, T, B);

    // 2. 转到局部空间
    float3 Vlocal = float3(dot(V, T), dot(V, B), dot(V, N));

    // 3. 拉伸视线
    float3 Vh = normalize(float3(alpha * Vlocal.x, alpha * Vlocal.y, Vlocal.z));

    // 4. 构建局部基
    float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
    float3 T1 = lensq > 0.0
        ? float3(-Vh.y, Vh.x, 0.0) / sqrt(lensq)
        : float3(1.0, 0.0, 0.0);
    float3 T2 = cross(Vh, T1);

    // 5. 采样单位盘
    float r = sqrt(next_rand(seed));
    float phi = 2.0 * PI * next_rand(seed);
    float t1 = r * cos(phi);
    float t2 = r * sin(phi);

    // 6. 可见性修正
    float s = 0.5 * (1.0 + Vh.z);
    t2 = lerp(sqrt(max(0.0, 1.0 - t1 * t1)), t2, s);

    // 7. 得到可见法线
    float3 Nh = t1 * T1 + t2 * T2 +
                sqrt(max(0.0, 1.0 - t1 * t1 - t2 * t2)) * Vh;

    // 8. 反拉伸
    float3 Hlocal = normalize(float3(alpha * Nh.x, alpha * Nh.y, max(0.0, Nh.z)));

    // 9. 回到世界空间
    return normalize(T * Hlocal.x + B * Hlocal.y + N * Hlocal.z);
}




// 检查向量是否在同一半球
bool IsSameHemisphere(float3 A, float3 B, float3 N)
{
    return dot(A, N) * dot(B, N) > 0.0;
}

float Luminance(float3 color)
{
    return dot(color, float3(0.2126, 0.7152, 0.0722));
}

// ===============================================================================================
// 辅助函数：Disney Diffuse 实现 (用于支持 Subsurface 近似)
// ===============================================================================================
float3 EvalDisneyDiffuse(float NdotL, float NdotV, float LdotH, float roughness, float3 baseColor, float subsurface)
{
    float Fd90 = 0.5 + 2.0 * roughness * LdotH * LdotH;
    float FL = pow(1.0 - NdotL, 5);
    float FV = pow(1.0 - NdotV, 5);
    
    // 基础 Diffuse (Retro-reflection oriented)
    float Fd = lerp(1.0, Fd90, FL) * lerp(1.0, Fd90, FV);
    
    // Subsurface 近似 (Based on Hanrahan-Krueger)
    // Fss90 通常更加平坦
    float Fss90 = LdotH * LdotH * roughness;
    float Fss = lerp(1.0, Fss90, FL) * lerp(1.0, Fss90, FV);
    float ss = 1.25 * (Fss * (1.0 / (NdotL + NdotV + EPS) - 0.5) + 0.5);
    
    // 混合两者
    float3 diffuse = baseColor * INV_PI * lerp(Fd, ss, subsurface);
    return diffuse;
}

// Sheen 近似
float3 EvalSheen(float LdotH, float sheenIntensity, float3 baseColor)
{
    if (sheenIntensity <= 0.0)
        return float3(0, 0, 0);
    float FH = pow(1.0 - LdotH, 5);
    float3 color = baseColor / (Luminance(baseColor) + EPS); // 归一化颜色
    return sheenIntensity * baseColor * FH; // 简单的 Sheen 颜色叠加
}

// ===============================================================================================
// 3. GetLobeWeight 函数实现 (新增 Clearcoat 权重)
// ===============================================================================================


void GetLobeWeight(
    float3 V, float3 N, float metallic, float transmission, float3 eta_vec,
    float clearcoat, float clearcoatRoughness,
    out float w_spec, out float w_diff, out float w_trans, out float w_cc
)
{
    float NdotV = abs(dot(V, N));
    float eta = (eta_vec.r + eta_vec.g + eta_vec.b) / 3.0;
    
    // 1. 计算 Clearcoat 权重 (假设 Clearcoat IOR = 1.5 -> F0 = 0.04)
    // Clearcoat 作为一个顶层，它的反射强度基于 Fresnel
    float F_cc = Luminance(F_Schlick(NdotV, float3(0.25, 0.25, 0.25)));
    w_cc = clearcoat * F_cc;
    
    // 剩余能量传递给 Base Layer
    float attenuation = 1.0 - w_cc;
    
    // 2. Base Layer 的 Fresnel (用于主 Specular)
    float FresnelBase = F_Dielectric(NdotV, eta);
    
    // 金属全是镜面反射
    float base_spec = lerp(FresnelBase, 1.0, metallic);
    float base_diff = (1.0 - metallic) * (1.0 - FresnelBase) * (1.0 - transmission);
    float base_trans = (1.0 - metallic) * transmission;
    
    // 应用 Clearcoat 的衰减
    w_spec = base_spec * attenuation;
    w_diff = base_diff * attenuation;
    w_trans = base_trans * attenuation;

}

// ===============================================================================================
// 4. EvalBSDF 函数实现 (整合 Subsurface, Specular Intensity, Clearcoat, Sheen)
// ===============================================================================================
void EvalBSDF(
    float3 V,
    float3 L,
    float3 N,
    Material mat,
    out float3 bsdf_value,
    out float pdf
)
{
    // 参数准备
    float roughness = max(0.001, mat.roughness);
    float alpha = roughness * roughness;
    float metallic = mat.metallic;
    float3 ior = mat.ior;
    float transmission = mat.transmission;
    float3 basecolor = mat.base_color;
    
    // Clearcoat 固定参数 (如果没有指定，通常设为 0.25 或更小)
    float clearcoatRoughness = 0.2;
    float alphaCC = clearcoatRoughness * clearcoatRoughness;
    
    bsdf_value = float3(0, 0, 0);
    pdf = 0.0;
    
    bool isTransmission = !(dot(V, N) * dot(L, N) > 0.0);
    
    // 计算基础 F0 (考虑 Specular Intensity)
    // Dielectric F0 = ((ior-1)/(ior+1))^2. 
    // Specular 参数通常用于缩放这个 F0. (Disney: spec=0.5 -> IOR=1.5)
    float ior_avg = (ior.r + ior.g + ior.b) / 3.0;
    float f0_dielectric = pow((ior_avg - 1.0) / (ior_avg + 1.0), 2.0);
    // 重新映射: spec 0.5 对应标准 f0, spec 0 -> 0, spec 1 -> 2*f0
    float3 F0_dielectric_vec = float3(f0_dielectric, f0_dielectric, f0_dielectric);
    float3 F0 = lerp(F0_dielectric_vec, basecolor, metallic);
    
    
    
    
    float3 H;
    float eta = (dot(V, N) > 0.0) ? (1.0 / ior_avg) : ior_avg;
    
    if (isTransmission)
    {
        H = normalize(eta * V + L);
        if (IsSameHemisphere(V, L, H))
            return;
    }
    else
    {
        H = normalize(V + L);
    }
    
    if (dot(H, N) < 0.0)
        H = -H;
    
    // 计算各个 Lobe 的权重 (PDF 选择概率)
    float wSpec, wDiff, wTrans, wCC;
    GetLobeWeight(V, N, metallic, transmission, eta, mat.clearcoat, clearcoatRoughness, wSpec, wDiff, wTrans, wCC);
    
    float sumW = wDiff + wSpec + wTrans + wCC + 1e-6;
    float pDiff = wDiff / sumW;
    float pSpec = wSpec / sumW;
    float pTrans = wTrans / sumW;
    float pCC = wCC / sumW;
    
    
    float NdotV = abs(dot(N, V));
    float NdotL = abs(dot(N, L));
    float LdotH = abs(dot(L, H));
    float VdotH = abs(dot(V, H));
    float NdotH = abs(dot(N, H));

    // Clearcoat Fresnel 用于衰减底层
    float3 Fc = F_Schlick(VdotH, 0.04); // Clearcoat IOR 1.5 -> F0=0.04
    float ccAttenuation = 1.0;

    // ===========================
    // Evaluation Logic
    // ===========================

    if (isTransmission)
    {
        // 透射部分 (BTDF)
        // 注意：透射层也被 Clearcoat 覆盖，所以需要 ccAttenuation
        
        float G = G_Smith(NdotV, NdotL, alpha);
        float D = D_GGX(NdotH, alpha);
        
        float3 F_color = F_Schlick(LdotH, F0);
        float3 T_color = 1.0 - F_color; // 内部 Fresnel
        
        float sqrtDenom = eta * VdotH + LdotH;
        float denom = sqrtDenom * sqrtDenom + EPS;
        
        float3 btdf = VdotH * LdotH * T_color * G * D / (NdotV * NdotL * denom + EPS);
        
        // 应用 Clearcoat 衰减
        bsdf_value += pTrans * btdf * basecolor * ccAttenuation;
        
        float pdfH = D * G1_GGX(VdotH, alpha) * VdotH / (abs(dot(V, N)) + EPS);
        float jacobian = VdotH / denom;
        pdf += pTrans * pdfH * jacobian;
    }
    else
    {
        // 反射部分
        
        // 1. Clearcoat Lobe (新增)
        if (mat.clearcoat > 0.0)
        {
            float D_cc = D_GGX(NdotH, alphaCC);
            float G_cc = G_Smith(NdotV, NdotL, alphaCC); // 通常 Clearcoat 使用简单的 G，这里复用 Smith
            float3 F_cc_term = Fc; // 使用之前计算的 Fc
            
            float3 brdf_cc = F_cc_term * D_cc * G_cc / (4.0 * NdotV * NdotL + EPS);
            bsdf_value += pCC * brdf_cc * mat.clearcoat; // 强度受 clearcoat 参数控制
            
            float pdfH_cc = D_cc * G1_GGX(LdotH, alphaCC) * LdotH / (abs(dot(L, N)) + EPS);
            float jacobian_cc = 1.0 / (4.0 * VdotH + EPS);
            pdf += pCC * pdfH_cc * jacobian_cc;
        }

        // 2. Base Diffuse + Subsurface + Sheen
        // 计算 Disney Diffuse 以处理 Subsurface
        float3 diffuseTerm = EvalDisneyDiffuse(NdotL, NdotV, LdotH, roughness, basecolor, mat.subsurface);
        
        // 计算 Sheen (相加项)
        float3 sheenTerm = EvalSheen(LdotH, mat.sheen, basecolor);
        
        // 组合 Diffuse: (Diffuse + Sheen) * Attenuation
        bsdf_value += pDiff * (diffuseTerm + sheenTerm) * ccAttenuation;
        
        // Diffuse PDF (Lambertian 近似)
        pdf += pDiff * NdotL * INV_PI;

        // 3. Base Specular
        float G = G_Smith(NdotV, NdotL, alpha);
        float D = D_GGX(NdotH, alpha);
        float3 F_color = F_Schlick(VdotH, F0);
        
        float3 brdf_spec = F_color * G * D / (4.0 * NdotV * NdotL + EPS);
        bsdf_value += pSpec * brdf_spec * ccAttenuation;
        
        float pdfH = D * G1_GGX(LdotH, alpha) * LdotH / (abs(dot(L, N)) + EPS);
        float jacobian = 1.0 / (4.0 * VdotH + EPS);
        pdf += pSpec * pdfH * jacobian;
    }
}

// ===============================================================================================
// 3. SampleBSDF 函数实现 (加入 Clearcoat 采样分支)
// ===============================================================================================
bool SampleBSDF(
    float3 V,
    float3 N,
    Material mat,
    inout uint seed,
    out BSDFSample ret,
    inout RayPayload payload
)
{
    // 参数预处理
    float roughness = max(0.001, mat.roughness);
    float alpha = roughness * roughness;
    float metallic = mat.metallic;
    float3 ior = mat.ior; // 使用float3保留色散信息
    float transmission = mat.transmission;
    float clearcoatRoughness = 0.2; // 固定的 Clearcoat 粗糙度
    float alphaCC = clearcoatRoughness * clearcoatRoughness;

    
    // ========== 色散功能（使用固定波长通道） ==========
    // 检查当前材质是否有色散（IOR的RGB分量不同）
    bool has_dispersion = (abs(ior.r - ior.g) > 0.01 || abs(ior.g - ior.b) > 0.01);
    
    // 选择IOR：使用payload中固定的波长通道
    float ior_scalar;
    float3 eta_vec;  // 用于GetLobeWeight的向量
    
    if (has_dispersion && transmission > 0.01 )
    {
        int channel;
        if (payload.wavelength_channel >= 0) {
            channel = payload.wavelength_channel;
        } else {
            // 随机选择一个通道
            float randVal = next_rand(seed);
            if (randVal < 1.0 / 3.0)
                channel = 0; // 红色通道
            else if (randVal < 2.0 / 3.0)
                channel = 1; // 绿色通道
            else
                channel = 2; // 蓝色通道
            payload.wavelength_channel = channel; // 存储选择的通道
        }
        // 有色散的透射材质：使用固定的波长通道（从payload获取）
        if (channel == 0)
            ior_scalar = ior.r;
        else if (channel == 1)
            ior_scalar = ior.g;
        else if (channel == 2)
            ior_scalar = ior.b;
        
        eta_vec = (dot(V, N) > 0.0) ? (1.0 / ior) : ior;  // 使用完整的色散IOR向量
    }
    else
    {
        // 无色散或非透射：使用平均IOR
        ior_scalar = (ior.r + ior.g + ior.b) / 3.0;
        float eta_s = (dot(V, N) > 0.0) ? (1.0 / ior_scalar) : ior_scalar;
        eta_vec = float3(eta_s, eta_s, eta_s);  // 统一的IOR向量
    }
    // ========== 色散功能结束 ==========
    
    float eta = (dot(V, N) > 0.0) ? (1.0 / ior_scalar) : ior_scalar;
    // Lobe 概率估计 (新增 Clearcoat)
    float wSpec, wDiff, wTrans, wCC;
    GetLobeWeight(V, N, metallic, transmission, eta_vec, mat.clearcoat, clearcoatRoughness, wSpec, wDiff, wTrans, wCC);
    
    float sumW = wSpec + wDiff + wTrans + wCC;
    float pSpec = wSpec / sumW;
    float pDiff = wDiff / sumW;
    float pTrans = wTrans / sumW;
    float pCC = wCC / sumW;
    
    float randLobe = next_rand(seed);
    
    float3 L = float3(0, 0, 0);
    bool sampled = false;
    
    // 概率区间: [0, pDiff) -> Diffuse
    //           [pDiff, pDiff + pSpec) -> Base Specular
    //           [pDiff + pSpec, pDiff + pSpec + pTrans) -> Transmission
    //           [... , 1.0] -> Clearcoat
    
    float pAccum = pDiff;
    
    // --- 1. Sample Diffuse ---
    if (randLobe < pAccum)
    {
        L = SampleCosineWeightedHemisphere(N, seed);
        float k = 0.5;
        float theta = (PI / 2.0);
        payload.angle = max(sqrt(payload.angle * payload.angle + k * theta * theta), (PI / 2.0));
        sampled = true;
    }
    
    // --- 2. Sample Base Specular ---
    if (!sampled)
    {
        pAccum += pSpec;
        if (randLobe < pAccum)
        {
            float3 H = SampleGGXVNDF(V, N, alpha, seed);
            L = reflect(-V, H);
            if (IsSameHemisphere(L, V, N))
            {
                float k = 0.0;
                float theta = atan(2.0 * roughness);
                payload.angle = max(sqrt(payload.angle * payload.angle + k * theta * theta), (PI / 2.0));
                sampled = true;
            }
        }
    }
    
    // --- 3. Sample Transmission ---
    if (!sampled)
    {
        pAccum += pTrans;
        if (randLobe < pAccum)
        {
            float3 H = SampleGGXVNDF(V, N, alpha, seed);
            bool tir = refract(-V, H, eta, L);
            if (tir)
            {
                // 全内反射 -> 转为 Specular
                L = reflect(-V, H);
                if (IsSameHemisphere(L, V, N))
                {
                    float k = 0.0;
                    float theta = atan(2.0 * roughness);
                    payload.angle = max(sqrt(payload.angle * payload.angle + k * theta * theta), (PI / 2.0));
                    sampled = true;
                }
            }
            else
            {
                // 折射
                if (!IsSameHemisphere(L, V, N)) // 必须在异侧
                {
                    payload.angle *= eta;
                    float k = 0.0;
                    float theta = atan(2.0 * roughness);
                    payload.angle = max(sqrt(payload.angle * payload.angle + k * theta * theta), (PI / 2.0));
                    sampled = true;
                }
            }
        }
    }
    
    // --- 4. Sample Clearcoat ---
    if (!sampled)
    {
        // 剩下的概率区间归属于 Clearcoat
        float3 H = SampleGGXVNDF(V, N, alphaCC, seed);
        L = reflect(-V, H);
        if (IsSameHemisphere(L, V, N))
        {
             // Clearcoat 通常比较光滑
            float k = 0.0;
            float theta = atan(2.0 * clearcoatRoughness);
            payload.angle = max(sqrt(payload.angle * payload.angle + k * theta * theta), (PI / 2.0));
            sampled = true;
        }
    }

    if (!sampled)
        return false;

    // 使用内部评估函数获取完整的 BSDF 和 PDF
    EvalBSDF(V, L, N, mat, ret.bsdf, ret.pdf);
    
    ret.isTransmission = (dot(V, N) * dot(L, N) <= 0.0);
    ret.direction = L;
    
    // 验证结果
    if (ret.pdf < 1e-8 || length(ret.bsdf) < 1e-8)
        return false;
    
    return true;
}