// Glass cube with refraction and bilinear patch surfaces
// Created by Danil (2021+) https://github.com/danilw
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// Converted to standalone GLSL for use with temiz.cpp host

uniform float iTime;
uniform vec3  iResolution;
uniform vec4  iMouse;

#define iTimeSlow (iTime * 0.25)

#define NO_ALPHA
#define ROTATION_SPEED 0.8999

#define tshift 53.

#define FDIST 0.7
#define PI 3.1415926
#define BOXDIMS vec3(0.75, 0.75, 1.25)
#define IOR 1.33
#define ANGLE_loops 0

mat3 rotx(float a){float s=sin(a);float c=cos(a);return mat3(1,0,0,0,c,s,0,-s,c);}
mat3 roty(float a){float s=sin(a);float c=cos(a);return mat3(c,0,s,0,1,0,-s,0,c);}
mat3 rotz(float a){float s=sin(a);float c=cos(a);return mat3(c,s,0,-s,c,0,0,0,1);}

vec3 fcos(vec3 x){
    vec3 w=fwidth(x);
    float lw=length(w);
    if(lw==0.||isnan(lw)||isinf(lw)){vec3 tc=vec3(0.);for(int i=0;i<8;i++)tc+=cos(x+x*float(i-4)*(0.01*400./iResolution.y));return tc/8.;}
    return cos(x)*smoothstep(PI*2.0,0.0,w);
}

vec3 getColor(vec3 p){
    p=abs(p);
    p*=1.25;
    p=0.5*p/dot(p,p);
    float t=0.13*length(p);
    vec3 col=vec3(0.3,0.4,0.5);
    col+=0.12*fcos(6.28318*t*1.0+vec3(0.0,0.8,1.1));
    col+=0.11*fcos(6.28318*t*3.1+vec3(0.3,0.4,0.1));
    col+=0.10*fcos(6.28318*t*5.1+vec3(0.1,0.7,1.1));
    col+=0.10*fcos(6.28318*t*17.1+vec3(0.2,0.6,0.7));
    col+=0.10*fcos(6.28318*t*31.1+vec3(0.1,0.6,0.7));
    col+=0.10*fcos(6.28318*t*65.1+vec3(0.0,0.5,0.8));
    col+=0.10*fcos(6.28318*t*115.1+vec3(0.1,0.4,0.7));
    col+=0.10*fcos(6.28318*t*265.1+vec3(1.1,1.4,2.7));
    return clamp(col,0.,1.);
}

void calcColor(vec3 ro,vec3 rd,vec3 nor,float d,float len,int idx,bool si,float td,out vec4 colx,out vec4 colsi){
    vec3 pos=ro+rd*d;
    float a=1.-smoothstep(len-0.075,len+0.00001,length(pos));
    vec3 col=getColor(pos);
    colx=vec4(col,a);
    if(si){
        pos=ro+rd*td;
        float ta=1.-smoothstep(len-0.075,len+0.00001,length(pos));
        col=getColor(pos);
        colsi=vec4(col,ta);
    }
}

