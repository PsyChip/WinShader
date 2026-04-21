/*
    "Waveform" by @XorDev
*/
uniform float time;
uniform vec2 resolution;

void main() {
    vec2 I = gl_FragCoord.xy;
    float i, d, z, r;
    for(gl_FragColor *= i; i++<9e1;
    gl_FragColor += (cos(z*.5+time*0.1+vec4(0,2,4,3))+1.3)/d/z)
    {
        vec3 p = z * normalize(vec3(I+I,0) - resolution.xyy);
        r = max(-++p, 0.).y;
        p.y += r+r;
        for(d=1.; d<3e1; d+=d)
            p.y += cos(p*d+0.2*time*cos(d)+z).x/d;
        z += d = (.1*r+abs(p.y-1.)/ (1.+r+r+r*r) + max(d=p.z+3.,-d*.1))/8.;
    }
    gl_FragColor = tanh(gl_FragColor/9e2);
}
