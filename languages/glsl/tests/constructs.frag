#version 450 core

struct Light {
    vec3 position;
    float intensity[2];
};

uniform Light lights;
subroutine(ShadingType) float shading(vec3 n);
layout(std140) uniform;

in vec3 normal;
out vec4 fragColor;

float sum(vec3 v, int n)
{
    float total = 0.0, extra = 1.0, scaled[2];
    int i = 0;

    for (i = 0, total = 0.0; i < n; i++)
        total += v[i] * 2 - 1;

    for (;;)
        break;

    switch (n) {
        case 0:
            discard;
        case 1:
            continue;
        default:
            total += 1.0;
    }

    bool ok = (n > 0) && (n < 4) || !(n == 2) ^^ true;
    int bits = (n << 1 | 1) & ~n ^ 3;
    float chosen = (n > 0) ? total : extra;
    float pair = (total += 1.0, total * 2.0);
    vec3 built = vec3(1.0, 2.0, 3.0);
    float arr[] = float[](1.0, 2.0, 3.0);

    return total + scaled[0] + pair + built.x + arr[0] + float(bits) +
           (ok ? chosen : 0.0);
}

void main(void)
{
    fragColor = vec4(sum(normal, 3));
}
