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

out vec4 fragColour[2];

void main(void) {
	mat3 TBN = mat3(normalize(IN.tangent), normalize(IN.binormal), normalize(IN.normal));

	vec4 diffuse;
	vec3 bumpNormal;

	int offset = 150;
		vec2 sandLayer = vec2(1,950);
		vec2 stoneLayer = vec2(sandLayer.y + offset, sandLayer.y + offset * 1.5);
		vec2 grassLayer = vec2(stoneLayer.y + offset, stoneLayer.y + offset * 2);
		vec2 mountainLayer = vec2(grassLayer.y + offset, 1000);

		if (IN.height < sandLayer.y) {
			fragColour[0] = texture2D(diffuseTex3, IN.texCoord);
			bumpNormal = texture2D(bumpTex3, IN.texCoord).rgb;
		}
		else if (IN.height > sandLayer.y && IN.height < stoneLayer.x) {
			fragColour[0] = mix(texture2D(diffuseTex3, IN.texCoord),texture2D(diffuseTex2, IN.texCoord), (IN.height - sandLayer.y) / offset);
			bumpNormal = mix(texture2D(bumpTex3, IN.texCoord).rgb, texture2D(bumpTex2, IN.texCoord).rgb, (IN.height - sandLayer.y) / offset);
		}
		else if (IN.height > stoneLayer.x && IN.height < stoneLayer.y){
			fragColour[0] = texture2D(diffuseTex2, IN.texCoord);
			bumpNormal = texture2D(bumpTex2, IN.texCoord).rgb;
		}
		else if (IN.height > stoneLayer.y && IN.height < grassLayer.x) {
			fragColour[0] = mix(texture2D(diffuseTex2, IN.texCoord),texture2D(diffuseTex, IN.texCoord), (IN.height - stoneLayer.y) / offset);
			bumpNormal = mix(texture2D(bumpTex2, IN.texCoord).rgb, texture2D(bumpTex, IN.texCoord).rgb, (IN.height - stoneLayer.y) / offset);
		}
		else if (IN.height > grassLayer.x && IN.height < grassLayer.y){
			fragColour[0] = texture2D(diffuseTex, IN.texCoord);
			bumpNormal = texture2D(bumpTex, IN.texCoord).rgb;
		}
		else if (IN.height > grassLayer.y && IN.height < mountainLayer.x) {
			fragColour[0] = mix(texture2D(diffuseTex, IN.texCoord),texture2D(diffuseTex2, IN.texCoord), (IN.height - grassLayer.y) / offset);
			bumpNormal = mix(texture2D(bumpTex, IN.texCoord).rgb, texture2D(bumpTex2, IN.texCoord).rgb, (IN.height - grassLayer.y) / offset);
		}
		else {
			fragColour[0] = texture2D(diffuseTex2, IN.texCoord);
			bumpNormal = texture2D(bumpTex2, IN.texCoord).rgb;
		}

	bumpNormal = normalize(TBN * normalize(bumpNormal * 2.0 - 1.0));

	//fragColour[0] = diffuse;
	fragColour[1] = vec4(bumpNormal.xyz * 0.5 + 0.5, 1.0);
}