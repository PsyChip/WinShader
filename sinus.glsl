// SINUS by Green120

uniform float time;
uniform vec2 resolution;

void main( void ) {
    vec2 p = ( gl_FragCoord.xy / resolution.xy ) - 0.5;

    float sx = 0.2 * (p.x + 0.5) * sin(20.0 * p.x - 2. * time);
    float dy = 1. / (1000. * abs(p.y - sx));

    float red = .0;
    float blue = dy * 5.;
    float green = blue / .3;

    if (p.x > .0) {
        blue  += sin(p.x / 2.);
        green += p.x / 12.;
    }

    blue -= 0.4;

    gl_FragColor = vec4( vec3( red, green, blue ), 1.0 );
}
