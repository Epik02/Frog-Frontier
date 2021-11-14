#version 410


layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUV;

// We output a single color to the color buffer
layout(location = 0) out vec4 frag_color;

////////////////////////////////////////////////////////////////
///////////// Application Level Uniforms ///////////////////////
////////////////////////////////////////////////////////////////

// Global light properties
uniform vec3  u_AmbientCol;

// Represents a single light source
struct Light {
	vec3  Position;
	vec3  Color;
	float Attenuation;
};

#define MAX_LIGHTS 8
// Our array of all lights
uniform Light u_Lights[MAX_LIGHTS];
// The number of enabled lights
uniform int u_NumLights;

////////////////////////////////////////////////////////////////
/////////////// Frame Level Uniforms ///////////////////////////
////////////////////////////////////////////////////////////////

// The position of the camera in world space
uniform vec3  u_CamPos;

////////////////////////////////////////////////////////////////
/////////////// Instance Level Uniforms ////////////////////////
////////////////////////////////////////////////////////////////

// Represents a collection of attributes that would define a material
// For instance, you can think of this like material settings in 
// Unity
struct Material {
	sampler2D Diffuse;
	float     Shininess;
};
// Create a uniform for the material
uniform Material u_Material;

// Calculates the contribution the given light has for
// the current fragment
// @param normal The fragment's normal (normalized)
// @param Light  The light to caluclate the contribution for
vec3 CalcLightContribution(vec3 normal, Light light) {
	// Get the direction to the light in world space
	vec3 toLight = light.Position - inWorldPos;
	// Get distance between fragment and light
	float dist = length(toLight);
	// Normalize toLight for other calculations
	toLight = normalize(toLight);

	// Direction between camera and fragment
	vec3 viewDir  = normalize(u_CamPos - inWorldPos);
	// Halfway vector between light normal and direction to camera
	vec3 halfDir     = normalize(toLight + viewDir);

	// Calculate our specular power
	float specPower  = pow(max(dot(normal, halfDir), 0.0), u_Material.Shininess);
	// Calculate specular color
	vec3 specularOut = specPower * light.Color;

	// Calculate diffuse factor
	float diffuseFactor = max(dot(normal, toLight), 0);
	// Calculate diffuse color
	vec3  diffuseOut = diffuseFactor * light.Color;

	// We'll use a modified distance squared attenuation factor to keep it simple
	// We add the one to prevent divide by zero errors
	float attenuation = 1.0 / (1.0 + light.Attenuation * pow(dist, 2));

	return (diffuseOut + specularOut) * attenuation;
}

// https://learnopengl.com/Advanced-Lighting/Advanced-Lighting
void main() {
	// Will accumulate the contributions of all lights on this fragment
	vec3 lightAccumulation = vec3(0);

	// Normalize our input normal
	vec3 normal = normalize(inNormal);

	// Iterate over all lights
	for(int ix = 0; ix < u_NumLights && ix < MAX_LIGHTS; ix++) {
		// Additive lighting model
		lightAccumulation += CalcLightContribution(normal, u_Lights[ix]);
	}

	// Get the albedo from the diffuse / albedo map
	vec4 textureColor = texture(u_Material.Diffuse, inUV);

	// combine for the final result
	vec3 result = (u_AmbientCol + lightAccumulation)  * inColor * textureColor.rgb;

	frag_color = vec4(result, textureColor.a);
}