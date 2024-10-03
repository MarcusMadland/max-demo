$input a_position, a_normal, a_texcoord0
$output v_normal, v_texcoord0, v_texcoord1, v_texcoord2, v_texcoord3

#include "common/common.sh"

void main()
{
	vec3 wsPos = mul(u_model[0], vec4(a_position, 1.0) ).xyz;
	gl_Position = mul(u_viewProj, vec4(wsPos, 1.0) );
	
	// Calculate previous frame's position
	vec3 vspPos = instMul(u_view, vec4(wsPos, 1.0)).xyz;
	vec4 pspPos = instMul(u_proj, vec4(vspPos, 1.0));
	
	// Calculate normal, unpack
	vec3 osNormal = a_normal.xyz * 2.0 - 1.0;

	// Transform normal into world space
	vec3 wsNormal = mul(u_model[0], vec4(osNormal, 0.0)).xyz;

	v_normal = normalize(wsNormal);

	// Texture coordinates
	v_texcoord0 = a_texcoord0;

	// Store previous frame projection space position in extra texCoord attribute
	v_texcoord1 = pspPos;

	// Store world space view vector in extra texCoord attribute
	vec3 wsCamPos = mul(u_invView, vec4(0.0, 0.0, 0.0, 1.0)).xyz;
	vec3 view = normalize(wsCamPos - wsPos);

	v_texcoord2 = vec4(wsPos, 1.0);
	v_texcoord3 = vec4(wsCamPos, 1.0);
}