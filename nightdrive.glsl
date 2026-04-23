// Night drive — city lights, car lines, mountains and stars
// Converted to standalone GLSL for use with temiz.cpp host

uniform float iTime;
uniform vec3  iResolution;

#define TIMESCALE 0.25
#define SKYCOLORA vec3(.05,.05,.05)
#define SKYCOLORB vec3(.15,.15,.15)
#define STARBRIGHTNESS .5
#define STARSIZE 1.5
#define CITYLIGHTSFARSIZE .5
#define CITYLIGHTSFARBRIGHTNESS .8
#define CITYLIGHTSCLOSESIZE 2.0
#define CITYLIGHTSCLOSEBRIGHTNESS 1.0
#define CARSPACING 2.0
#define CARLENGTH .2

float uTime;

float hash(float n) { return fract(sin(n)*43758.5453123); }
vec3 hash3(float n) { return fract(sin(vec3(n,n+1.0,n+2.0))*vec3(43758.5453123,22578.1459123,19642.3490423)); }

float circle(vec2 p, vec2 c, float r) { return (length(p-c)<r) ? 1.0 : 0.0; }

vec2 closestpointHorizontal(vec2 p, vec2 p0, vec2 v) {
    float t1 = (p.x-p0.x)/v.x;
    return p0+v*t1;
}

float linedistA(vec2 p, vec2 p0, vec2 v, float len){
    vec2 r = p-p0;
    float t1 = clamp(dot(r,v), 0.0, len);
    vec2 d = p0+v*t1 - p;
    return dot(d,d);
}

vec3 sparkles(vec2 coord) {
    float h = hash(coord.x*.1+coord.y*1.345);
    float i = 0.0;
    if(h>.995) i = .5+.5*sin(6.28*hash(coord.x*1.2568+coord.y*.1578)+uTime);
    return vec3(i);
}

vec3 carlines(vec2 coord, vec2 p0, vec2 v0, bool away) {
    vec2 linepoint = closestpointHorizontal(coord,p0,v0);
    float d = length(linepoint-coord);
    float threshold = max(0.0,-linepoint.y)*.02;
    float intensity = 0.0;
    if(d<threshold) intensity = 1.0;
    float z = 1.0/(-threshold);
    if(away) z+=uTime; else z-=uTime;
    float interval = mod(z/CARLENGTH,CARSPACING);
    if(away) interval = 1.0-interval;
    interval = clamp(interval, 0.0, 1.0);
    intensity *= interval;
    if(away) return vec3(intensity,0,0);
    else return vec3(intensity);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 ouv = fragCoord.xy / iResolution.xy;
    vec3 uScreen = iResolution;
    vec3 color = vec3(0);
    vec2 ocoord = (ouv-vec2(.5))*uScreen.xy;
    vec2 uv = ouv;
    vec2 coord = ocoord;
    uTime = iTime*TIMESCALE;

    // Car lines
    coord.x += .05*uScreen.x*sin(uv.y*20.0);
    vec2 p1 = vec2(uScreen.x/2.5,0.0);
    vec2 p0 = vec2(0,-uScreen.y/2.0);
    color += carlines(coord, p0, normalize(p1-p0), false);
    p0.x+=uScreen.x*.05;
    color += carlines(coord, p0, normalize(p1-p0), false);
    p0.x+=uScreen.x*.1;
    color += carlines(coord, p0, normalize(p1-p0), true);
    p0.x+=uScreen.x*.05;
    color += carlines(coord, p0, normalize(p1-p0), true);

    coord.x = ocoord.x;
    uv.y += .01*sin(uv.x*20.0);

    // Small city lights
    {
        vec3 cl = sparkles(floor(coord/CITYLIGHTSFARSIZE));
        cl *= CITYLIGHTSFARBRIGHTNESS;
        cl *= clamp(1.0-pow((uv.y-.5)/.03,2.0),0.0,1.0);
        cl *= 1.0-coord.x/uScreen.x*2.0;
        color += cl;
    }
    // Larger city lights
    {
        vec3 cl = sparkles(floor(coord/CITYLIGHTSCLOSESIZE));
        cl *= CITYLIGHTSCLOSEBRIGHTNESS;
        cl *= clamp(1.0-pow((uv.y-.35)/.08,2.0),0.0,1.0);
        cl *= (.4-uv.x)*5.0;
        color += cl;
    }

    uv.y = ouv.y;

    // Mountains and sky
    uv.y += -.03*cos(uv.x*6.28);
    if((uv.y>abs(mod(uv.x,.3)-.15)*.4+.65)
        && (uv.y>abs(mod(uv.x,.11)-.055)*.4+.65+.025)){
        float skymix = (1.0-uv.y)/.35+hash(uv.x+uv.y)*.05;
        color = mix(SKYCOLORA,SKYCOLORB, skymix);
        color += sparkles(floor(coord/STARSIZE))*mix(STARBRIGHTNESS, .0, skymix);
    }

    fragColor = vec4(color, 1.0);
}

void main() {
    vec4 fragColor;
    mainImage(fragColor, gl_FragCoord.xy);
    gl_FragColor = fragColor;
}