bool iBilinearPatch(vec3 ro,vec3 rd,vec4 ps,vec4 ph,float sz,out float t,out vec3 norm,out bool si,out float tsi,out vec3 normsi,out float fade,out float fadesi){
    vec3 va=vec3(0.,0.,ph.x+ph.w-ph.y-ph.z);
    vec3 vb=vec3(0.,ps.w-ps.y,ph.z-ph.x);
    vec3 vc=vec3(ps.z-ps.x,0.,ph.y-ph.x);
    vec3 vd=vec3(ps.xy,ph.x);
    t=-1.;tsi=-1.;si=false;fade=1.;fadesi=1.;
    norm=vec3(0.,1.,0.);normsi=vec3(0.,1.,0.);
    float tmp=1./(vb.y*vc.x);
    float d_=va.z*tmp;
    float g=(vc.z*vb.y-vd.y*va.z)*tmp;
    float h=(vb.z*vc.x-va.z*vd.x)*tmp;
    float j=(vd.x*vd.y*va.z+vd.z*vb.y*vc.x)*tmp-(vd.y*vb.z*vc.x+vd.x*vc.z*vb.y)*tmp;
    float p=d_*rd.x*rd.z;
    float q=d_*(ro.x*rd.z+ro.z*rd.x)+g*rd.x+h*rd.z-rd.y;
    float r=d_*ro.x*ro.z+g*ro.x+h*ro.z-ro.y+j;
    if(abs(p)<0.000001){
        float tt=-r/q;
        if(tt<=0.)return false;
        t=tt;
        vec3 pos=ro+t*rd;
        if(length(pos)>sz)return false;
        vec3 grad=vec3(d_*pos.z+g,-1.,d_*pos.x+h);
        norm=-normalize(grad);
        return true;
    }else{
        float sq=q*q-4.*p*r;
        if(sq<0.)return false;
        float s=sqrt(sq);
        float t0=(-q+s)/(2.*p);
        float t1=(-q-s)/(2.*p);
        float tt1=min(t0<0.?t1:t0,t1<0.?t0:t1);
        float tt2=max(t0>0.?t1:t0,t1>0.?t0:t1);
        float tt0=tt1;
        if(tt0<=0.)return false;
        vec3 pos=ro+tt0*rd;
        bool ru=step(sz,length(pos))>0.5;
        if(ru){tt0=tt2;pos=ro+tt0*rd;}
        if(tt0<=0.)return false;
        if(step(sz,length(pos))>0.5)return false;
        if(tt2>0.&&!ru&&!(step(sz,length(ro+tt2*rd))>0.5)){
            si=true;fadesi=s;tsi=tt2;
            vec3 tpos=ro+tsi*rd;
            vec3 tgrad=vec3(d_*tpos.z+g,-1.,d_*tpos.x+h);
            normsi=-normalize(tgrad);
        }
        fade=s;t=tt0;
        vec3 grad=vec3(d_*pos.z+g,-1.,d_*pos.x+h);
        norm=-normalize(grad);
        return true;
    }
}

float dot2(vec3 v){return dot(v,v);}

float segShadow(vec3 ro,vec3 rd,vec3 pa,float sh){
    float dm=dot(rd.yz,rd.yz);
    float k1=(ro.x-pa.x)*dm;float k2=(ro.x+pa.x)*dm;
    vec2 k5=(ro.yz+pa.yz)*dm;float k3=dot(ro.yz+pa.yz,rd.yz);
    vec2 k4=(pa.yz+pa.yz)*rd.yz;vec2 k6=(pa.yz+pa.yz)*dm;
    for(int i=0;i<4;i++){
        vec2 s=vec2(i&1,i>>1);
        float t=dot(s,k4)-k3;
        if(t>0.)sh=min(sh,dot2(vec3(clamp(-rd.x*t,k1,k2),k5-k6*s)+rd*t)/(t*t));
    }
    return sh;
}

float boxSoftShadow(vec3 ro,vec3 rd,vec3 rad,float sk){
    rd+=0.0001*(1.-abs(sign(rd)));
    vec3 m=1./rd;vec3 n=m*ro;vec3 k=abs(m)*rad;
    vec3 t1=-n-k;vec3 t2=-n+k;
    float tN=max(max(t1.x,t1.y),t1.z);float tF=min(min(t2.x,t2.y),t2.z);
    if(tN<tF&&tF>0.)return 0.;
    float sh=1.;
    sh=segShadow(ro.xyz,rd.xyz,rad.xyz,sh);
    sh=segShadow(ro.yzx,rd.yzx,rad.yzx,sh);
    sh=segShadow(ro.zxy,rd.zxy,rad.zxy,sh);
    sh=clamp(sk*sqrt(sh),0.,1.);
    return sh*sh*(3.-2.*sh);
}

