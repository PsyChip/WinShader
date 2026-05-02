// CC0: Trailing the Twinkling Tunnelwisp
// Converted to standalone GLSL for use with shader.cpp host

uniform float iTime;
uniform vec3  iResolution;

float g(vec4 p,float s) {
  return abs(dot(sin(p*=s),cos(p.zxwy))-1.)/s;
}

void mainImage(out vec4 O,vec2 C) {
  float i, d, z, s, T = iTime;
  vec4 o, q, p, U=vec4(2,1,0,3);
  for (
    vec2 r = iResolution.xy
    ; ++i < 50.
    ; z += d + 1.5E-3
    , q = vec4(normalize(vec3(C-.5*r, r.y)) * z, .2)
    , q.z += T/3E1
    , s = q.y + .1
    , q.y = abs(s)
    , p = q
    , p.y -= .11
    , p.xy *= mat2(cos(11.*U.zywz - 2. * p.z ))
    , p.y -= .2
    , d = abs(g(p,8.) - g(p,24.)) / 4.
    , p = 1. + cos(.7 * U + 5. * q.z)
  )
    o += (s > 0. ? 1. : .1) * p.w * p / max(s > 0. ? d : d*d*d, 5E-4)
    ;

  // Rectangular door-shaped glow: width 0.12, height 0.30
  vec2 dq = abs(q.xy) - vec2(0.06, 0.15);
  float doorDist = length(max(dq, 0.0)) + min(max(dq.x, dq.y), 0.0);
  o += (1.4 + sin(T) * sin(1.7 * T) * sin(2.3 * T))
       * 1E2 * U / max(doorDist + 0.02, 0.02);

  O = tanh(o / 1E5);
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
