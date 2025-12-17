#ifndef RANDOM_HLSLI
#define RANDOM_HLSLI

// 简单的常量 PI
static const float PI = 3.14159265359f;
static const float EPS = 1e-5f;

// PCG Hash 随机数生成器
uint init_rand(uint val0, uint val1, uint backoff = 16)
{
    uint v0 = val0, v1 = val1, s0 = 0;
    [unroll]
    for (uint n = 0; n < backoff; n++)
    {
        s0 += 0x9e3779b9;
        v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
        v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
    }
    return v0;
}

float next_rand(inout uint s)
{
    s = (1664525u * s + 1013904223u);
    return float(s & 0x00FFFFFF) / float(0x01000000);
}

// 辅助函数：生成余弦加权的半球采样方向
float3 GetCosineWeightedSample(float3 N, inout uint seed)
{
    float u1 = next_rand(seed);
    float u2 = next_rand(seed);
    
    float r = sqrt(u1);
    float theta = 2.0 * PI * u2;
    
    float x = r * cos(theta);
    float y = r * sin(theta);
    float z = sqrt(max(0.0, 1.0 - u1));
    
    // 构建切线空间 (TBN)
    float3 up = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 T = normalize(cross(up, N));
    float3 B = cross(N, T);
    
    return normalize(T * x + B * y + N * z);
}

#endif