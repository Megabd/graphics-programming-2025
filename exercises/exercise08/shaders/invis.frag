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
    return texture(NoiseTexture, uv).r; // Get value from the noise texture map using the tex coodinates. .r since the value is in this channel.
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

    // Calculate distance and inverted visibility factor
    float distance = length(WorldPosition - CameraPosition);
    float visibilityFactor = smoothstep(maxVisDist, 0.0, distance);

    // Fresnel calculation
    float cosTheta = dot(viewDir, normalize(WorldNormal)); // Find cosine of the angle between vector from fragment to camera and surface normal. 0 = perpendicular, higher or lower is more perpendicular
    float fresnel = pow(1.0 - cosTheta, 3.0)*outlineStr; // Times increases the effect. Power off increases falloff, lower numbers become lower than larger numbers.
    fresnel = clamp(fresnel, 0.0, 1.0);           


    // Flicker effect for visibility bursts
    float alpha = 1.0;
    if (flickerOn){
        float noiseValue = getNoiseValue(TexCoord*flickerSize); 
        float flicker = clamp(sin(Time * flickerSpeed + noiseValue * flickerChaos), 0.0, 1.0);
        flicker = step(flickerThreshold, flicker);  
        alpha = flicker * visibilityFactor;
    }


    // Calculate the refraction vector and use instead of lighting color
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