float box(vec3 ro,vec3 rd,vec3 r,out vec3 nn,bool entering){
    rd+=0.0001*(1.-abs(sign(rd)));
    vec3 dr=1./rd;vec3 n=ro*dr;vec3 k=r*abs(dr);
    vec3 pin=-k-n;vec3 pout=k-n;
    float tin=max(pin.x,max(pin.y,pin.z));float tout=min(pout.x,min(pout.y,pout.z));
    if(tin>tout)return-1.;
    if(entering)nn=-sign(rd)*step(pin.zxy,pin.xyz)*step(pin.yzx,pin.xyz);
    else nn=sign(rd)*step(pout.xyz,pout.zxy)*step(pout.xyz,pout.yzx);
    return entering?tin:tout;
}

vec3 bgcol(vec3 rd){return mix(vec3(0.01),vec3(0.336,0.458,.668),1.-pow(abs(rd.z+0.25),1.3));}

vec3 background(vec3 ro,vec3 rd,vec3 l_dir,out float alpha){
    float t=(-BOXDIMS.z-ro.z)/rd.z;
    alpha=0.;
    vec3 bgc=bgcol(rd);
    if(t<0.)return bgc;
    vec2 uv=ro.xy+t*rd.xy;
    float shad=boxSoftShadow(ro+t*rd,normalize(l_dir+vec3(0.,0.,1.))*rotz(PI*0.65),BOXDIMS,1.5);
    float aofac=smoothstep(-0.95,.75,length(abs(uv)-min(abs(uv),vec2(0.45))));
    aofac=min(aofac,smoothstep(-0.65,1.,shad));
    float lght=max(dot(normalize(ro+t*rd+vec3(0.,0.,-5.)),normalize(l_dir-vec3(0.,0.,1.))*rotz(PI*0.65)),0.);
    vec3 col=mix(vec3(0.4),vec3(.71,.772,0.895),lght*lght*aofac+0.05)*aofac;
    alpha=1.-smoothstep(7.,10.,length(uv));
    return mix(col*length(col)*0.8,bgc,smoothstep(7.,10.,length(uv)));
}

#define swap(a,b) tv=a;a=b;b=tv

