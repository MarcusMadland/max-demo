$input v_normal, v_texcoord0

#include "common/common.sh"
#include "common/uniforms.sh"

SAMPLER2D(s_atlasIrradiance, 0);
SAMPLER2D(s_gbufferDepth,    1);

void main()
{
    // @todo u_width and u_height is not necesary... we have u_viewRect.
    vec2 uv = gl_FragCoord.xy / vec2(u_width, u_height);
    float deviceDepth = texture2D(s_gbufferDepth, uv).r;
    if (gl_FragCoord.z > deviceDepth)
    {
        discard; 
    }

    // 
    vec3 volumeMin = u_volumeMin;  // Origin of the probe grid (world space origin of volume)
    vec3 volumeMax = u_volumeMax;  // Extents of the probe grid + volumeMin 
    vec3 gridSize  = u_volumeSize; // Number of probes in each direction (should always be whole numbers 1.0, 2.0 etc)
    vec3 gridSpacing = vec3_splat(u_volumeSpacing); // Spacing between probes (world space spacing between probes)

    float octMapRes = 48.0;
    vec2 octAtlasRes = vec2(gridSize.x + gridSize.z, gridSize.y) * octMapRes;

    // Calculate closest probe based on surface world position
    vec3 probeGridPosition = u_probeGridPos;

    // Get octahedral coords based on surface normal
    vec2 octCoord = encodeNormalOctahedron(v_normal);
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
    vec3 irradiance = texelFetch(s_atlasIrradiance, ivec2(texel), 0).rgb; 

	gl_FragColor = vec4(irradiance, 1.0);
}
