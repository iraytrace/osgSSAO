#version 120

// G-buffer
uniform sampler2D linearDepthTexture;
uniform sampler2D normalTexture;
uniform sampler2D colorTexture;

uniform sampler2D noiseTexture;

const int MAX_KERNEL_SIZE = 128;
uniform vec3 ssaoKernel[MAX_KERNEL_SIZE];

uniform mat4 projMatrix;
uniform mat4 invProjMatrix;
uniform vec2 noiseTextureRcp;
uniform int kernelSize;

uniform float ssaoRadius;
uniform float ssaoPower;

uniform int displayType = 0; 

vec3 reconstruct_pos(float z, vec2 vTexCoord, in mat4 projMatrix){
    vec4 vProjectedPos = vec4(vTexCoord * 2.0 - 1.0, z, 1.0f);
    vProjectedPos = invProjMatrix * vProjectedPos; 
    return vProjectedPos.xyz / vProjectedPos.w;  
}

float reconstruct_z(in float depth, in mat4 projMatrix){
    return -projMatrix[3][2] / (depth + projMatrix[2][2]);
}

float ssao()
{
    //	Calculate view space position
    float originDepthNormalized = texture2D(linearDepthTexture, gl_TexCoord[0].st).r;
	
    // Skip fragments on far plane
    if (originDepthNormalized == 1.0f) return 1.0f;

    vec3 origin = reconstruct_pos(originDepthNormalized, gl_TexCoord[0].st, projMatrix);

    // Fetch view space normal
    vec3 normal = normalize(texture2D(normalTexture, gl_TexCoord[0].st).xyz * 2.0 - 1.0);

    // Fetch noise
    vec3 rvec = texture2D(noiseTexture, gl_TexCoord[0].st * noiseTextureRcp).xyz * 2.0 - 1.0;

    // Calculate change-of-basis matrix (view space -> "face" space)
    vec3 tangent = normalize(rvec - dot(rvec, normal) * normal);
    vec3 bitangent = cross(tangent, normal);
    mat3 tbn = mat3(tangent, bitangent, normal);
	
    float occlusion = 0.0;

    for (int i = 0; i < kernelSize; ++i) {

	// get sample position:
	vec3 _sample = origin + (tbn * (ssaoKernel[i])) * ssaoRadius;

	// project sample position:
	vec4 offset = projMatrix * vec4(_sample, 1.0);

	offset.xy /= offset.w;
	offset.xy = offset.xy * 0.5 + 0.5;

	// get sample depth:
	float sampleDepth = texture2D(linearDepthTexture, offset.xy).r;
	sampleDepth = reconstruct_z(sampleDepth, projMatrix);
	
	float dist = abs(origin.z - sampleDepth);
		
	float rangeCheck = smoothstep(0.0, 1.0, ssaoRadius / dist);
	occlusion += rangeCheck * step(_sample.z, sampleDepth);
    }

    occlusion = 1.0 - (occlusion / float(kernelSize));
    occlusion = pow(occlusion, ssaoPower);
	 
    return occlusion;
}


void main(void)
{
    float occlusion = ssao();
    vec3 color = texture2D(colorTexture, gl_TexCoord[0].st).rgb;
    gl_FragColor = vec4(color, occlusion);
}
