// Copyright Inigo Quilez, 2014 - https://iquilezles.org/
// I am the sole copyright owner of this Work. You cannot
// host, display, distribute or share this Work neither as
// is or altered, in any form including physical and
// digital. You cannot use this Work in any commercial or
// non-commercial product, website or project. You cannot
// sell this Work and you cannot mint an NFTs of it. You
// cannot use this Work to train AI models. I share this
// Work for educational purposes, you can link to it as
// an URL, proper attribution and unmodified screenshot,
// as part of your educational material. If these
// conditions are too restrictive please contact me.

// NOTE: requires iChannel0 (noise), iChannel1 (grass), iChannel3 (rock) textures

uniform float iTime;
uniform vec3  iResolution;

#define USE_BOUND_PLANE

const mat2 m2 = mat2(1.6,-1.2,1.2,1.6);

float noi( in vec2 p )
{
    return 0.5*(cos(6.2831*p.x) + cos(6.2831*p.y));
}

float soilHash( vec2 p )
{
    float h = dot(p, vec2(127.1, 311.7));
    return fract(sin(h) * 43758.5453123);
}

float soilNoise( vec2 p )
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(soilHash(i), soilHash(i + vec2(1.0, 0.0)), u.x),
               mix(soilHash(i + vec2(0.0, 1.0)), soilHash(i + vec2(1.0, 1.0)), u.x), u.y);
}

float soilFBM( vec2 p )
{
    float f = 0.0, s = 0.5;
    for( int i = 0; i < 6; i++ )
    {
        f += s * soilNoise( p );
        p = m2 * p;
        s *= 0.5;
    }
    return f;
}

vec3 soilTexture( vec3 pos, vec3 nor )
{
    vec3 dirt  = vec3(0.10, 0.07, 0.04);
    vec3 clay  = vec3(0.14, 0.09, 0.05);
    vec3 sand  = vec3(0.18, 0.15, 0.08);
    vec3 rock  = vec3(0.06, 0.06, 0.06);
    vec3 grass = vec3(0.05, 0.09, 0.03);

    float n1 = soilFBM( pos.xz * 0.02 );
    float n2 = soilFBM( pos.xz * 0.08 );
    float n3 = soilFBM( pos.xz * 0.3 );
    float n4 = soilFBM( pos.xz * 1.5 );

    vec3 col = mix( dirt, clay, smoothstep(0.2, 0.6, n1) );
    col = mix( col, sand, smoothstep(0.3, 0.7, n2) * 0.4 );

    // grass on flat areas
    float slope = 1.0 - nor.y;
    col = mix( col, grass, smoothstep(0.6, 0.4, slope) * smoothstep(0.3, 0.5, n2) * 0.5 );

    // fine grain
    col *= 0.8 + 0.4 * n3;
    // micro detail
    col *= 0.9 + 0.2 * n4;

    // rocky on steep slopes
    col = mix( col, rock, smoothstep(0.5, 0.8, slope) );

    // height variation
    float hv = smoothstep(20.0, 80.0, pos.y);
    col = mix( col, rock * 1.2, hv * 0.5 );

    return col;
}

float terrainLow( vec2 p )
{
    p *= 0.0013;
    float s = 1.0, t = 0.0;
    for( int i=0; i<2; i++ )
    {
        t += s*noi( p );
        s *= 0.5 + 0.1*t;
        p = 0.97*m2*p + (t-0.5)*0.2;
    }
    return t*55.0;
}

float terrainMed( vec2 p )
{
    p *= 0.0013;
    float s = 1.0, t = 0.0;
    for( int i=0; i<6; i++ )
    {
        t += s*noi( p );
        s *= 0.5 + 0.1*t;
        p = 0.97*m2*p + (t-0.5)*0.2;
    }
    return t*55.0;
}

float terrainHigh( vec2 p )
{
    vec2 q = p;
    p *= 0.0013;
    float s = 1.0, t = 0.0;
    for( int i=0; i<7; i++ )
    {
        t += s*noi( p );
        s *= 0.5 + 0.1*t;
        p = 0.97*m2*p + (t-0.5)*0.2;
    }
    return t*55.0;
}

