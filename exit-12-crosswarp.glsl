// exit-12-crosswarp.glsl — shader collapses inward while desktop expands outward.
// Standalone fragment shader for shader.cpp host.
uniform sampler2D uShader;
uniform sampler2D uDesktop;
uniform vec2  uResolution;
uniform float uFade;

void main() {
    vec2 uv = gl_FragCoord.xy / uResolution;
    float x = smoothstep(0.0, 1.0, uFade * 2.0 + uv.x - 1.0);
    vec2 uvA = (uv - 0.5) * (1.0 - x) + 0.5;
    vec2 uvB = (uv - 0.5) * x + 0.5;
    vec4 a = texture2D(uShader, clamp(uvA, vec2(0.001), vec2(0.999)));
    vec2 sb = clamp(uvB, vec2(0.001), vec2(0.999));
    vec4 b = texture2D(uDesktop, vec2(sb.x, 1.0 - sb.y));
    gl_FragColor = mix(a, b, x);
}