vec4 insides(vec3 ro,vec3 rd,vec3 nor_c,vec3 l_dir,out float tout){
    tout=-1.;
    vec3 col=vec3(0.);
    float pi=PI;
    if(abs(nor_c.x)>0.5){rd=rd.xzy*nor_c.x;ro=ro.xzy*nor_c.x;}
    else if(abs(nor_c.z)>0.5){l_dir*=roty(pi);rd=rd.yxz*nor_c.z;ro=ro.yxz*nor_c.z;}
    else if(abs(nor_c.y)>0.5){l_dir*=rotz(-pi*0.5);rd=rd*nor_c.y;ro=ro*nor_c.y;}
    const float curvature=.5;
    float bil_size=1.;
    vec4 ps=vec4(-1,-1,1,1)*curvature;
    vec4 ph=vec4(-1,1,1,-1)*curvature;
    vec4[3]colx=vec4[3](vec4(0.),vec4(0.),vec4(0.));
    vec3[3]dx=vec3[3](vec3(-1.),vec3(-1.),vec3(-1.));
    vec4[3]colxsi=vec4[3](vec4(0.),vec4(0.),vec4(0.));
    int[3]order=int[3](0,1,2);
    for(int i=0;i<3;i++){
        if(abs(nor_c.x)>0.5){ro*=rotz(-pi/3.);rd*=rotz(-pi/3.);}
        else if(abs(nor_c.z)>0.5){ro*=rotz(pi/3.);rd*=rotz(pi/3.);}
        else if(abs(nor_c.y)>0.5){ro*=rotx(pi/3.);rd*=rotx(pi/3.);}
        vec3 normnew;float tnew;bool si;float tsi;vec3 normsi;float fade,fadesi;
        if(iBilinearPatch(ro,rd,ps,ph,bil_size,tnew,normnew,si,tsi,normsi,fade,fadesi)){
            if(tnew>0.){
                vec4 tcol,tcolsi;
                calcColor(ro,rd,normnew,tnew,bil_size,i,si,tsi,tcol,tcolsi);
                if(tcol.a>0.){
                    dx[i]=vec3(tnew,float(si),tsi);
                    float dif=clamp(dot(normnew,l_dir),0.,1.);
                    float amb=clamp(0.5+0.5*dot(normnew,l_dir),0.,1.);
                    vec3 shad=vec3(0.32,0.43,0.54)*amb+vec3(1.,0.9,0.7)*dif;
                    const vec3 tcr=vec3(1.,0.21,0.11);
                    float ta=clamp(length(tcol.rgb),0.,1.);
                    tcol=clamp(tcol*tcol*2.,0.,1.);
                    vec4 tv=vec4(tcol.rgb*shad*1.4+3.*(tcr*tcol.rgb)*clamp(1.-(amb+dif),0.,1.),min(tcol.a,ta));
                    tv.rgb=clamp(2.*tv.rgb*tv.rgb,0.,1.);
                    tv*=min(fade*5.,1.);
                    colx[i]=tv;
                    if(si){
                        dif=clamp(dot(normsi,l_dir),0.,1.);
                        amb=clamp(0.5+0.5*dot(normsi,l_dir),0.,1.);
                        shad=vec3(0.32,0.43,0.54)*amb+vec3(1.,0.9,0.7)*dif;
                        ta=clamp(length(tcolsi.rgb),0.,1.);
                        tcolsi=clamp(tcolsi*tcolsi*2.,0.,1.);
                        tv=vec4(tcolsi.rgb*shad+3.*(tcr*tcolsi.rgb)*clamp(1.-(amb+dif),0.,1.),min(tcolsi.a,ta));
                        tv.rgb=clamp(2.*tv.rgb*tv.rgb,0.,1.);
                        tv.rgb*=min(fadesi*5.,1.);
                        colxsi[i]=tv;
                    }
                }
            }
        }
    }
    float a=1.;
    if(dx[0].x<dx[1].x){{vec3 swap(dx[0],dx[1]);}{int swap(order[0],order[1]);}}
    if(dx[1].x<dx[2].x){{vec3 swap(dx[1],dx[2]);}{int swap(order[1],order[2]);}}
    if(dx[0].x<dx[1].x){{vec3 swap(dx[0],dx[1]);}{int swap(order[0],order[1]);}}
    tout=max(max(dx[0].x,dx[1].x),dx[2].x);
    if(dx[0].y<0.5)a=colx[order[0]].a;
    bool[3]rul=bool[3](
        dx[0].y>0.5&&dx[1].x<=0.,
        dx[1].y>0.5&&dx[0].x>dx[1].z,
        dx[2].y>0.5&&dx[1].x>dx[2].z);
    for(int k=0;k<3;k++){
        if(rul[k]){
            vec4 tv=mix(colxsi[order[k]],colx[order[k]],colx[order[k]].a);
            colx[order[k]]=mix(vec4(0.),tv,max(colx[order[k]].a,colxsi[order[k]].a));
        }
    }
    float a1=(dx[1].y<0.5)?colx[order[1]].a:((dx[1].z>dx[0].x)?colx[order[1]].a:1.);
    float a2=(dx[2].y<0.5)?colx[order[2]].a:((dx[2].z>dx[1].x)?colx[order[2]].a:1.);
    col=mix(mix(colx[order[0]].rgb,colx[order[1]].rgb,a1),colx[order[2]].rgb,a2);
    a=max(max(a,a1),a2);
    return vec4(col,a);
}

