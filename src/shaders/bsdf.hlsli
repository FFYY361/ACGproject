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


// ===============================================================================================
// 3. Sample 函数实现
// 采样下一条光线 L
// ===============================================================================================

void GetLobeWeight(
float3 V, float3 N, float metallic, float transmission, float3 eta,
out float w_spec, out float w_diff, out float w_trans
)
{
    // 使用平均eta值计算Fresnel
    float eta_scalar = (eta.r + eta.g + eta.b) / 3.0;
    float Fresnel = F_Dielectric(abs(dot(V, N)), eta_scalar);
    
    // 金属全是镜面反射
    w_spec = lerp(Fresnel, 1.0, metallic);
    
    // 漫反射：非金属 * 透过的能量 * 不透明的部分
    w_diff = (1.0 - metallic) * (1.0 - Fresnel) * (1.0 - transmission);
    
    // 透射：非金属 * 透明度（Fresnel 已经在 BTDF 公式中处理）
    w_trans = (1.0 - metallic) * transmission;
}



// 仅评估 BSDF 值和 PDF（用于 NEE）
void EvalBSDFForNEE(
    float3 V,
    float3 L,
    float3 N,
    Material mat,
    out float3 bsdf_value,
    out float pdf
)
{
    float alpha = max(0.001, mat.roughness * mat.roughness);
    float metallic = mat.metallic;
    float3 ior = mat.ior;
    float transmission = mat.transmission;
    float3 basecolor = mat.base_color;
    
    bsdf_value = float3(0, 0, 0);
    pdf = 0.0;
    
    bool isTransmission = !(dot(V, N) * dot(L, N) > 0.0);
    
    // 使用平均IOR计算F0和eta（评估函数不使用色散）
    float ior_avg = (ior.r + ior.g + ior.b) / 3.0;
    float f0 = pow((1 - ior_avg) / (1 + ior_avg), 2.0);
    float3 F0 = lerp(float3(f0, f0, f0), basecolor, metallic);
    
    float3 H;
    float eta = (dot(V, N) > 0.0) ? (1.0 / ior_avg) : ior_avg;
    
    if (isTransmission)
    {
        H = normalize(eta * V + L);
        if (IsSameHemisphere(V, L, H))
        {
            return; // 无效配置
        }
    }
    else
    {
        H = normalize(V + L);
    }
    
    if (dot(H, N) < 0.0)
        H = -H;
    
    float wSpec, wDiff, wTrans;
    GetLobeWeight(V, H, metallic, transmission, eta, wSpec, wDiff, wTrans);
    
    float sumW = wDiff + wSpec + wTrans + 1e-6;
    float pDiff = wDiff / sumW;
    float pSpec = wSpec / sumW;
    float pTrans = wTrans / sumW;
    
    if (isTransmission)
    {
        float G = G_Smith(abs(dot(N, V)), abs(dot(N, L)), alpha);
        float D = D_GGX(abs(dot(N, H)), alpha);
        
        // Fresnel 项：透射部分 = 1 - F
        float3 F_color = F_Schlick(abs(dot(L, H)), F0);
        float3 T_color = 1.0 - F_color;
        
        float sqrtDenom = eta * abs(dot(V, H)) + abs(dot(L, H));
        float denom = sqrtDenom * sqrtDenom + EPS;
        
        // BTDF 公式（包含 Fresnel 透射系数）
        float3 btdf = abs(dot(V, H)) * abs(dot(L, H)) * T_color * G * D / (abs(dot(N, V)) * abs(dot(N, L)) * denom + EPS);
        
        // 注意：pTrans 不再包含 (1-F)，因为已经在 T_color 中
        bsdf_value += pTrans * btdf * basecolor;
        
        float pdfH = D * G1_GGX(abs(dot(V, H)), alpha) * abs(dot(V, H)) / (abs(dot(V, N)) + EPS);
        float jacobian = abs(dot(V, H)) / denom;
        pdf += pTrans * pdfH * jacobian;
    }
    else
    {
        // Diffuse
        bsdf_value += pDiff * BurleyDiffuseTerm(N, V, L, mat.roughness) * basecolor / PI;
        pdf += pDiff * abs(dot(N, L)) / PI;
        
        // Specular
        float G = G_Smith(abs(dot(N, V)), abs(dot(N, L)), alpha);
        float D = D_GGX(abs(dot(N, H)), alpha);
        float3 F_color = F_Schlick(abs(dot(V, H)), F0);
        
        float3 brdf = F_color * G * D / (4.0 * abs(dot(N, V)) * abs(dot(N, L)) + EPS);
        bsdf_value += pSpec * brdf;
        
        float pdfH = D * G1_GGX(abs(dot(V, H)), alpha) * abs(dot(V, H)) / (abs(dot(V, N)) + EPS);
        float jacobian = 1.0 / (4.0 * abs(dot(V, H)) + EPS);
        pdf += pSpec * pdfH * jacobian;
    }
}

