$input v_texcoord0

#include "common/common.sh"

SAMPLERCUBE(s_cubeDiffuse,  0); 
SAMPLERCUBE(s_cubeNormal,   1); 
SAMPLERCUBE(s_cubePosition, 2); 
SAMPLERCUBE(s_cubeDepth,    3); 

void main()
{
    vec3 dir = decodeNormalOctahedron(v_texcoord0); 

    gl_FragData[0] = vec4(textureCube(s_cubeDiffuse,  dir).rgb, 1.0);
    gl_FragData[1] = vec4(textureCube(s_cubeNormal,   dir).rgb, 1.0);
    gl_FragData[2] = vec4(textureCube(s_cubePosition, dir).rgb, 1.0);
    gl_FragData[3] = vec4(textureCube(s_cubeDepth,    dir).r, 1.0, 1.0, 1.0);
}