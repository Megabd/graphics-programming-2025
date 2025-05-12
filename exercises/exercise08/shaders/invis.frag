//Inputs
in vec3 WorldPosition;
in vec3 WorldNormal;
in vec3 WorldTangent;
in vec3 WorldBitangent;
in vec2 TexCoord;

//Outputs
out vec4 FragColor;

//Uniforms
uniform vec3 Color;
uniform sampler2D ColorTexture;
uniform sampler2D NormalTexture;
uniform sampler2D SpecularTexture;
uniform sampler2D NoiseTexture;

uniform vec3 CameraPosition;
uniform float Time;

float getNoiseValue(vec2 uv) {
    return texture(NoiseTexture, uv).r;
}

void main()
{
    SurfaceData data;
    data.normal = SampleNormalMap(NormalTexture, TexCoord, normalize(WorldNormal), normalize(WorldTangent), normalize(WorldBitangent));
    data.albedo = Color * texture(ColorTexture, TexCoord).rgb;
    vec3 arm = texture(SpecularTexture, TexCoord).rgb;
    data.ambientOcclusion = arm.x;
    data.roughness = arm.y;
    data.metalness = arm.z;

    vec3 position = WorldPosition;
    vec3 viewDir = GetDirection(position, CameraPosition);

    // Calculate distance and inverted visibility factor
    float distance = length(WorldPosition - CameraPosition);
    float visibilityFactor = 1.0 - smoothstep(1.0, 10.0, distance);  // Scale effects with distance

    // Fresnel outline calculation
    vec3 outlineNormal = normalize(WorldNormal); // Use outline Normal for general geo, ignore small details
    float cosTheta = dot(viewDir, outlineNormal);
    float fresnel = pow(1.0 - cosTheta, 3.0);       
    fresnel = clamp(fresnel * 3, 0.0, 1.0);         

    vec3 outlineColor = vec3(1.0, 0.5, 0.0); 

    // Flicker effect for visibility bursts
    float noiseValue = getNoiseValue(TexCoord * 5.0);
    float flicker = sin(Time * 10.0 + noiseValue * 20.0) * 0.8 + 0.2;
    flicker = step(0.9, flicker);  

    // Make the object normally invisible, but appear during flicker
    float alpha = mix(1.0, 0.5, fresnel) * (flicker * visibilityFactor);

    // Determine final color
    vec3 lightingColor = ComputeLighting(position, data, viewDir, true);

    // Calculate the refraction vector
    vec3 refractVec = refract(-normalize(viewDir), normalize(WorldNormal), 0.95f);
    refractVec = vec3(refractVec.x, refractVec.y, -refractVec.z);

    vec3 refractedColor = texture(EnvironmentTexture, refractVec).rgb;

    // Mix refraction color with outline and lighting
    vec3 finalColor = mix(refractedColor, outlineColor, fresnel);

    // Blend the outline color with the lighting based on the Fresnel factor

    FragColor = vec4(finalColor, alpha);

    // Only flicker
    //FragColor = vec4(lightingColor, alpha);

    // Only refraction
    //FragColor = vec4(refractedColor, 1.0);

    // only outline
    //FragColor = vec4(mix(lightingColor, outlineColor, fresnel), 1.0);

    // Refraction + flicker
    //FragColor = vec4(refractedColor, alpha);

    //Refraction + outline
    //FragColor = vec4(mix(refractedColor, outlineColor, fresnel), 1.0);

    // Outline + flicker
    //FragColor = vec4(mix(lightingColor, outlineColor, fresnel), alpha);
}