float tubes( vec3 pos, float time )
{
    float sep = 400.0;
    pos.z -= sep*0.025*noi( 0.005*pos.xz*vec2(0.5,1.5) );
    pos.x -= sep*0.050*noi( 0.005*pos.zy*vec2(0.5,1.5) );
    vec3 qos = mod( pos + sep*0.5, sep ) - sep*0.5;
    qos.y = pos.y - 70.0;
    qos.x += sep*0.3*cos( 0.01*pos.z);
    qos.y += sep*0.1*cos( 0.01*pos.x );
    float sph = length( qos.xy ) - sep*0.012;
    sph -= (1.0-0.8*smoothstep(-10.0,0.0,qos.y))*sep*0.003*noi( 0.15*pos.xy*vec2(0.2,1.0) );
    return sph;
}

vec2 map( in vec3 pos, float time )
{
    float m = 0.0;
    float h = pos.y - terrainMed(pos.xz);
    float sph = tubes( pos, time );
    float k = 60.0;
    float w = clamp( 0.5 + 0.5*(h-sph)/k, 0.0, 1.0 );
    h = mix( h, sph, w ) - k*w*(1.0-w);
    m = mix( m, 1.0, w ) - 1.0*w*(1.0-w);
    m = clamp(m,0.0,1.0);
    return vec2( h, m );
}

float mapH( in vec3 pos, in float time )
{
    float h = pos.y - terrainHigh(pos.xz);
    float sph = tubes( pos, time );
    float k = 60.0;
    float w = clamp( 0.5 + 0.5*(h-sph)/k, 0.0, 1.0 );
    h = mix( h, sph, w ) - k*w*(1.0-w);
    return h;
}

vec2 interesct( in vec3 ro, in vec3 rd, in float tmin, in float tmax, in float time )
{
    float t = tmin, m = 0.0;
    for( int i=0; i<160; i++ )
    {
        vec3 pos = ro + t*rd;
        vec2 res = map( pos, time );
        m = res.y;
        if( res.x<(0.001*t) || t>tmax ) break;
        t += res.x * 0.5;
    }
    return vec2( t, m );
}

float calcShadow(in vec3 ro, in vec3 rd )
{
    float h1 = terrainMed( ro.xz );
    float h2 = terrainLow( ro.xz );
    float d1 = 10.0, d2 = 80.0, d3 = 200.0;
    float s1 = clamp( 1.0*(h1 + rd.y*d1 - terrainMed(ro.xz + d1*rd.xz)), 0.0, 1.0 );
    float s2 = clamp( 0.5*(h1 + rd.y*d2 - terrainMed(ro.xz + d2*rd.xz)), 0.0, 1.0 );
    float s3 = clamp( 0.2*(h2 + rd.y*d3 - terrainLow(ro.xz + d3*rd.xz)), 0.0, 1.0 );
    return min(min(s1,s2),s3);
}

vec3 calcNormalHigh( in vec3 pos, float t, in float time )
{
    vec2 e = vec2(1.0,-1.0)*0.001*t;
    return normalize( e.xyy*mapH( pos + e.xyy, time ) +
                      e.yyx*mapH( pos + e.yyx, time ) +
                      e.yxy*mapH( pos + e.yxy, time ) +
                      e.xxx*mapH( pos + e.xxx, time ) );
}

vec3 calcNormalMed( in vec3 pos, float t )
{
    float e = 0.005*t;
    vec2  eps = vec2(e,0.0);
    float h = terrainMed( pos.xz );
    return normalize(vec3( terrainMed(pos.xz-eps.xy)-h, e, terrainMed(pos.xz-eps.yx)-h ));
}

vec3 camPath( float time )
{
    vec2 p = 1100.0*vec2( cos(0.0+0.23*time), cos(1.5+0.205*time) );
    return vec3( p.x, 0.0, p.y );
}

vec3 dome( in vec3 rd, in vec3 light1 )
{
    float sda = clamp(0.5 + 0.5*dot(rd,light1),0.0,1.0);
    float cho = max(rd.y,0.0);
    vec3 bgcol = mix( mix(vec3(0.00,0.40,0.60)*0.7,
                          vec3(0.80,0.70,0.20), pow(1.0-cho,3.0 + 4.0-4.0*sda)),
                          vec3(0.43+0.2*sda,0.4-0.1*sda,0.4-0.25*sda), pow(1.0-cho,10.0+ 8.0-8.0*sda) );
    bgcol *= 0.8 + 0.2*sda;
    return bgcol*0.75;
}

