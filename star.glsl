// twigl.app shader (geeker 300es)
// Converted to standalone GLSL for use with temiz.cpp host

uniform float time;
uniform vec2  resolution;

void main() {
    vec4 o = vec4(0.0);
    float t = time;
    vec2 r = resolution;
    vec4 FC = gl_FragCoord;

    vec3 c, p;
    for(float i = 0.0, z = 0.1, f = 0.0; i++ < 1e2; o += vec4(9, 4, 2, 0) / f / length(c.xy / z)) {
        p = c = z * normalize(FC.rgb * 2.0 - r.xyy);
        for(p.x *= f = 0.6; f++ < 9.0; p += sin(p.yzx * f + 0.5 * z - t / 4.0) / f);
        z += f = 0.03 + 0.1 * max(f = 6.0 - 0.2 * z + min(f = (p + c).y, -f * 0.2), -f * 0.6);
    }
    o = tanh(o * o / 9e8);

    gl_FragColor = o;
}
