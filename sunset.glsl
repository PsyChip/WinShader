// Sunset over the ocean with shark fin
// Copyright (c) srvstr 2025, MIT License
// Converted to standalone GLSL for use with temiz.cpp host

uniform float iTime;
uniform vec3  iResolution;

float cnoise(in vec2 uv)
{
    const mat2 r = mat2(-0.1288, -0.9917, 0.9917, -0.1288);
    vec2 s0 = cos(uv);
    vec2 s1 = cos(uv * 2.5 * r);
    vec2 s2 = cos(uv * 4.0 * r * r);
    vec2 s = s0 * s1 * s2;
    return (s.x + s.y) * 0.25 + 0.5;
}

#define S(x) (smoothstep(0.0, 1.0, (x)))

float fin(in vec2 uv)
{
    uv.x += S(S(S(abs(1.0 - 2.0 * fract(iTime * 0.02))))) - 0.5;
    uv *= vec2(sign(abs(1.0 - 2.0 * fract(iTime * 0.02 + 0.25)) - 0.5), 1) * 3.5;
    float d = smoothstep(1.5/iResolution.y, 0.0,
                         uv.y + 2.0 * uv.x * uv.x
                         + max(0.0, -(uv.y + 0.3) * (uv.y + 0.3) + uv.x * 3.0) * 5.0);
    return 1.0 - d * smoothstep(-0.4, -0.4+3.0/iResolution.y,
                                uv.y + sin(iTime * 4.0 - uv.x * 16.0) / 100.0);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 uv = (fragCoord - 0.5 * iResolution.xy) / iResolution.y * 2.0;

    float dy = (smoothstep(0.0, -1.0, uv.y) * 40.0 + 1.5) / iResolution.y;

    // Wave displacement — unrolled to avoid array constructor issues on older GLSL
    float avg = 0.0;
    avg += cnoise(uv * vec2(0.5, 20.0) + iTime) * 8.0 - 4.0;
    avg += cnoise(uv * vec2(2.5, 60.0) + iTime) * 4.0 - 2.0;
    avg += cnoise(uv * vec2(5.0, 80.0) + iTime) * 2.0 - 1.0;
    avg += cnoise(uv * vec2(10.0, 20.0) + iTime) * 2.0 - 1.0;
    avg /= 4.0;

    vec2 st = vec2(uv.x,
                   uv.y + clamp(avg * smoothstep(0.1, -1.0, uv.y), -0.1, 0.1));

    fragColor.rgb = mix(vec3(0.85, 0.55, 0),
                        vec3(0.90, 0.40, 0),
                        sqrt(abs(st.y * st.y * st.y)) * 28.0) * fin(uv)
                        * smoothstep(0.25 + dy, 0.25, length(st))
                        + smoothstep(2.0, 0.5, length(uv)) * 0.1;
    fragColor.a = 1.0;
}

void main() {
    vec4 fragColor;
    mainImage(fragColor, gl_FragCoord.xy);
    gl_FragColor = fragColor;
}
