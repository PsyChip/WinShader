// twigl.app shader (geeker 300es)
// Converted to standalone GLSL for use with temiz.cpp host

uniform float time;
uniform vec2  resolution;

void main() {
    vec4 o = vec4(0.0);
    float t = time;
    vec2 r = resolution;
    vec4 FC = gl_FragCoord;

    for(float i = 0.0, z = 0.0, d = 0.0; i++ < 1e2;) {
        vec3 p = z * normalize(FC.rgb * 2.0 - r.xyy), a = normalize(cos(vec3(0, 2, 4) + t / 4.0));
        p.z += 9.0, a = a * dot(a, p) - cross(a, p);
        z += d = 0.01 + 0.3 * abs(max(dot(cos(a), sin(a / 0.6).yzx), length(a) - 7.0) + 1.5 - i / 8e1);
        o += sin(i / 6.0 + z * vec4(0, 1, 2, 0) / 5e1) / d;
    }
    o = tanh(1.0 + o / 2e3);

    gl_FragColor = o;
}
