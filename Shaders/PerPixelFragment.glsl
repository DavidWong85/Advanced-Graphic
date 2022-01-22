#version 330 core

uniform sampler2D diffuseTex;
uniform sampler2D diffuseTex2;
uniform sampler2D diffuseTex3;

uniform sampler2D bumpTex;
uniform sampler2D bumpTex2;
uniform sampler2D bumpTex3;

uniform sampler2D shadowTex;

uniform int doHeightMap;

uniform vec3 cameraPos;
uniform vec4 lightColour;
uniform vec3 lightPos;
uniform float lightRadius;

in Vertex {
	vec3 colour;
	vec2 texCoord;
	vec3 normal;
	vec3 tangent;
	vec3 binormal;
	vec3 worldPos;
	vec4 shadowProj;
	float height;
} IN;

out vec4 fragColour;

void main(void) {
	vec3 incident = normalize(lightPos - IN.worldPos);
	vec3 viewDir = normalize(cameraPos - IN.worldPos);
	vec3 halfDir = normalize(incident + viewDir);

	mat3 TBN = mat3(normalize(IN.tangent), normalize(IN.binormal), normalize(IN.normal));
	
	vec4 diffuse;
	vec3 bumpNormal;

	//HeightMap
	if (doHeightMap == 1) {
		int offset = 150;
		vec2 sandLayer = vec2(1,950);
		vec2 stoneLayer = vec2(sandLayer.y + offset, sandLayer.y + offset * 1.5);
		vec2 grassLayer = vec2(stoneLayer.y + offset, stoneLayer.y + offset * 2);
		vec2 mountainLayer = vec2(grassLayer.y + offset, 1000);

		if (IN.height < sandLayer.y) {
			diffuse = texture(diffuseTex3, IN.texCoord);
			bumpNormal = texture(bumpTex3, IN.texCoord).rgb;
		}
		else if (IN.height > sandLayer.y && IN.height < stoneLayer.x) {
			diffuse = mix(texture(diffuseTex3, IN.texCoord),texture(diffuseTex2, IN.texCoord), (IN.height - sandLayer.y) / offset);
			bumpNormal = mix(texture(bumpTex3, IN.texCoord).rgb, texture(bumpTex2, IN.texCoord).rgb, (IN.height - sandLayer.y) / offset);
		}
		else if (IN.height > stoneLayer.x && IN.height < stoneLayer.y){
			diffuse = texture(diffuseTex2, IN.texCoord);
			bumpNormal = texture(bumpTex2, IN.texCoord).rgb;
		}
		else if (IN.height > stoneLayer.y && IN.height < grassLayer.x) {
			diffuse = mix(texture(diffuseTex2, IN.texCoord),texture(diffuseTex, IN.texCoord), (IN.height - stoneLayer.y) / offset);
			bumpNormal = mix(texture(bumpTex2, IN.texCoord).rgb, texture(bumpTex, IN.texCoord).rgb, (IN.height - stoneLayer.y) / offset);
		}
		else if (IN.height > grassLayer.x && IN.height < grassLayer.y){
			diffuse = texture(diffuseTex, IN.texCoord);
			bumpNormal = texture(bumpTex, IN.texCoord).rgb;
		}
		else if (IN.height > grassLayer.y && IN.height < mountainLayer.x) {
			diffuse = mix(texture(diffuseTex, IN.texCoord),texture(diffuseTex2, IN.texCoord), (IN.height - grassLayer.y) / offset);
			bumpNormal = mix(texture(bumpTex, IN.texCoord).rgb, texture(bumpTex2, IN.texCoord).rgb, (IN.height - grassLayer.y) / offset);
		}
		else {
			diffuse = texture(diffuseTex2, IN.texCoord);
			bumpNormal = texture(bumpTex2, IN.texCoord).rgb;
		}
	}
	//None HeightMap Object
	else {
		diffuse = texture(diffuseTex, IN.texCoord);
		bumpNormal = texture(bumpTex, IN.texCoord).rgb;	
	}

	bumpNormal = normalize(TBN * normalize(bumpNormal * 2.0 - 1.0));
	
	float lambert = max(dot(incident, bumpNormal), 0.0f);
	float distance = length(lightPos - IN.worldPos);
	float attenuation = 1.0 - clamp(distance / lightRadius, 0.0, 1.0);

	float specFactor = clamp(dot(halfDir, bumpNormal), 0.0, 1.0);
	specFactor = pow(specFactor, 60.0);

	float shadow = 1.0;
	
	vec3 shadowNDC = IN.shadowProj.xyz / IN.shadowProj.w;
	if (abs(shadowNDC.x) < 1.0f && abs(shadowNDC.y) < 1.0f && abs(shadowNDC.z) < 1.0f) {
		vec3 biasCoord = shadowNDC * 0.5f + 0.5f;
		float shadowZ = texture(shadowTex, biasCoord.xy).x;
		if (shadowZ +0.0000001f < biasCoord.z) {
			shadow = 0.0f;
		}
	}

	vec3 surface = (diffuse.rgb * lightColour.rgb);
	fragColour.rgb = surface * lambert * attenuation;
	fragColour.rgb += (lightColour.rgb * specFactor) * attenuation * 0.33;
	fragColour.rgb *= shadow;
	fragColour.rgb += surface * 0.2f;
	fragColour.a = diffuse.a;
}