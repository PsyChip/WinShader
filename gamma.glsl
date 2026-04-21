uniform float iTime;
uniform vec3  iResolution;
uniform vec4  iMouse;
uniform float iTimeDelta;
uniform int   iFrame;

/*
    "Gamma" by @XorDev
    
    A different take on Firewall.

*/

void mainImage(out vec4 O, vec2 I)
{
    //Raymarch depth
    float z,
    //Step distance
    d,
    //Raymarch iterator
    i,
    //Time for animation
    t = iTime * 0.08;
    //Clear fragColor and raymarch 100 steps
    for(O*=i; i++<4e1; )
    {
        //Sample point (from ray direction)
        vec3 P = z*normalize(vec3(I+I,0)-iResolution.xyx)+.1,
        
        //Polar coordinates and additional transformations
        p = vec3(atan(P.z+=9.,P.x+.1)*2.-.3*P.y, .6*P.y-t, length(P.xz)-4.);
        
        //Apply turbulence and refraction effect
        for(d=0.; d++<9.;)
            p += sin(p.yzx*d+t+.4*i)/d;
            
        //Distance to cylinder and waves with refraction
        z += d = .2*length(vec4(cos(p+P.y*.2)-1., p.z));
        
        //Coloring and brightness
        O += vec4(4,z,2,0)/d/d/z;
    }
    //Tanh tonemap
    O = tanh(O/4e2);
}
void main() {
    vec4 fragColor = vec4(0.0);
    mainImage(fragColor, gl_FragCoord.xy);
    gl_FragColor = fragColor;
}
