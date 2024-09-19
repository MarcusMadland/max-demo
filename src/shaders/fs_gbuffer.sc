$input v_wpos, v_view, v_normal, v_tangent, v_bitangent, v_texcoord0

#include "common/common.sh"
#include "common/uniforms.sh"

SAMPLER2D(s_texDiffuse,   0);
SAMPLER2D(s_texNormal,    1); 
SAMPLER2D(s_texRoughness, 2);
SAMPLER2D(s_texMetallic,  3);

void main()
{
	vec2 texNormal = (texture2D(s_texNormal, v_texcoord0).rgb * u_texNormalFactor.rgb).xy;

	vec3 normal;
	normal.xy = texNormal * 2.0 - 1.0;
	normal.z  = sqrt(1.0 - dot(normal.xy, normal.xy) );

	mat3 tbn = mat3(
				normalize(v_tangent),
				normalize(v_bitangent),
				normalize(v_normal)
				);

	normal = normalize(mul(tbn, normal) );

	vec3 wnormal = normalize(mul(u_invView, vec4(normal, 0.0) ).xyz);

	vec3 texDiffuse = texture2D(s_texDiffuse, v_texcoord0).rgb * u_texDiffuseFactor;

	float roughness = texture2D(s_texRoughness, v_texcoord0).r * u_texRoughnessFactor;
	float metallic = texture2D(s_texMetallic, v_texcoord0).r * u_texMetallicFactor;

	gl_FragData[0] = vec4(texDiffuse, 1.0);
	gl_FragData[1] = vec4(encodeNormalUint(wnormal), 1.0);
	gl_FragData[2] = vec4(roughness, metallic, 0.0, 1.0);
}