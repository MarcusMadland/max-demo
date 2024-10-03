$input v_texcoord0

#include "common/common.sh"
#include "common/uniforms.sh"

// Atlases
SAMPLER2D(s_diffuse,  3); 
SAMPLER2D(s_normal,   4); 
SAMPLER2D(s_position, 5); 

void main()
{
    // Params.
    vec3 diffuse = texture2D(s_diffuse, v_texcoord0).rgb;
    vec3 normal = normalize(texture2D(s_normal, v_texcoord0).xyz * 2.0 - 1.0); // Normalize normals from [-1,1]
    vec3 wpos = texture2D(s_position, v_texcoord0).rgb;

    vec3 lightDir = normalize(u_lightDir); 
    vec3 lightCol = u_lightCol;

    // Compute NdotL (Lambertian reflection model)
    float NdotL = max(dot(normal, lightDir), 0.0); // Ensure no negative lighting

    // Final radiance computation (diffuse only)
    vec3 radiance = diffuse * lightCol * NdotL;

    // Output the radiance to the radiance atlas
    gl_FragColor = vec4(radiance, 1.0); 
}