uniform float time;
uniform vec2 resolution;

void main()
{
    gl_FragColor = vec4(0.0);
    vec2 p = (gl_FragCoord.xy * 2.0 - resolution) / resolution.y / 0.3, v;
    for(float i, l, f; i++ < 9.0; gl_FragColor += 0.1 / abs(l = dot(p, p) - 5.0 - 2.0 / v.y) * (cos(i / 3.0 + 0.1 / l + vec4(1, 2, 3, 4)) + 1.0))
    {
        for(v = p, f = 0.0; f++ < 9.0; v += sin(ceil(v.yx * f + i * 0.3) + resolution - time / 2.0) / f);
    }
    gl_FragColor = tanh(gl_FragColor);
}