mat3 setCamera( in vec3 ro, in vec3 ta, float cr )
{
    vec3 cw = normalize(ta-ro);
    vec3 cp = vec3(sin(cr), cos(cr),0.0);
    vec3 cu = normalize( cross(cw,cp) );
    vec3 cv = normalize( cross(cu,cw) );
    return mat3( cu, cv, cw );
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 xy = -1.0 + 2.0*fragCoord.xy/iResolution.xy;
    vec2 sp = xy*vec2(iResolution.x/iResolution.y,1.0);

    float camid = floor(iTime/9.0);
    float time = 16.5 + iTime*0.05 + 72.1*camid + 19.0*max(0.0,camid-1.0);

    float cr = 0.18*sin(-0.1*time);
    vec3  ro = camPath( time );
    vec3  ta = camPath( time + 3.0 );
    ro.y = terrainLow( ro.xz ) + 60.0 + 30.0*sin(1.0*(time-14.4));
    ta.y = ro.y - 200.0;
    mat3 cam = setCamera( ro, ta, cr );

    vec3 light1 = normalize( vec3(-0.8,0.2,0.5) );
    vec3 rd = cam * normalize(vec3(sp.xy,1.5));
    vec3 bgcol = dome( rd, light1 );

    float tmin = 10.0, tmax = 4500.0;

#ifdef USE_BOUND_PLANE
    float maxh = 130.0;
    float tp = (maxh-ro.y)/rd.y;
    if( tp>0.0 )
    {
        if( ro.y>maxh ) tmin = max( tmin, tp );
        else            tmax = min( tmax, tp );
    }
#endif

    float sundotc = clamp( dot(rd,light1), 0.0, 1.0 );
    vec3  col = bgcol;

    vec2 res = interesct( ro, rd, tmin, tmax, time );
    if( res.x>tmax )
    {
        col += 0.2*0.12*vec3(1.0,0.5,0.1)*pow( sundotc,5.0 );
        col += 0.2*0.12*vec3(1.0,0.6,0.1)*pow( sundotc,64.0 );
        col += 0.2*0.12*vec3(2.0,0.4,0.1)*pow( sundotc,512.0 );
        col += 0.2*0.2*vec3(1.5,0.7,0.4)*pow( sundotc, 4.0 );
    }
    else
    {
        float t = res.x;
        vec3 pos = ro + t*rd;
        vec3 nor = calcNormalHigh( pos, t, time );
        vec3 sor = calcNormalMed( pos, t );
        vec3 ref = reflect( rd, nor );

        col = soilTexture( pos, nor );
        vec3 col2 = vec3(1.0,0.2,0.1)*0.01;
        col = mix( col, col2, 0.5*res.y );

        vec3 ptnor = nor;

        float amb = clamp( nor.y,0.0,1.0);
        float dif = clamp( dot( light1, nor ), 0.0, 1.0 );
        float bac = clamp( dot( normalize( vec3(-light1.x, 0.0, light1.z ) ), nor ), 0.0, 1.0 );
        float sha = mix( calcShadow( pos, light1 ), 1.0, res.y );
        float spe = pow( clamp( dot(ref,light1), 0.0, 1.0 ), 4.0 ) * dif;

        vec3 lin = vec3(0.0);
        lin += dif*vec3(11.0,6.00,3.00)*vec3( sha, sha*sha*0.5+0.5*sha, sha*sha*0.8+0.2*sha );
        lin += amb*vec3(0.25,0.30,0.40);
        lin += bac*vec3(0.35,0.40,0.50);
        lin += spe*vec3(4.00,4.00,4.00)*res.y;
        col *= lin;

        col = mix( col, 0.25*mix(vec3(0.4,0.75,1.0),vec3(0.3,0.3,0.3), sundotc*sundotc), 1.0-exp(-0.0000008*t*t) );
        col += 0.15*vec3(1.0,0.8,0.3)*pow( sundotc, 8.0 )*(1.0-exp(-0.003*t));
        col = mix( col, bgcol, 1.0-exp(-0.00000004*t*t) );
    }

    col = pow( col, vec3(0.45) );
    col = col*1.4*vec3(1.0,1.0,1.02) + vec3(0.0,0.0,0.11);
    col = clamp(col,0.0,1.0);
    col = col*col*(3.0-2.0*col);
    col = mix( col, vec3(dot(col,vec3(0.333))), 0.25 );
    col *= 0.5 + 0.5*pow( (xy.x+1.0)*(xy.y+1.0)*(xy.x-1.0)*(xy.y-1.0), 0.1 );
    col *= smoothstep( 0.0, 0.1, 2.0*abs(fract(0.5+iTime/9.0)-0.5) );

    fragColor = vec4( col, 1.0 );
}

void main() {
    vec4 fragColor;
    mainImage(fragColor, gl_FragCoord.xy);
    gl_FragColor = fragColor;
}
