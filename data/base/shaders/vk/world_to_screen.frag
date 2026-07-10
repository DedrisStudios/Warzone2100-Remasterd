#version 450

layout(std140, set = 0, binding = 0) uniform cbuffer {
	float gamma;
};

layout(set = 1, binding = 0) uniform sampler2D Texture;

layout(location = 0) in vec2 texCoords;
layout(location = 0) out vec4 FragColor;

void main()
{
	vec3 texColour = texture(Texture, texCoords).rgb;

	// DedrisRemastered (OVERWATCH-C2): subtle cinematic grade of the 3D scene (mirror of
	// the GL variant so it also renders on the Vulkan backend) — light filmic tonemap +
	// gentle vignette. Conservative, single-pass, no bloom. HUD is drawn afterwards.
	vec3 x = texColour * 1.02;
	vec3 tonemapped = clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
	texColour = mix(texColour, tonemapped, 0.35);

	vec2 vd = texCoords - vec2(0.5, 0.5);
	float vig = smoothstep(0.35, 0.9, dot(vd, vd) * 2.2);
	texColour *= mix(1.0, 0.9, vig);

	FragColor = vec4(texColour, 1.0);
}
