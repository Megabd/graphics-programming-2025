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

uniform bool outlineOn;
uniform bool flickerOn;
uniform bool refractionOn;

uniform float outlineStr;

uniform float maxVisDist;
uniform float flickerSpeed;
uniform float flickerSize;
uniform float flickerChaos;
uniform float flickerThreshold;

uniform float IOR;

float getNoiseValue(vec2 uv) {
    return texture(NoiseTexture, uv).r; // Get pixel from the noise texture map using the tex coodinates. .r to get the noise value
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

    vec3 lightingColor = ComputeLighting(position, data, viewDir, true);

    // Calculate distance and visibility factor (the further away, to closer to 0)
    float distance = length(WorldPosition - CameraPosition);
    float visibilityFactor = smoothstep(maxVisDist, 0.0, distance);

    // Edge calculation
    float cosTheta = dot(viewDir, normalize(WorldNormal)); // Find cosine of the angle between vector from fragment to camera and surface normal. 0 = perpendicular, higher or lower is more perpendicular
    float fresnel = pow(1.0 - cosTheta, 3.0)*outlineStr; // Times increases the effect. Power off increases falloff, lower numbers become lower than larger numbers.
    fresnel = clamp(fresnel, 0.0, 1.0);           


    // Flicker effect for visibility bursts
    float alpha = 1.0;
    if (flickerOn){
        float noiseValue = getNoiseValue(TexCoord*flickerSize); // Flicker size increases the distance between neighbors on the noise map.
        float flicker = clamp(sin(Time * flickerSpeed + noiseValue * flickerChaos), 0.0, 1.0); // We move across a sine wave. Flicker speed increases radians per second, noiseValue*flickerchaos definess the phase
        flicker = step(flickerThreshold, flicker); // We have to be above a flickerThreshold on the sine wave to be counted as on/1, otherwise 0. 
        alpha = flicker * visibilityFactor; // Decrease the alpha at longer distances
    }


    // Calculate the refraction vector and sample from enviroment texture
    if (refractionOn){
        vec3 refractVec = refract(-normalize(viewDir), normalize(WorldNormal), IOR);
        lightingColor = texture(EnvironmentTexture, refractVec).rgb;
    }

    // Give outline based on fresnel
    if (outlineOn){
        vec3 outlineColor = vec3(1.0, 0.5, 0.0); 
        lightingColor = mix(lightingColor, outlineColor, fresnel);
    }

    FragColor = vec4(lightingColor, alpha);
}