// 内部使用：完整评估（返回结构体）
BSDFSample EvalBSDF_Internal(
    float3 V,
    float3 L,
    float3 N,
    Material mat
)
{
    float alpha = max(0.001, mat.roughness * mat.roughness); // 防止除零
    float metallic = mat.metallic;
    float3 ior = mat.ior;
    float transmission = mat.transmission;
    float3 basecolor = mat.base_color;
    
    BSDFSample ret;
    ret.direction = L;
    ret.bsdf = 0;
    ret.pdf = 0;
    bool isTransmission = !(dot(V, N) * dot(L, N) > 0.0);
    
    // 只有当IOR三个分量不同时才使用色散F0
    float ior_avg = (ior.r + ior.g + ior.b) / 3.0;
    float f0 = pow((1 - ior_avg) / (1 + ior_avg), 2.0);
    float3 F0 = lerp(float3(f0, f0, f0), basecolor, metallic);
    
    float3 H;
    // 对于评估，使用平均IOR值（不使用色散）
    float avg_ior = ior_avg;
    float eta_scalar = (dot(V, N) > 0.0) ? (1.0 / avg_ior) : avg_ior;
    if (isTransmission)
    {
        H = normalize(eta_scalar * V + L);
        if (IsSameHemisphere(V, L, H) == true)
        {
            ret.bsdf = 0;
            ret.pdf = 0;
            return ret;
        }
    }
    else
    {
        H = normalize(V + L);
    }
    if (dot(H, N) < 0.0)
        H = -H;
    
    // 对于评估，使用平均eta值
    float wSpec;
    float wDiff;
    float wTrans;
    GetLobeWeight(V, H, metallic, transmission, float3(eta_scalar, eta_scalar, eta_scalar), wSpec, wDiff, wTrans);
    
    float sumW = wDiff + wSpec + wTrans + 1e-6;
    float pDiff = wDiff / sumW;
    float pSpec = wSpec / sumW;
    float pTrans = wTrans / sumW;
    
    bool tir = false;
    
    if (isTransmission == true)
    {
        float G = G_Smith(abs(dot(N, V)), abs(dot(N, L)), alpha);
        float D = D_GGX(abs(dot(N, H)), alpha);
        
        // Fresnel 透射系数
        float3 F_color = F_Schlick(abs(dot(L, H)), F0);
        float3 T_color = 1.0 - F_color;
        
        // Use average eta for evaluation (no wavelength info)
        float sqrtDenom = eta_scalar * abs(dot(V, H)) + abs(dot(L, H));
        float denom = sqrtDenom * sqrtDenom + EPS;
        
        // BTDF
        float3 btdf = abs(dot(V, H)) * abs(dot(L, H)) * T_color * G * D / (abs(dot(N, V)) * abs(dot(N, L)) * denom + EPS);
        ret.bsdf += pTrans * btdf * basecolor;
        
        // PDF
        float pdfH = D * G1_GGX(abs(dot(V, H)), alpha) * abs(dot(V, H)) / (abs(dot(V, N)) + EPS);
        float jacobian = abs(dot(V, H)) / denom;
        ret.pdf += pTrans * pdfH * jacobian;
    }
    else
    {
        // Diffuse
        ret.bsdf += pDiff * BurleyDiffuseTerm(N, V, L, mat.roughness) * basecolor / PI;
        ret.pdf += pDiff * abs(dot(N, L)) / PI;
        // Specular
        float3 H = normalize(V + L);
        float G = G_Smith(abs(dot(N, V)), abs(dot(N, L)), alpha);
        float D = D_GGX(abs(dot(N, H)), alpha);
        float3 F_color = F_Schlick(abs(dot(V, H)), F0);
        
        float3 brdf = F_color * G * D / (4.0 * abs(dot(N, V)) * abs(dot(N, L)) + EPS);
        ret.bsdf += pSpec * brdf;
        //Use PDF For VNDF sampling
        float pdfH = D * G1_GGX(dot(V, H), alpha) * abs(dot(V, H)) / (abs(dot(V, N)) + EPS);
        float jacobian = 1.0 / (4.0 * abs(dot(V, H)) + EPS);
        float pdfL = pdfH * jacobian;
        ret.pdf += pSpec * pdfL;
        //ret.bsdf /= (pSpec + pDiff);
    }
    ret.isTransmission = (dot(V, N) * dot(L, N) <= 0.0);
    return ret;
}


