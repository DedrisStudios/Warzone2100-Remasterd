// Version directive is set by Warzone when loading the shader
// (This shader supports GLSL 1.20 - 1.50 core.)

uniform sampler2D Texture;

uniform float gamma;

#if (!defined(GL_ES) && (__VERSION__ >= 130)) || (defined(GL_ES) && (__VERSION__ >= 300))
#define NEWGL
#else
#define texture(tex,uv) texture2D(tex,uv)
#endif

#ifdef NEWGL
in vec2 texCoords;
#else
varying vec2 texCoords;
#endif

#ifdef NEWGL
out vec4 FragColor;
#else
// Uses gl_FragColor
#endif

void main()
{
	vec3 texColour = texture(Texture, texCoords).rgb;

	// DedrisRemastered (OVERWATCH-C2): subtle cinematic grade of the 3D scene as it is
	// copied to screen — a light filmic (ACES-approx) tonemap to deepen blacks and tame
	// highlights, plus a gentle radial vignette. Conservative and single-pass; NO bloom.
	// The HUD is drawn afterwards, so it stays crisp and ungraded.
	vec3 x = texColour * 1.02;
	vec3 tonemapped = clamp((x * (2.51 * x + 0.03)) / (x * (2.43 * x + 0.59) + 0.14), 0.0, 1.0);
	texColour = mix(texColour, tonemapped, 0.35);

	vec2 vd = texCoords - vec2(0.5, 0.5);
	float vig = smoothstep(0.35, 0.9, dot(vd, vd) * 2.2);
	texColour *= mix(1.0, 0.9, vig);

	#ifdef NEWGL
	FragColor = vec4(texColour, 1.0);
	#else
	gl_FragColor = vec4(texColour, 1.0);
	#endif
}
