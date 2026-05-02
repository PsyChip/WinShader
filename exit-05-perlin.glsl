// exit-05-perlin.glsl — organic fbm-mask sweep (clouds dissolving).
// Standalone fragment shader for shader.cpp host.

uniform sampler2D uShader;
uniform sampler2D uDesktop;
uniform vec2  uResolution;
uniform float uFade;

float hash21(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}
float vnoise2(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f*f*(3.0-2.0*f);
    float a = hash21(i);
    float b = hash21(i + vec2(1.0, 0.0));
    float c = hash21(i + vec2(0.0, 1.0));
    float d = hash21(i + vec2(1.0, 1.0));
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}
float fbm2(vec2 p) {
    float f = 0.0;
    f += 0.5000 * vnoise2(p);          p *= 2.03;
    f += 0.2500 * vnoise2(p);          p *= 2.01;
    f += 0.1250 * vnoise2(p);
    return f;
}

void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;
    float n = fbm2(uv * 4.0);
    float edge = smoothstep(uFade - 0.12, uFade + 0.12, n);
    vec4 a = texture2D(uShader, uv);
    vec4 b = texture2D(uDesktop, vec2(uv.x, 1.0 - uv.y));
    gl_FragColor = mix(b, a, edge);
}
