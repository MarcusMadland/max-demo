$input v_texcoord0

#include "common/common.sh"
#include "common/uniforms.sh"

SAMPLERCUBE(s_skybox, 0);
SAMPLER2D(s_gbufferDepth, 1);

void main()
{
	// Depth test
    float deviceDepth = texture2D(s_gbufferDepth, v_texcoord0).x;
	float depth       = toClipSpaceDepth(deviceDepth);

    if (deviceDepth < 0.9999)
    {
        discard;
    }

	//
	mat4 cameraMtx;
	cameraMtx[0] = u_cameraMtx0;
	cameraMtx[1] = u_cameraMtx1;
	cameraMtx[2] = u_cameraMtx2;
	cameraMtx[3] = u_cameraMtx3;
    
	// 
	float fov = radians(45.0);
	float height = tan(fov*0.5);
	float aspect = height*(u_viewRect.z / u_viewRect.w);
	vec2 tex = (2.0*v_texcoord0-1.0) * vec2(aspect, height);
#if !MAX_SHADER_LANGUAGE_GLSL
	tex.y = -tex.y;
#endif // !MAX_SHADER_LANGUAGE_GLSL

	//
	vec3 dir = instMul(cameraMtx, vec4(tex, 1.0, 0.0) ).xyz;
	dir = normalize(dir);

	gl_FragColor = vec4(textureCube(s_skybox, dir).xyz, 1.0);
}
