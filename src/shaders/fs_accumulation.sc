$input v_texcoord0

#include "common/common.sh"
#include "common/uniforms.sh"

SAMPLER2D(s_gbufferNormal,  0); // GBuffer Normal
SAMPLER2D(s_gbufferSurface, 1); // GBuffer Surface
SAMPLER2D(s_gbufferDepth,   2); // GBuffer Depth

SAMPLER2D(s_atlasIrradiance, 3); // Atlas of irradiance probes (x*y, z)

vec3 getClosestProbeGridPosition(vec3 wpos, vec3 gridOrigin, vec3 gridSpacing, vec3 gridSize)
{
    // Move world position into the local space of the grid
    vec3 localPos = (wpos - gridOrigin) / gridSpacing;

    // Round to the nearest integer to find the closest probe position
    vec3 probeGridPos = floor(localPos + 0.5); 

    // Clamp the probe position within the grid bounds
    probeGridPos = clamp(probeGridPos, vec3_splat(0.0), gridSize - vec3_splat(1.0));

    return probeGridPos;
}

void main()
{
    // Params
    float roughness   = texture2D(s_gbufferSurface, v_texcoord0).r; 
    float metallic    = texture2D(s_gbufferSurface, v_texcoord0).g; 
    vec3  normal      = decodeNormalUint(texture2D(s_gbufferNormal, v_texcoord0).rgb);
    
    //
    float deviceDepth = texture2D(s_gbufferDepth, v_texcoord0).x;
    if (deviceDepth > 0.9999)
    {
        gl_FragData[0] = vec4(0.0, 0.0, 0.0, 1.0);
        gl_FragData[1] = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    float depth       = toClipSpaceDepth(deviceDepth);

    // 
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
	vec3 wpos = clipToWorld(invViewProj, clip); // Wordpos of screen pixel

    // 
    vec3 volumeMin = u_volumeMin;  // Origin of the probe grid (world space origin of volume)
    vec3 volumeMax = u_volumeMax;  // Extents of the probe grid + volumeMin 
    vec3 gridSize  = u_volumeSize; // Number of probes in each direction (should always be whole numbers 1.0, 2.0 etc)
    vec3 gridSpacing = vec3_splat(u_volumeSpacing); // Spacing between probes (world space spacing between probes)

    float octMapRes = 48.0;
    vec2 octAtlasRes = vec2(gridSize.x + gridSize.z, gridSize.y) * octMapRes;

    // Calculate closest probe based on surface world position
    vec3 probeGridPosition = getClosestProbeGridPosition(wpos, volumeMin, gridSpacing, gridSize);

    // Get octahedral coords based on surface normal
    vec2 octCoord = encodeNormalOctahedron(normal);
    octCoord.y = 1.0 - octCoord.y;
    vec2 octTexel = (octCoord * octMapRes);

    // Calculate the atlas offset for the desired octahedral map
    vec2 atlasOffsetCoord = vec2(probeGridPosition.x + probeGridPosition.z * gridSize.x, probeGridPosition.y);
    vec2 atlasOffsetTexel = (atlasOffsetCoord * octMapRes);

    // Compute final texel coordinates in the atlas
    vec2 texel = floor(atlasOffsetTexel + octTexel);

    // Sample radiance using texelFetch
    // @todo Instead of texel fetch lets go with bilinear sampling using texture2D. 
    // a padding of 1pixel is required for this tho.
    vec3 radiance = texelFetch(s_atlasIrradiance, ivec2(texel), 0).rgb; 

    // Output to render targets
    gl_FragData[0] = vec4(radiance, 1.0); 
    gl_FragData[1] = vec4(probeGridPosition / gridSize, 1.0); // For debugging voxels.
    //gl_FragData[1] = vec4(probeGridPosition, 1.0); // For debugging voxels.
    //gl_FragData[1] = vec4(atlasOffsetTexel, 0.0, 1.0); // For debugging voxels.
}
