// twigl.app shader (geeker 300es)
// Converted to standalone GLSL for use with temiz.cpp host

uniform float time;
uniform vec2  resolution;

mat2 rotate2D(float a) {
    float c = cos(a), s = sin(a);
    return mat2(c, -s, s, c);
}

vec3 hsv(float h, float s, float v) {
    return ((clamp(abs(fract(h + vec3(0.0, 2.0/3.0, 1.0/3.0)) * 6.0 - 3.0) - 1.0, 0.0, 1.0) - 1.0) * s + 1.0) * v;
}

#define R rotate2D

void main() {
    vec4 o = vec4(0.0);
    float t = time;
    vec2 r = resolution;
    vec4 FC = gl_FragCoord;

    vec2 p = FC.xy, q, l = (p + p - r) / r.x * 0.4 + vec2(-0.25, 0.05), n;
    float s = 6.0, h = 0.0, i = 0.0, L = dot(l + 1.8, l), e = 129.0;
    for(; i++ < e;) l *= R(4.96), n *= R(4.8 + sin(t) * 0.05) + rotate2D(t) * 0.035,
        h += dot(r / r, sin(q = l * s * i + n) / s * 4.0), n += cos(q),
        s *= 1.05;
    h = 0.4 - h * 0.26 - L;
    o.rgb += 0.5 * h - hsv(0.1, h * 0.5, 0.3);

    gl_FragColor = o;
}
