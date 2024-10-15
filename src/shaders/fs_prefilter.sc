$input v_texcoord0

#include "common/common.sh"

SAMPLER2D(s_atlasRadiance, 0); 
SAMPLER2D(s_atlasNormal,   1); 
SAMPLER2D(s_atlasPosition, 2); 
SAMPLER2D(s_atlasDepth,    3); 

// Generate a random direction in a hemisphere using uniform sampling
vec3 generateHemisphereSample(vec3 normal, vec2 rand) 
{
    float theta = acos(sqrt(1.0 - rand.x)); // Elevation angle
    float phi = 2.0 * 3.14159265359 * rand.y;          // Azimuth angle
    
    // Convert spherical to cartesian coordinates
    float x = sin(theta) * cos(phi);
    float y = sin(theta) * sin(phi);
    float z = cos(theta);
    
    // Transform to align with the normal
    vec3 tangent = normalize(cross(vec3(0.0, 1.0, 0.0), normal));
    vec3 bitangent = cross(normal, tangent);
    return normalize(x * tangent + y * bitangent + z * normal);
}

// Importance sample the hemisphere and accumulate weighted radiance
vec3 prefilterRadianceAtlas(vec3 normal, vec2 uv) 
{
    vec3 accumulatedRadiance = vec3_splat(0.0);
    float totalWeight = 0.0;

    // Monte Carlo integration loop
    const int numSamples = 64;
    for (int ii = 0; ii < numSamples; ++ii) 
    {
        // Generate random direction on the hemisphere
        vec2 randomSample = vec2(float(ii) / float(numSamples), fract(sin(float(ii) * 12.9898) * 43758.5453));
        vec3 sampleDir = generateHemisphereSample(normal, randomSample);

        // Adjust UV coordinates based on sample direction (with padding)
        vec2 atlasSize = vec2(432, 144);
        vec2 padding = vec2(1.0 / atlasSize); // 1-pixel padding
        vec2 sampleUV = uv + padding * sampleDir.xy;
        sampleUV = clamp(sampleUV, padding, vec2_splat(1.0) - padding); // Clamp to avoid sampling outside the texture

        // Fetch radiance from the atlas
        vec3 sampleRadiance = texture2D(s_atlasRadiance, sampleUV).rgb;

        // Weight the sample based on its contribution (cosine-weighted hemisphere sampling)
        float weight = max(dot(normal, sampleDir), 0.0);  // Cosine term
        accumulatedRadiance += sampleRadiance * weight;
        totalWeight += weight;
    }

    // Return the weighted average radiance
    return accumulatedRadiance / totalWeight;
}

void main()
{
    vec3 normal = texture2D(s_atlasNormal, v_texcoord0).rgb; 
    vec3 irradiance = prefilterRadianceAtlas(normal, v_texcoord0);

    gl_FragColor = vec4(irradiance, 1.0);
}