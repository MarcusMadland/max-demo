$input v_texcoord0

#include "common/common.sh"

// In Cubemap Atlases
SAMPLERCUBE(s_diffuse,  0); 
SAMPLERCUBE(s_normal,   1); 
SAMPLERCUBE(s_position, 2); 
SAMPLERCUBE(s_depth,    3); 

void main()
{
    vec3 dir = decodeNormalOctahedron(v_texcoord0); 

    // Out Octahedral Atlases
    gl_FragData[0] = vec4(textureCube(s_diffuse, dir).rgb,  1.0);
    gl_FragData[1] = vec4(textureCube(s_normal, dir).rgb,   1.0);
    gl_FragData[2] = vec4(textureCube(s_position, dir).rgb, 1.0);
    gl_FragData[3] = vec4(textureCube(s_depth, dir).rgb,    1.0);
}