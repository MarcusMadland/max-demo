$input v_texcoord0, v_texcoord1, v_texcoord2

#include "common/common.sh"
#include "common/uniforms.sh"

SAMPLER2D(s_gbufferDepth, 0);

// https://www.shadertoy.com/view/4ssXRX
// http://www.loopit.dk/banding_in_games.pdf
// http://www.dspguide.com/ch2/6.htm

float nrand(in vec2 n)
{
	return fract(sin(dot(n.xy, vec2(12.9898, 78.233)))* 43758.5453);
}

float n4rand_ss(in vec2 n)
{
	float nrnd0 = nrand( n + 0.07*fract( u_time ) );
	float nrnd1 = nrand( n + 0.11*fract( u_time + 0.573953 ) );
	return 0.23*sqrt(-log(nrnd0+0.00001))*cos(2.0*3.141592*nrnd1)+0.5;
}

void main()
{
	// Depth test
    float deviceDepth = texture2D(s_gbufferDepth, v_texcoord0 * 0.5 + 0.5).x;
	float depth       = toClipSpaceDepth(deviceDepth);
	
    if (deviceDepth < 0.9999)
    {
        discard;
    }

	//
	float size2 = u_sunSize * u_sunSize;
	vec3 lightDir = normalize(u_lightDir.xyz);
	float distance = 2.0 * (1.0 - dot(normalize(v_texcoord2.xyz), lightDir));
	float sun = exp(-distance/ u_sunBloom / size2) + step(distance, size2);
	float sun2 = min(sun * sun, 1.0);
	vec3 color = v_texcoord1.xyz + sun2;
	color = toGamma(color);
	float r = n4rand_ss(v_texcoord0.xy);
	color += vec3(r, r, r) / 40.0;

	//
	gl_FragColor = vec4(color, 1.0);
}
