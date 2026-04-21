// Fireworks over water
// Converted to standalone GLSL for use with temiz.cpp host

uniform float iTime;
uniform vec3  iResolution;

vec2 hash21(float p)
{
    vec3 p3 = fract(vec3(p) * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.xx+p3.yz)*p3.zy);
}

vec3 hash31(float p) {
    vec3 p2 = fract(p * vec3(5.3983, 5.4427, 6.9371));
    p2 += dot(p2.zxy, p2.xyz + vec3(21.5351, 14.3137, 15.3219));
    return fract(vec3(p2.x * p2.y * 95.4337, p2.y * p2.z * 97.597, p2.z * p2.x * 93.8365));
}

vec2 dir(float id){
    vec2 h = hash21(id);
    h.y*=2.*acos(-1.);
    return h.x*vec2(cos(h.y),sin(h.y));
}

#define PARTICLES_MIN 20.
#define PARTICLES_MAX 200.

float bang(vec2 uv, float t,float id){
    float o = 0.;
    if(t<=0.){
        return .04/dot(uv,uv);
    }
    float s = (sqrt(t)+t*exp2(-t/.125)*.8)*10.;
    float brightness = sqrt(1.-t)*.015*(step(.0001,t)*.9+.1);
    float blinkI = exp2(-t/.125);
    float PARTICLES = PARTICLES_MIN+(PARTICLES_MAX-PARTICLES_MIN)*fract(cos(id)*45241.45);
    for(float i=0.;i<PARTICLES_MAX;i++){
        if(i>=PARTICLES) break;
        vec2 d = dir(i+.012*id);
        vec2 p = d*s;
        vec2 h = hash21(5.33345*i+.015*id);
        float blink = mix(cos((t+h.x)*10.*(2.+h.y)+h.x*h.y*10.)*.3+.7,1.,blinkI);
        o+=blink*brightness/dot(uv-p,uv-p);
    }
    return o;
}

const float ExT = 1./4.;

#define duration 2.2

float firework(vec2 uv,float t,float id){
    if(id<1.)return 0.;
    vec2 h = hash21(id*5.645)*2.-1.;
    vec2 offset = vec2(h.x*.1,0.);
    h.y=h.y*.95;
    h.y*=abs(h.y);
    vec2 di = vec2(h.y,sqrt(1.-h.y*h.y));
    float thrust = sqrt(min(t,ExT)/ExT)*25.;
    vec2 p = offset+duration*(di*thrust+vec2(0.,-9.81)*t)*t;
    return sqrt(1.-t)*bang(uv-p,max(0.,(t-ExT)/(1.-ExT)),id);
}

#define NUM_ROCKETS 3.

// Simple hash for dithering (replaces iChannel0 texture)
float hashDither(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * vec3(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (2.*fragCoord-iResolution.xy*vec2(1.,0.))/iResolution.y;
    vec3 col = vec3(0.);

    float time = .75*iTime;
    float t = time/duration;
    uv.y-=.65;
    uv*=35.;
    float m = 1.;
    float d = 0.;

    // Water reflection
    if(uv.y<0.){
        const float h0 = 5.;
        const float dcam = 1000.5;

        float y = uv.y-h0;
        float z = dcam*h0/y;
        d=-40.*uv.y/(h0*dcam);

        float x = uv.x*z/dcam;

        uv+=vec2(sin((x*1.5+z*.75)*.0005-t*1.5),cos((z*2.-x*.5)*.0005-t*2.69))
        *(sin(x*.07+z*.09+sin(x*.2-t)-t*15.)+cos(z*.1-x*(.08+.001*sin(x*.01-t))-t*16.)*.7+cos(z*.01+x*.004-t*10.)*1.7)
        *.15*dcam/z;

        float ndv = -uv.y/sqrt(dcam*dcam+uv.y*uv.y);
        m=mix(1.,.98,pow(1.-ndv,5.));

        uv.y = -uv.y;
    }

    // Sky gradient
    col+=(exp2(-abs(uv.y)*vec3(1.,2.,3.)-.5)+exp2(-abs(uv.y)*vec3(1.,.2,.1)-4.))*.5;

    // Land silhouette
    if(uv.y*1.5<(uv.x-20.)*.01*(-uv.x+90.)+sin(uv.x)*cos(uv.y*1.1)*.75)
        col*=0.;

    // Fireworks
    for(float i = 0.;i<3.;i++){
        float T = 1.+t+i/NUM_ROCKETS;
        float id = floor(T)-i/NUM_ROCKETS;
        vec3 color = hash31(id*.75645);
        color/=max(color.r,max(color.g,color.b));
        col+=firework(uv,fract(T),id)*color;
    }

    fragColor = vec4(m*col,1.0);

    // Procedural dithering (replaces texture-based dither)
    float noise = hashDither(fragCoord + fract(iTime) * 100.0);
    vec4 lcol = clamp(fragColor,0.,1.);
    vec4 gcol = pow(lcol,vec4(1./2.2));
    vec4 gcol_f = floor(gcol*255.)/255.;
    vec4 lcol_f = pow(gcol_f, vec4(2.2));
    vec4 lcol_c = pow(ceil(gcol*255.)/255., vec4(2.2));
    vec4 x = (lcol-lcol_f)/(lcol_c-lcol_f);
    fragColor = gcol_f+step(vec4(noise),x)/255.;
    fragColor.a = 1.0;
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
