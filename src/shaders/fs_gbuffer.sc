$input v_normal, v_texcoord0, v_texcoord1, v_texcoord2, v_texcoord3

#include "common/common.sh"
#include "common/uniforms.sh"
#include "common/normal_encoding.sh"

SAMPLER2D(s_texDiffuse,   0);
SAMPLER2D(s_texNormal,    1); 
SAMPLER2D(s_texRoughness, 2);
SAMPLER2D(s_texMetallic,  3);

// http://www.thetenthplanet.de/archives/1180
// "followup: normal mapping without precomputed tangents"
mat3 cotangentFrame(vec3 N, vec3 p, vec2 uv)
{
	// get edge vectors of the pixel triangle
	vec3 dp1 = dFdx(p);
	vec3 dp2 = dFdy(p);
	vec2 duv1 = dFdx(uv);
	vec2 duv2 = dFdy(uv);

	// solve the linear system
	vec3 dp2perp = cross(dp2, N);
	vec3 dp1perp = cross(N, dp1);
	vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
	vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

	// construct a scale-invariant frame
	float invMax = inversesqrt(max(dot(T,T), dot(B,B)));
	return mat3(T*invMax, B*invMax, N);
}

void main()
{
	// Sample textures.
	vec2 texNormal  = vec3(texture2D(s_texNormal, v_texcoord0).rgb * u_texNormalFactor.rgb).xy;
	vec3 texDiffuse = texture2D(s_texDiffuse, v_texcoord0).rgb * u_texDiffuseFactor;
	float roughness = texture2D(s_texRoughness, v_texcoord0).r * u_texRoughnessFactor;
	float metallic  = texture2D(s_texMetallic, v_texcoord0).r * u_texMetallicFactor;

	// Get vertex normal
	vec3 normal = normalize(v_normal);

	// Get normal map normal, unpack, and calculate z
	vec3 normalMap;
	normalMap.xy = texNormal * 2.0 - 1.0;
	normalMap.z = sqrt(1.0 - dot(normalMap.xy, normalMap.xy));

	// Perturb geometry normal by normal map
	vec3 pos = v_texcoord2.xyz; // Contains world space pos
	mat3 TBN = cotangentFrame(normal, pos, v_texcoord0);
	vec3 bumpedNormal = normalize(instMul(TBN, normalMap));
	vec3 bufferNormal = normalEncode(bumpedNormal);

	//
	gl_FragData[0] = vec4(toGamma(texDiffuse), 1.0);
	gl_FragData[1] = vec4(bufferNormal, 1.0);
	gl_FragData[2] = vec4(roughness, metallic, 0.0, 1.0);
}