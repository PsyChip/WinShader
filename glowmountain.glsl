// CC0: Glowing mountain lines
// Playing around with @XorDev's dot noise function
// Converted to standalone GLSL for use with temiz.cpp host

uniform float iTime;
uniform vec3  iResolution;

void mainImage(out vec4 O, vec2 C) {
  float slowTime = iTime * 0.4;
  vec3
    p
  , Z=iResolution
  , T=vec3(0,0,slowTime)
  , I=normalize(vec3(C, Z.y) -.5*Z)
  ;
  vec4 o;
  Z=fract(-T)/I.z;
  for(
    int i,j
  ; ++i<60
  ; Z+=.5/abs(I)
  ) {
    float
      a=.6
    , d
    , z=.2*Z[j^=2]
    ;
    p=z*I+.2*T;
    d=p.y;
    for(
      O=1.+sin(.5*p.x+p.z+vec4(2,7,0,2))
    ; a>.1
    ; p.xy*=mat2(6,8,-8,6)/8.
    )
      d+=a+a*dot(sin(p), cos(p=p.yzx*1.62))
    , a*=.5
    ;
   a=cosh(8.-z);
   o+=
      O.w
    / (abs(d)*5e2+.3/I[j]/I[j]+5./(8.<z?1.:a))
    / (8.>z?1.:a)
    * O
    ;
  }
  O=tanh(o);
}

void main() {
    // Scanline skip: every other row black, saves 50% GPU
    vec2 fc = gl_FragCoord.xy;
    if (mod(floor(fc.y), 2.0) < 1.0) {
        gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    vec4 fragColor;
    mainImage(fragColor, fc);
    gl_FragColor = fragColor;
}
