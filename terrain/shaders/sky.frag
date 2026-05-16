#version 330 core

out vec4 fragColor;

in vec3 vWorldPos;

uniform vec3 uSunDir;
uniform float uDayFactor;
uniform float uDayTime;

void main() {
    vec3 dir = normalize(vWorldPos);
    float horizonBlend = clamp(dir.y * 2.0, 0.0, 1.0);

    // ── Sky gradient ──
    // Daytime sky: deep blue at zenith, pale blue-white at horizon
    vec3 dayZenith   = vec3(0.10, 0.30, 0.65);
    vec3 dayHorizon  = vec3(0.65, 0.78, 0.90);

    // Nighttime sky: near-black at zenith, very dark navy at horizon
    vec3 nightZenith  = vec3(0.01, 0.01, 0.04);
    vec3 nightHorizon = vec3(0.03, 0.03, 0.08);

    vec3 dayColor   = mix(dayHorizon,   dayZenith,   horizonBlend);
    vec3 nightColor = mix(nightHorizon, nightZenith, horizonBlend);
    vec3 skyColor   = mix(nightColor, dayColor, uDayFactor);

    // ── Sun disk ── only visible during day
    float sunDot = dot(dir, normalize(uSunDir));
    float sunDisk = smoothstep(0.9985, 0.9998, sunDot);
    float sunGlow = smoothstep(0.97, 0.9985, sunDot) * 0.4;
    vec3 sunColor = vec3(1.0, 0.95, 0.7);
    vec3 sunGlowColor = vec3(1.0, 0.75, 0.3);
    skyColor += sunColor * sunDisk * uDayFactor;
    skyColor += sunGlowColor * sunGlow * uDayFactor;

    // ── Moon disk ── only visible at night, opposite direction to sun
    vec3 moonDir = -normalize(uSunDir);
    float moonDot = dot(dir, moonDir);
    float moonDisk = smoothstep(0.9990, 0.9998, moonDot);
    float moonGlow = smoothstep(0.985, 0.9990, moonDot) * 0.15;
    vec3 moonColor = vec3(0.92, 0.95, 1.0);
    float nightFactor = 1.0 - uDayFactor;
    skyColor += moonColor * moonDisk * nightFactor;
    skyColor += moonColor * moonGlow * nightFactor;

    // ── Procedural stars ── only at night, upper hemisphere
    if (dir.y > 0.0) {
        float starX = floor(dir.x * 80.0 + dir.y * 43.0);
        float starY = floor(dir.z * 80.0 + dir.y * 37.0);
        float starRand = fract(sin(starX * 127.1 + starY * 311.7) * 43758.5453);
        float star = step(0.987, starRand); // only ~1.3% of cells are stars
        star *= clamp(dir.y * 3.0, 0.0, 1.0); // fade out near horizon
        skyColor += vec3(0.8, 0.85, 1.0) * star * nightFactor * 1.5;
    }

    // ── Procedural clouds ── layered sine noise
    // Only render clouds near a horizon band, not at zenith
    float cloudAltitude = clamp(1.0 - abs(dir.y - 0.15) * 5.0, 0.0, 1.0);

    // Use world direction to generate pseudo-random cloud pattern
    float cloudX = dir.x / (dir.y + 0.3);
    float cloudZ = dir.z / (dir.y + 0.3);

    // Animate clouds slowly using uDayTime
    float cloudMove = uDayTime * 0.8;

    float cloud = 0.0;
    cloud += sin(cloudX * 3.1 + cloudMove) * 0.5 + 0.5;
    cloud *= sin(cloudZ * 2.7 + cloudMove * 0.7) * 0.5 + 0.5;
    cloud += sin(cloudX * 6.3 + cloudZ * 5.1 + cloudMove * 1.3) * 0.25 + 0.25;
    cloud = clamp(cloud - 0.4, 0.0, 1.0) * 2.5;
    cloud = smoothstep(0.0, 1.0, cloud);
    cloud *= cloudAltitude;

    // Day clouds: bright white. Night clouds: very dark, barely visible
    vec3 dayCloud   = vec3(0.95, 0.96, 1.00);
    vec3 nightCloud = vec3(0.04, 0.04, 0.07);
    vec3 cloudColor = mix(nightCloud, dayCloud, uDayFactor);
    float cloudOpacity = mix(cloud * 0.15, cloud * 0.85, uDayFactor);

    skyColor = mix(skyColor, cloudColor, cloudOpacity);

    // ── Sunrise/sunset orange tint near horizon ──
    float duskFactor = smoothstep(0.0, 0.3, uDayFactor) * (1.0 - smoothstep(0.7, 1.0, uDayFactor));
    vec3 duskColor = vec3(1.0, 0.45, 0.15);
    float duskHorizon = clamp(1.0 - abs(dir.y) * 6.0, 0.0, 1.0);
    skyColor += duskColor * duskFactor * duskHorizon * 0.5;

    fragColor = vec4(skyColor, 1.0);
}