void mainImage(out vec4 fragColor,in vec2 fragCoord){
    vec3 l_dir=normalize(vec3(0.,1.,0.));
    l_dir*=rotz(0.5);
    float mouseY=PI*0.49-smoothstep(0.,8.5,mod((iTimeSlow+tshift)*0.33,25.))*(1.-smoothstep(14.,24.,mod((iTimeSlow+tshift)*0.33,25.)))*0.55*PI;
    float mouseX=-2.*PI-0.25*(iTimeSlow*ROTATION_SPEED+tshift);
    vec3 eye=15.*vec3(cos(mouseX)*cos(mouseY),sin(mouseX)*cos(mouseY),sin(mouseY));
    vec3 w=normalize(-eye);
    vec3 up=vec3(0.,0.,1.);
    vec3 u=normalize(cross(w,up));
    vec3 v=cross(u,w);
    vec4 tot=vec4(0.);
    vec2 uv=(fragCoord-0.5*iResolution.xy)/iResolution.x;
    vec3 rd=normalize(w*FDIST+uv.x*u+uv.y*v);
    vec3 ni;
    float t=box(eye,rd,BOXDIMS,ni,true);
    vec3 ro=eye+t*rd;
    vec2 coords=ro.xy*ni.z/BOXDIMS.xy+ro.yz*ni.x/BOXDIMS.yz+ro.zx*ni.y/BOXDIMS.zx;
    float fadeborders=(1.-smoothstep(0.915,1.05,abs(coords.x)))*(1.-smoothstep(0.915,1.05,abs(coords.y)));
    if(t>0.){
        vec3 col=vec3(0.);
        float R0=(IOR-1.)/(IOR+1.);R0*=R0;
        vec3 n=vec3(0.,0.,1.);
        vec3 nr=n.zxy*ni.x+n.yzx*ni.y+n.xyz*ni.z;
        vec3 rdr=reflect(rd,nr);
        float talpha;
        vec3 reflcol=background(ro,rdr,l_dir,talpha);
        vec3 rd2=refract(rd,nr,1./IOR);
        float accum=1.;vec3 no2=ni;vec3 ro_refr=ro;
        vec4[2]colo=vec4[2](vec4(0.),vec4(0.));
        for(int j=0;j<2;j++){
            float tb;
            vec2 c2=ro_refr.xy*no2.z+ro_refr.yz*no2.x+ro_refr.zx*no2.y;
            vec3 e2=vec3(c2,-1.);
            vec3 rd2t=rd2.yzx*no2.x+rd2.zxy*no2.y+rd2.xyz*no2.z;
            rd2t.z=-rd2t.z;
            vec4 ic=insides(e2,rd2t,no2,l_dir,tb);
            if(tb>0.){ic.rgb*=accum;colo[j]=ic;}
            if(tb<=0.||ic.a<1.){
                float tout=box(ro_refr,rd2,BOXDIMS,no2,false);
                no2=n.zyx*no2.x+n.xzy*no2.y+n.yxz*no2.z;
                vec3 rout=ro_refr+tout*rd2;
                vec3 rdout=refract(rd2,-no2,IOR);
                float f2=R0+(1.-R0)*pow(1.-dot(rdout,no2),1.3);
                rd2=reflect(rd2,-no2);
                ro_refr=rout;ro_refr.z=max(ro_refr.z,-0.999);
                accum*=f2;
            }
        }
        float fresnel=R0+(1.-R0)*pow(1.-dot(-rd,nr),5.);
        col=mix(mix(colo[1].rgb*colo[1].a,colo[0].rgb,colo[0].a)*fadeborders,reflcol,pow(fresnel,1.5));
        col=clamp(col,0.,1.);
        tot=vec4(col,1.);
    }else{
        float alpha;
        tot=vec4(background(eye,rd,l_dir,alpha),1.);
    }
    fragColor=vec4(clamp(tot.rgb,0.,1.),1.);
}

void main(){
    vec4 fragColor;
    mainImage(fragColor,gl_FragCoord.xy);
    gl_FragColor=fragColor;
}
