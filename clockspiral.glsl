// Spiral clock shader
// Converted to standalone GLSL for use with temiz.cpp host
// Note: number rendering requires iChannel0 font texture (not available),
// the spiral pattern and animation still work.

uniform float iTime;
uniform vec3  iResolution;

#define smooth_val (2.0/iResolution.y)
#define thick 0.5
#define timeScale 1.0
#define rt (iTime * timeScale)

#define PI 3.14159265359

float rand(vec3 v){
    return fract(cos(dot(v,vec3(13.46543,67.1132,123.546123)))*43758.5453);
}

float rand(vec2 v){
    return fract(sin(dot(v,vec2(5.11543,71.3132)))*43758.5453);
}

// Without iChannel0 texture, return a procedural fake number pattern
float getNum(vec2 uv, int x, float smoothT){
    float n = fract(sin(float(x) * 73.156 + uv.x * 13.7 + uv.y * 37.1) * 43758.5);
    float mask = smoothstep(0.1, 0.4, uv.x) * smoothstep(0.9, 0.6, uv.x)
               * smoothstep(0.1, 0.3, uv.y) * smoothstep(0.9, 0.7, uv.y);
    return step(0.55, n) * mask * 0.8;
}

float getNum2(vec2 uv, int x, float smoothT){
    if(x < 10){
        return getNum(uv, x, smoothT);
    } else{
        if(uv.x < 0.5){
            return getNum(vec2(fract(uv.x * 2.0) * 0.5  +0.22,uv.y),x/10,smoothT);
        }else{
            return getNum(vec2(fract(uv.x * 2.0) * 0.5 +0.28,uv.y),x-10*(x/10),smoothT);
        }
    }
}

float noise(vec2 uv){
    vec2 off = vec2(1,0);
    vec2 fuv = floor(uv);

    float tl = rand(fuv);
    float tr = rand(fuv + off.xy);
    float bl = rand(fuv + off.yx);
    float br = rand(fuv + off.xx);

    vec2 fruv = fract(uv);
    fruv = fruv * fruv * (2.0 - fruv);

    return mix(mix(tl,tr,fruv.x), mix(bl,br,fruv.x),fruv.y);
}

float octNoise(vec2 uv, int octaves){
    float sum = 0.0;
    float f = 1.0;
    for (int i = 0; i < 8; ++i){
        if (i >= octaves) break;
        sum += (noise(uv*f) - 0.5) * 1. /f;
        f *= 2.0;
    }
    return sum;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord.xy - 0.5* iResolution.xy) / iResolution.y;
    uv += octNoise(uv*1.6 + rt*0.1,2) * 0.1 * ( 0.5 + length(uv));
    float a = (atan(uv.x,uv.y) + PI) /PI /2.;
    float l = length(uv);

    float ll = a * 2.0 + pow(l,0.3) * 7.0;
    float trueSm = smooth_val * 16.0 / ll;
    float fll = floor(ll);
    ll = fract(ll);
    float cll = clamp((ll-0.46)*3.0,0.0,1.0);
    float ra = a *24.0 - rt;

    float bgline = smoothstep(trueSm,-trueSm,abs(ll - 0.65)-0.35);
    float bgline2 = smoothstep(trueSm,-trueSm,abs(ll - 0.6)-0.27);
    float bgline3 = smoothstep(trueSm,-trueSm,abs(ll - 0.65)-0.3);
    float bgwhite = smoothstep(trueSm,-trueSm,abs(ll - 0.6)-0.25);
    float bgstrips = smoothstep(trueSm,-trueSm,abs(ll - 0.81)-0.03);
    float bgstrips2 = smoothstep(trueSm,-trueSm,abs(ll - 0.81)-0.018);
    float bgstrips3 = smoothstep(trueSm,-trueSm,abs(fract(ra * 5.0 + 0.5)-0.5)-0.45);
    float bgstrips4 = smoothstep(trueSm,-trueSm,abs(fract(ra + 0.5)-0.5)-0.45);
    float fresnel = smoothstep(0.2,-0.26,abs(ll - 0.8));

    float fra = floor(ll*1.5) +
        fract(a *3.0 - mix(floor(rt) - 3./12.,floor(rt)+ 1.- 3./12.,  pow(fract(rt),2.0)) * 3./12.);

    float arrow =abs(fra-0.5)*2.0 * (ll+0.25) * length(vec2(fra,ll+0.25)-0.5);

    float num = getNum2(vec2(clamp(fract(ra) *2. - 0.5,0.01,0.99),cll),
                        int(mod(floor(ra),12.)) + 1,trueSm);

    float blick = abs(a-0.5)*2.0;
    vec3 finCol = vec3(fract(a*30.0-l*40.0)*0.1);
    finCol = mix(finCol, vec3(blick), bgline);
    finCol = mix(finCol, vec3(1.0-blick), bgline2);
    finCol = mix(finCol, vec3(1.0), bgwhite);
    finCol = mix(finCol, vec3(0.0), bgstrips-bgstrips2 * min(bgstrips3,bgstrips4));
    finCol = mix(finCol, vec3(0.0), num);
    finCol = mix(finCol, vec3(0.0), fresnel +pow((1.0- l),8.0));
    finCol = mix(finCol, vec3(smoothstep(0.003,-0.0,arrow)) * 0.5, smoothstep(0.002,0.0015,arrow));

    fragColor = vec4(finCol,1.0);
}

void main() {
    vec4 fragColor;
    mainImage(fragColor, gl_FragCoord.xy);
    gl_FragColor = fragColor;
}
