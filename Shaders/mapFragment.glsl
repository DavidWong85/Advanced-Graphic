#version 330 core

uniform sampler2D diffuseTex;
uniform sampler2D diffuseTex2;
uniform sampler2D diffuseTex3;

uniform vec3 cameraPos;

in Vertex {
	vec4 colour;	
	vec2 texCoord;
	float height;
} IN;

out vec4 fragColour;

void main(void) {
	int offset = 150;
	vec2 sandLayer = vec2(1,950);
	vec2 stoneLayer = vec2(sandLayer.y + offset, sandLayer.y + offset * 1.5);
	vec2 grassLayer = vec2(stoneLayer.y + offset, stoneLayer.y + offset * 2);
	vec2 mountainLayer = vec2(grassLayer.y + offset, 1000);

	if (IN.height < sandLayer.y) {
		fragColour = texture(diffuseTex3, IN.texCoord);
	}
	else if (IN.height > sandLayer.y && IN.height < stoneLayer.x) {
		fragColour = mix(texture(diffuseTex3, IN.texCoord),texture(diffuseTex2, IN.texCoord), (IN.height - sandLayer.y) / offset);
	}
	else if (IN.height > stoneLayer.x && IN.height < stoneLayer.y){
		fragColour = texture(diffuseTex2, IN.texCoord);
	}
	else if (IN.height > stoneLayer.y && IN.height < grassLayer.x) {
		fragColour = mix(texture(diffuseTex2, IN.texCoord),texture(diffuseTex, IN.texCoord), (IN.height - stoneLayer.y) / offset);
	}
	else if (IN.height > grassLayer.x && IN.height < grassLayer.y){
		fragColour = texture(diffuseTex, IN.texCoord);
	}
	else if (IN.height > grassLayer.y && IN.height < mountainLayer.x) {
		fragColour = mix(texture(diffuseTex, IN.texCoord),texture(diffuseTex2, IN.texCoord), (IN.height - grassLayer.y) / offset);
	}
	else {
		fragColour = texture(diffuseTex2, IN.texCoord);
	}
}