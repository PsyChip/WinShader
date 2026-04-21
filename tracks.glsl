// Cole Peterson (Plento)
// Converted to standalone GLSL for use with temiz.cpp host

uniform float iTime;
uniform vec3  iResolution;

#define R iResolution.xy
#define ss(a, b, t) smoothstep(a, b, t)
#define rot(a) mat2(cos(a), -sin(a), sin(a), cos(a))

float hash11(float p){
    p = fract(p * .1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

vec3 ref(vec3 rp, vec3 q){
    return -vec3(abs(rp.x), abs(rp.y), abs(rp.z)) + q*step(vec3(0), q);
}

float rbox( vec3 p, vec3 b, float r ){
  vec3 q = abs(p) - b;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0) - r;
}

float line( vec3 p, vec3 a, vec3 b, float r ){
  vec3 pa = p - a, ba = b - a;
  float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
  return length( pa - ba*h ) - r;
}

float rcyl( vec3 p, float ra, float rb, float h ){
  vec2 d = vec2( length(p.xz)-2.0*ra+rb, abs(p.y) - h );
  return min(max(d.x,d.y),0.0) + length(max(d,0.0)) - rb;
}

float h(float x){
    return cos(x * 3.4) * .18 + cos(x * 1.2)*.1;
}

float hp(float x){
    return (-3.4*sin(x * 3.4) * .18) + (-1.2*sin(x *1.2)*.1);
}

#define tw 0.12
#define cs vec3(.08, .03, .04)
#define sep 1.5

float map(vec3 rp){
    float d = 999.;

    rp.xy*=rot(rp.z*.23);
    vec3 p = rp, p0 = rp;

    float tId = floor(p.z / sep);
    float to = hash11(tId*999.)*5.;

    p.y -= cos(tId*2.) * .2;
    float tX = iTime*.78 * sign(cos(tId*4.))*min(to, .8);

    float trackHeight = h(p.x + to);

    p.zy = mod(p.zy, sep)-sep*.5;
    p.y += trackHeight;

    p.x = mod(p.x, 0.2)-0.2*.5;
    d = min(line(p, vec3(0., 0., -tw), vec3(0., 0., tw), 0.01), d);
    p.xy*=rot(3.14/2.);
    d = min(rcyl(ref(p, vec3(0., 0.0, tw)), .006, .001, .2), d);

    p = p0;
    p.y -= cos(tId*2.) * .2;
    p.x -= tX;

    float cId = floor(p.x);

    float carHeight = h(tX + cId + cs.x*2. + .35 + to);
    float trackDer = hp(tX + cId + cs.x*2. + .35 + to);

    p.x = mod(p.x, 1.)-1.*.5;
    p.zy = mod(p.zy, sep)-sep*.5;

    p.xy *= rot(trackDer);
    p.y += carHeight - cs.y + .11;

    d = min(rbox(p + vec3(0., 0., 0.), cs, 0.05), d);

    return d;
}

void mainImage( out vec4 f, in vec2 u ){
    vec2 uv = vec2(u.xy - 0.5*R.xy)/R.y;
    vec3 rd = normalize(vec3(uv, 1.7));
    vec3 ro = vec3(0., .0, iTime+10.);

    float d = 0.0, t = 0.0;

    for(int i = 0; i < 80; i++){
        d = map(ro + rd*t);

        if(abs(d) < 0.003 || t > 20.) break;

        t += abs(d) * .62;
    }

    vec3 p = ro + rd*t;

    vec3 sky = mix(vec3(0.7, 0.9, 0.9), vec3(0.95, 0.47, 0.36), (uv.y+.15)*2.);
    vec3 col = mix(sky, vec3(0), exp(-t*t*t*0.001));
    col *= ss(.54, .1, abs(uv.y));
    f = vec4(sqrt(clamp(col, .0, 1.)), 1.);
}

void main() {
    vec4 fragColor;
    mainImage(fragColor, gl_FragCoord.xy);
    gl_FragColor = fragColor;
}
