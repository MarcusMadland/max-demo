$input v_texcoord0

#include "common/common.sh"

SAMPLER2D(s_buffer, 0);

void main()
{
	vec3 color  = toLinear(texture2D(s_buffer, v_texcoord0)).rgb;
	gl_FragColor = toGamma(vec4(color, 1.0));
}
