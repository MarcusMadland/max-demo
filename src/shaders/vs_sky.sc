$input a_position
$output v_texcoord0, v_texcoord1, v_texcoord2

#include "common/common.sh"
#include "common/uniforms.sh"

vec3 Perez(vec3 A,vec3 B,vec3 C,vec3 D, vec3 E,float costeta, float cosgamma)
{
	float _1_costeta = 1.0 / costeta;
	float cos2gamma = cosgamma * cosgamma;
	float gamma = acos(cosgamma);
	vec3 f = (vec3_splat(1.0) + A * exp(B * _1_costeta))
		   * (vec3_splat(1.0) + C * exp(D * gamma) + E * cos2gamma);
	return f;
}

void main()
{
	v_texcoord0 = a_position.xy;

	vec4 rayStart = mul(u_invViewProj, vec4(vec3(a_position.xy, -1.0), 1.0));
	vec4 rayEnd = mul(u_invViewProj, vec4(vec3(a_position.xy, 1.0), 1.0));

	rayStart = rayStart / rayStart.w;
	rayEnd = rayEnd / rayEnd.w;

	v_texcoord2.xyz = normalize(rayEnd.xyz - rayStart.xyz);
	v_texcoord2.y = abs(v_texcoord2.y);

	gl_Position = vec4(a_position.xy, 1.0, 1.0);

	vec3 lightDir = normalize(u_lightDir.xyz);
	vec3 skyDir = vec3(0.0, 1.0, 0.0);

	// Perez coefficients.
	vec3 A = u_perezCoeff0.xyz;
	vec3 B = u_perezCoeff1.xyz;
	vec3 C = u_perezCoeff2.xyz;
	vec3 D = u_perezCoeff3.xyz;
	vec3 E = u_perezCoeff4.xyz;

	float costeta = max(dot(v_texcoord2.xyz, skyDir), 0.001);
	float cosgamma = clamp(dot(v_texcoord2.xyz, lightDir), -0.9999, 0.9999);
	float cosgammas = dot(skyDir, lightDir);

	vec3 P = Perez(A,B,C,D,E, costeta, cosgamma);
	vec3 P0 = Perez(A,B,C,D,E, 1.0, cosgammas);

	vec3 skyColorxyY = vec3(
		  u_skyLuminance.x / (u_skyLuminance.x + u_skyLuminance.y + u_skyLuminance.z)
		, u_skyLuminance.y / (u_skyLuminance.x + u_skyLuminance.y + u_skyLuminance.z)
		, u_skyLuminance.y
		);

	vec3 Yp = skyColorxyY * P / P0;

	vec3 skyColorXYZ = vec3(Yp.x * Yp.z / Yp.y, Yp.z, (1.0 - Yp.x- Yp.y)*Yp.z/Yp.y);

	v_texcoord1.xyz = convertXYZ2RGB(skyColorXYZ * u_exposition);
}