// 仅评估 BSDF 值和 PDF（用于 NEE）
// void EvalBSDFForNEE(
//     float3 V,
//     float3 L,
//     float3 N,
//     Material mat,
//     out float3 bsdf_value,
//     out float pdf
// )
// {
//     // 直接调用内部评估函数
//     BSDFSample result = EvalBSDF_Internal(V, L, N, mat);
//     bsdf_value = result.bsdf;
//     pdf = result.pdf;
// }

// 采样 BSDF（带 MIS 的完整实现）
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
    float alpha = max(0.001, mat.roughness * mat.roughness); // 防止除零
    float metallic = mat.metallic;
    float3 ior = mat.ior; // 使用float3保留色散信息
    float transmission = mat.transmission;
    float3 basecolor = mat.base_color;
    
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
    
    // lobe概率估计
    float wSpec;
    float wDiff;
    float wTrans;
    GetLobeWeight(V, N, metallic, transmission, eta_vec, wSpec, wDiff, wTrans);
    
    //normalize
    float sumW = wDiff + wSpec + wTrans;
    float pSpec = wSpec / sumW;
    float pDiff = wDiff / sumW;
    float pTrans = wTrans / sumW;
    
    float randLobe = next_rand(seed);
    
    float3 L = float3(0.0f, 0.0f, 0.0f);
    bool Debug = false;
    float3 debugValue = N;
    
    if (randLobe < pDiff)
    {
        L = SampleCosineWeightedHemisphere(N, seed);
        float k = 0.5; // 调整因子，根据需要调整
        float theta = (PI / 2.0);
        payload.angle = max(sqrt(payload.angle * payload.angle + k * theta * theta), (PI / 2.0));

    }
    if (randLobe >= pDiff && randLobe < pDiff + pSpec)
    {
        float3 H = SampleGGXVNDF(V, N, alpha, seed);
        L = reflect(-V, H);
        if (IsSameHemisphere(L, V, N) == false)
            return false; // 无效采样
        float k = 0; // 调整因子，根据需要调整
        float theta = atan(2 * mat.roughness);
        payload.angle = max(sqrt(payload.angle * payload.angle + k * theta * theta), (PI / 2.0));

    }
    if (randLobe >= pDiff + pSpec)
    {
        float3 H = SampleGGXVNDF(V, N, alpha, seed);
        bool tir = refract(-V, H, eta, L);
        if (tir)
        {
            // 全内反射：转为镜面反射
            L = reflect(-V, H);
            if (IsSameHemisphere(L, V, N) == false)
                return false; // 反射方向无效
            // 不增加 payload.angle，因为这是反射而非折射
            
            float k = 0.0;
            float theta = atan(2.0 * mat.roughness);
            payload.angle = max(sqrt(payload.angle * payload.angle + k * theta * theta), (PI / 2.0));
        }
        else
        {
            // 成功折射
            if (IsSameHemisphere(L, V, N) == true)
                return false; // 无效采样：折射应该在另一半球
            
            payload.angle *= eta;
            
            float k = 0.0;
            float theta = atan(2.0 * mat.roughness);
            payload.angle = max(sqrt(payload.angle * payload.angle + k * theta * theta), (PI / 2.0));
        }
    }
    
    // 使用内部评估函数获取完整的 BSDF 和 PDF
    ret = EvalBSDF_Internal(V, L, N, mat);
    
    // 验证结果
    if (ret.pdf < 1e-8 || length(ret.bsdf) < 1e-8)
        return false;
    
    return true;
}