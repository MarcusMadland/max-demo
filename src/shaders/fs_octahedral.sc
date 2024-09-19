$input v_texcoord0

#include "common/common.sh"

// In Cubemap Atlases
SAMPLERCUBE(s_diffuse, 0); 
SAMPLERCUBE(s_normal, 1); 
SAMPLERCUBE(s_position, 2); 

vec3 octahedralToDirection(vec2 _octCoord) 
{
    vec3 v = vec3_splat(0.0);
    v.z = 1.0 - abs(_octCoord.x) - abs(_octCoord.y);
    if (v.z < 0.0) 
    {
        _octCoord = (1.0 - abs(_octCoord.yx)) * sign(_octCoord);
    }
    v.xy = _octCoord;
    return normalize(v);
}

void main()
{
    vec3 dir = octahedralToDirection(v_texcoord0 * 2.0 - 1.0); // Remap from [0, 1] to [-1, 1]

    // Out Octahedral Atlases
    gl_FragData[0] = vec4(textureCube(s_diffuse, dir).rgb, 1.0);
    gl_FragData[1] = vec4(textureCube(s_normal, dir).rgb, 1.0);
    gl_FragData[2] = vec4(textureCube(s_position, dir).rgb, 1.0);
}