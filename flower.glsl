// twigl.app shader (geeker 300es)
// Converted to standalone GLSL for use with temiz.cpp host

uniform float time;
uniform vec2  resolution;

mat2 rotate2D(float a) {
    float c = cos(a), s = sin(a);
    return mat2(c, -s, s, c);
}

void main() {
    // Scanline skip: every other row black, saves 50% GPU
    vec2 fc = gl_FragCoord.xy;
    if (mod(floor(fc.y), 2.0) < 1.0) {
        gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec4 o = vec4(0.0);
    float t = time * 0.4;
    vec2 r = resolution;
    vec4 FC = gl_FragCoord;

    vec3 p, q = vec3(-0.1, 0.65, -0.6);
    for(float j = 0.0, i = 0.0, e = 0.0, v = 0.0, u = 0.0; i++ < 70.0; o += 0.013 / exp(3e3 / (v * vec4(9, 5, 4, 4) + e * 4e6))) {
        p = q += vec3((FC.xy - 0.5 * r) / r.y, 1) * e;
        for(j = e = v = 7.0; j++ < 14.0; e = min(e, max(length(p.xz = abs(p.xz * rotate2D(j + sin(1.0 / u + t) / v)) - 0.53) - 0.02 / u, p.y = 1.8 - p.y) / v))
            v /= u = dot(p, p), p /= u + 0.01;
    }

    gl_FragColor = o;
}
