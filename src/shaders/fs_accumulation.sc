$input v_texcoord0

#include "common/common.sh"
#include "common/uniforms.sh"

SAMPLER2D(s_surface,  0); // GBuffer Surface: This contains metallic and roughness values only. Diffuse is not used here since this shader will only output light
SAMPLER2D(s_normal,   1); // GBuffer Normal
SAMPLER2D(s_depth,    2); // GBuffer Depth
SAMPLER2D(s_radiance, 3); // Radiance Atlas of probes.
//SAMPLER2D(s_irradiance,   4); @todo later

void main()
{
    // Params
    float roughness   = texture2D(s_surface, v_texcoord0).r; 
    float metallic    = texture2D(s_surface, v_texcoord0).g; 
    vec3  normal      = decodeNormalUint(texture2D(s_normal, v_texcoord0).rgb);
    float deviceDepth = texture2D(s_depth, v_texcoord0).x;
	float depth       = toClipSpaceDepth(deviceDepth);

    // This is the InvViewProj
    mat4 invViewProj;
	invViewProj[0] = u_invViewProj0;
	invViewProj[1] = u_invViewProj1;
	invViewProj[2] = u_invViewProj2;
	invViewProj[3] = u_invViewProj3;

    // World pos from depth
    vec3 clip = vec3(v_texcoord0 * 2.0 - 1.0, depth);
#if !MAX_SHADER_LANGUAGE_GLSL
	clip.y = -clip.y;
#endif // !MAX_SHADER_LANGUAGE_GLSL
	vec3 wpos = clipToWorld(invViewProj, clip);

    // Probe Volume extents (Min and Max corners)
    vec3 minCorner = u_volumeMin; 
    vec3 maxCorner = u_volumeMax;
    vec3 volumeExtents = maxCorner - minCorner;
    
    // Local probe position
    vec3 localPos = (wpos - minCorner) / volumeExtents;
    
    // Compute light probe indices
    vec3 probeGridSize = u_volumeSize;
    vec3 probeCoord = localPos * (probeGridSize - 1.0); 
    vec3 probeCoordFloor = floor(probeCoord);
    vec3 probeCoordFrac = probeCoord - probeCoordFloor;
    
    // Closest 8 surrounding probes
    vec3 probeCoords[8];
    probeCoords[0] = probeCoordFloor;
    probeCoords[1] = probeCoordFloor + vec3(1.0, 0.0, 0.0);
    probeCoords[2] = probeCoordFloor + vec3(0.0, 1.0, 0.0);
    probeCoords[3] = probeCoordFloor + vec3(0.0, 0.0, 1.0);
    probeCoords[4] = probeCoordFloor + vec3(1.0, 1.0, 0.0);
    probeCoords[5] = probeCoordFloor + vec3(1.0, 0.0, 1.0);
    probeCoords[6] = probeCoordFloor + vec3(0.0, 1.0, 1.0);
    probeCoords[7] = probeCoordFloor + vec3(1.0, 1.0, 1.0);
    
    // Sample the radiance atlas
    vec3 radiance[8];
    for (int ii = 0; ii < 8; ++ii) 
    {
        vec3 probeUV = probeCoords[ii] / vec3(probeGridSize.x * probeGridSize.z, probeGridSize.y, 0.0); // UV coordinates
        
        // I need to get the exact position on the probe inside the atlas as well...
        // Because right now this just gives me a bunch of squares at different colors because each probe is sampled from same spot.
        // Let me know if I need to input s_positionAtlas.

        radiance[ii] = texture2D(s_radiance, probeUV.xy).rgb; // Sample from the atlas
    }

    // Trilinear interpolation
    vec3 colorInterp0 = mix(radiance[0], radiance[1], probeCoordFrac.x);
    vec3 colorInterp1 = mix(radiance[2], radiance[3], probeCoordFrac.x);
    vec3 colorInterp2 = mix(radiance[4], radiance[5], probeCoordFrac.x);
    vec3 colorInterp3 = mix(radiance[6], radiance[7], probeCoordFrac.x);
   
    vec3 colorInterp = mix(mix(colorInterp0, colorInterp1, probeCoordFrac.y),
                           mix(colorInterp2, colorInterp3, probeCoordFrac.y),
                           probeCoordFrac.z);

    // Output to render targets
    gl_FragData[0] = vec4(colorInterp, 1.0); 
    gl_FragData[1] = vec4_splat(1.0);    
}