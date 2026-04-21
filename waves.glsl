uniform float time;
uniform vec2 resolution;

void main( void )
{
    vec2 uPos = ( gl_FragCoord.xy / resolution.xy );

    uPos.x -= 1.0;
    uPos.y -= 0.5;
    uPos *= 1.4; // zoom out

    vec3 color = vec3(0.0);
    for( float i = 0.0; i < 5.0; ++i )
    {
        float t = time * 0.35;

        uPos.y += sin( uPos.x*i + t+i/2.0 ) * 0.1;
        float fTemp = abs(1.0 / uPos.y / 100.0);
        color += vec3( fTemp*0.05, fTemp*(i/10.0+0.4), fTemp*0.15 );
    }

    gl_FragColor = vec4(color, 1.0);
}
