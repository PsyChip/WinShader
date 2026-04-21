uniform float time;
uniform vec2 resolution;

void main() {
    gl_FragColor = vec4(0.0);
    for(float i,z,d;i++<4e1;gl_FragColor+=vec4(9,i,z,1)/d){vec3 p=z*normalize(gl_FragCoord.rgb*2.-vec3(resolution,1.).xyy),a=normalize(sin(time/4.+vec3(0,2,4))),v;p.z+=7.;v=a=dot(a,p)*a+cross(a,p);
    for(d=2.;d++<6.;a+=sin(ceil(a*d)-time).yzx/d);z+=d=.1*length(sin(a*a))*sqrt(length(v*sin(v.yzx)));}
    gl_FragColor=tanh(gl_FragColor/6e4);
}
