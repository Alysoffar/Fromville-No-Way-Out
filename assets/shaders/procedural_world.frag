#version 450 core

layout(location = 0) out vec3 gPosition;
layout(location = 1) out vec3 gNormal;
layout(location = 2) out vec4 gAlbedoSpec;
layout(location = 3) out vec3 gEmission;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 albedoColor;
uniform float roughness;
uniform float metalness;
uniform vec3 emission;
uniform bool useTexture;
uniform sampler2D baseTexture;
uniform bool terrainMaterial;
uniform sampler2D terrainGrassTexture;
uniform sampler2D terrainDirtTexture;
uniform sampler2D terrainRockTexture;
uniform float terrainTextureScale;

float Hash21(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float ValueNoise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    float a = Hash21(i);
    float b = Hash21(i + vec2(1.0, 0.0));
    float c = Hash21(i + vec2(0.0, 1.0));
    float d = Hash21(i + vec2(1.0, 1.0));
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float Stripe(float x, float period, float width) {
    float s = abs(fract(x / period) - 0.5);
    return 1.0 - smoothstep(width, width + 0.04, s);
}

vec2 SurfaceUV(vec3 N, vec3 pos) {
    vec3 an = abs(N);
    if (an.x > an.y && an.x > an.z) {
        return pos.zy;
    }
    if (an.z > an.y) {
        return pos.xy;
    }
    return pos.xz;
}

float BrickMask(vec2 uv, float brickW, float brickH) {
    vec2 p = uv / vec2(brickW, brickH);
    float row = floor(p.y);
    float offset = mod(row, 2.0) * 0.5;
    float bx = fract(p.x + offset);
    float by = fract(p.y);
    float mortarX = smoothstep(0.04, 0.08, min(bx, 1.0 - bx));
    float mortarY = smoothstep(0.06, 0.10, min(by, 1.0 - by));
    return clamp(mortarX * mortarY, 0.0, 1.0);
}

float ShingleMask(vec2 uv, float shingleW, float shingleH) {
    vec2 p = uv / vec2(shingleW, shingleH);
    float row = floor(p.y);
    float offset = mod(row, 2.0) * 0.35;
    float sx = fract(p.x + offset);
    float sy = fract(p.y);
    float edgeX = smoothstep(0.05, 0.12, min(sx, 1.0 - sx));
    float edgeY = smoothstep(0.10, 0.18, min(sy, 1.0 - sy));
    return clamp(edgeX * edgeY, 0.0, 1.0);
}

float WindowGridMask(vec2 uv, vec2 repeatCount) {
    vec2 p = uv * repeatCount;
    vec2 cell = fract(p);
    float frameX = smoothstep(0.03, 0.08, min(cell.x, 1.0 - cell.x));
    float frameY = smoothstep(0.03, 0.08, min(cell.y, 1.0 - cell.y));
    return clamp(frameX * frameY, 0.0, 1.0);
}

vec3 ApplyFacadeDetail(vec3 baseColor, vec3 N, vec3 albedo, vec2 uv, vec3 pos) {
    float upness = clamp(N.y, 0.0, 1.0);
    float side = 1.0 - upness;
    float brightness = max(max(albedo.r, albedo.g), albedo.b);
    float saturation = max(max(albedo.r, albedo.g), albedo.b) - min(min(albedo.r, albedo.g), albedo.b);

    vec2 surfaceUV = SurfaceUV(N, pos);
    vec2 worldUV = surfaceUV * 0.45;

    float panel = Stripe(worldUV.x + pos.y * 0.01, 0.20, 0.09);
    float plank = Stripe(worldUV.y + pos.x * 0.01, 0.17, 0.07);
    float grime = ValueNoise(pos.xz * 0.22 + uv * 4.0) * 0.24;
    float edgeWear = smoothstep(0.0, 0.08, uv.x) + smoothstep(0.92, 1.0, uv.x);
    edgeWear += smoothstep(0.0, 0.08, uv.y) + smoothstep(0.92, 1.0, uv.y);
    edgeWear = clamp(edgeWear, 0.0, 1.0);

    vec3 color = baseColor;

    bool isWall = brightness > 0.20 && roughness > 0.40 && upness < 0.80;
    bool isRoof = upness > 0.50 && roughness > 0.45;
    bool isRoad = brightness < 0.36 && roughness > 0.60 && side > 0.30;
    bool isGround = brightness >= 0.20 && brightness <= 0.45 && upness > 0.55 && roughness > 0.45;

    if (isWall) {
        float brick = BrickMask(surfaceUV * 5.5, 0.24, 0.11);
        float panelNoise = ValueNoise(worldUV * 8.0);
        float windowBand = smoothstep(0.18, 0.22, fract(surfaceUV.y * 0.45)) * smoothstep(0.08, 0.14, fract(surfaceUV.x * 0.42));
        float windowGrid = WindowGridMask(surfaceUV, vec2(1.9, 1.2));
        float mullion = smoothstep(0.46, 0.5, abs(fract(surfaceUV.x * 1.9) - 0.5)) + smoothstep(0.46, 0.5, abs(fract(surfaceUV.y * 1.2) - 0.5));
        mullion = clamp(mullion, 0.0, 1.0);

        vec3 sidingA = color * (0.88 - plank * 0.08);
        vec3 sidingB = color * (0.82 - panel * 0.08);
        color = mix(sidingA, sidingB, 0.4 + 0.3 * panelNoise);
        color = mix(color, color * 0.72, 1.0 - brick);
        color *= 1.0 - grime * 0.24;
        color -= edgeWear * 0.08;

        vec3 windowTint = vec3(0.08, 0.10, 0.12) + 0.10 * vec3(panelNoise);
        color = mix(color, windowTint, windowGrid * 0.84);
        color += mullion * 0.05;
        color += windowBand * 0.06;
    } else if (isRoof) {
        float shingle = ShingleMask(surfaceUV * 7.0, 0.28, 0.13);
        float layer = Stripe(worldUV.y, 0.12, 0.05);
        float ridge = smoothstep(0.42, 0.5, abs(uv.x - 0.5));
        float roofNoise = ValueNoise(pos.xz * 0.85 + uv * 5.0);
        color *= 0.80 + shingle * 0.18 + layer * 0.04;
        color -= ridge * 0.10;
        color += roofNoise * 0.06;
    } else if (isRoad) {
        float speckle = ValueNoise(pos.xz * 2.8 + uv * 20.0);
        float crack = smoothstep(0.48, 0.5, abs(fract(uv.x * 7.0 + uv.y * 3.0) - 0.5));
        float lane = smoothstep(0.47, 0.5, abs(fract(surfaceUV.x * 0.28) - 0.5));
        color *= 0.80 + speckle * 0.16;
        color -= crack * 0.04;
        color += vec3(0.03, 0.03, 0.03) * ValueNoise(pos.xz * 4.0);
        color = mix(color, vec3(0.74, 0.67, 0.30), lane * 0.18);
    } else if (isGround) {
        float grassA = ValueNoise(surfaceUV * 3.0 + pos.xz * 0.10);
        float grassB = ValueNoise(surfaceUV * 7.5 + pos.xz * 0.23);
        float blades = Stripe(surfaceUV.x + grassA * 0.2, 0.10, 0.03) * Stripe(surfaceUV.y + grassB * 0.2, 0.18, 0.05);
        color = mix(color * 0.82, color * 1.14, grassA * 0.55);
        color += vec3(0.02, 0.05, 0.02) * blades;
        color += vec3(0.03, 0.04, 0.02) * grassB;
    } else {
        color += vec3(0.01, 0.01, 0.01) * ValueNoise(pos.xz * 0.35);
    }

    if (brightness > 0.55 && saturation < 0.18) {
        color += vec3(0.02, 0.02, 0.02) * ValueNoise(pos.xz * 1.2);
    }

    return color;
}

void main() {
    gPosition = FragPos;
    gNormal = normalize(Normal);

    float tint = 0.92 + TexCoords.y * 0.08;
    vec3 materialColor = albedoColor;
    if (terrainMaterial) {
        vec3 N = normalize(Normal);
        vec2 terrainUV = FragPos.xz * terrainTextureScale;
        float slope = clamp(1.0 - N.y, 0.0, 1.0);
        float altitude = clamp((FragPos.y + 2.0) * 0.06, 0.0, 1.0);
        float grassMask = clamp(1.0 - slope * 1.6 - altitude * 0.35, 0.0, 1.0);
        float dirtMask = clamp(slope * 1.2 + altitude * 0.25, 0.0, 1.0);
        float rockMask = clamp(slope * 1.8 + altitude * 0.50, 0.0, 1.0);
        float total = max(grassMask + dirtMask + rockMask, 0.0001);
        grassMask /= total;
        dirtMask /= total;
        rockMask /= total;

        vec3 grass = texture(terrainGrassTexture, terrainUV).rgb;
        vec3 dirt = texture(terrainDirtTexture, terrainUV * 0.78 + vec2(0.11, 0.07)).rgb;
        vec3 rock = texture(terrainRockTexture, terrainUV * 0.54 + vec2(0.31, 0.24)).rgb;
        float micro = ValueNoise(terrainUV * 8.0 + FragPos.xz * 0.05);
        materialColor = grass * grassMask + dirt * dirtMask + rock * rockMask;
        materialColor = mix(materialColor, materialColor * 0.84, micro * 0.25);
        materialColor += vec3(0.03, 0.04, 0.02) * smoothstep(0.0, 0.7, 1.0 - N.y);
        vec2 gradUV = terrainUV * 1.8;
        float hx = ValueNoise(gradUV + vec2(0.02, 0.0)) - ValueNoise(gradUV - vec2(0.02, 0.0));
        float hz = ValueNoise(gradUV + vec2(0.0, 0.02)) - ValueNoise(gradUV - vec2(0.0, 0.02));
        gNormal = normalize(N + vec3(hx * 0.42, 0.0, hz * 0.42));
        gAlbedoSpec = vec4(materialColor, clamp(0.96 + slope * 0.03, 0.02, 1.0));
        gEmission = emission;
        return;
    }
    if (useTexture) {
        materialColor *= texture(baseTexture, TexCoords).rgb;
    }
    vec3 detailedColor = ApplyFacadeDetail(materialColor * tint, normalize(Normal), materialColor, TexCoords, FragPos);
    gAlbedoSpec = vec4(detailedColor, clamp(roughness + metalness * 0.05, 0.02, 1.0));
    gEmission = emission;
}
