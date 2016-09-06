#version 120

uniform sampler2D sceneTex;
uniform sampler2D linearDepthTexture;
uniform int uBlurSize; // use size of noise texture
uniform vec2 sceneSize;

uniform mat4 projMatrix;

uniform int displayType = 0; 
uniform int blurAOEnable = 0; 
uniform int haloRemoval = 0; 
uniform float haloTreshold = 0; 

float reconstruct_z(in float depth, in mat4 projMatrix){
	return -projMatrix[3][2] / (depth + projMatrix[2][2]);
}

float blurAOHaloRemoval()
{
	vec2 texelSize = 1.0 / vec2(sceneSize);
	float result = 0.0;
	vec2 hlim = vec2(float(-uBlurSize) * 0.5 + 0.5);
	float depth = reconstruct_z(texture2D(linearDepthTexture, gl_TexCoord[0].st).r, projMatrix);

	float totalWeight = float(uBlurSize * uBlurSize);
	float pixelWeight = 1.0f;

	for (int i = 0; i < uBlurSize; ++i) {
		for (int j = 0; j < uBlurSize; ++j) {
			vec2 offset = (hlim + vec2(float(i), float(j))) * texelSize;
			float offsetDepth = reconstruct_z(texture2D(linearDepthTexture, gl_TexCoord[0].st + offset).r, projMatrix);

			if (abs(offsetDepth - depth) > haloTreshold) {
				totalWeight -= pixelWeight;
				continue;
			}

			result += texture2D(sceneTex, gl_TexCoord[0].st + offset).a;
		}
	}
	
	// This is blurred occlusion factor
	float occlusionFactor = result / totalWeight;

	return occlusionFactor;
}

float blurAO()
{
	vec2 texelSize = 1.0 / vec2(sceneSize);
	float result = 0.0;
	vec2 hlim = vec2(float(-uBlurSize) * 0.5 + 0.5);
	for (int i = 0; i < uBlurSize; ++i) {
		for (int j = 0; j < uBlurSize; ++j) {
			vec2 offset = (hlim + vec2(float(i), float(j))) * texelSize;
			result += texture2D(sceneTex, gl_TexCoord[0].st + offset).a;
		}
	}
	
	// This is blurred occlusion factor
	float occlusionFactor = (result / float(uBlurSize * uBlurSize));

	return occlusionFactor;
}

float AO()
{
	return texture2D(sceneTex, gl_TexCoord[0].st).a;
}

vec3 Color()
{
	return texture2D(sceneTex, gl_TexCoord[0].st).rgb;
}

void main(void)
{
	float occlusion;
	vec3 color = Color();
	vec3 resultColor;

	if (displayType == 1) {
		if (haloRemoval == 1) {
			if (blurAOEnable == 1) {
				resultColor = color * blurAOHaloRemoval();
			} else {
				resultColor = color * AO();
			}
		} else {
			if (blurAOEnable == 1) {
				resultColor = color * blurAO();
			} else {
				resultColor = color * AO();
			}
		}
	} else if(displayType == 2) {
		if (haloRemoval == 1) {
			if (blurAOEnable == 1) {
				resultColor = vec3(blurAOHaloRemoval());
			} else {
				resultColor = vec3(AO());
			}
		} else {
			if (blurAOEnable == 1) {
				resultColor = vec3(blurAO());
			} else {
				resultColor = vec3(AO());
			}
		}
	} else {
		resultColor = color;
	}

	gl_FragColor = vec4(resultColor, 1);
}
