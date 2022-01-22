#version 330 core

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;

in vec3 position;
in vec4 colour;
in vec2 texCoord;

out Vertex {
	vec4 colour;
	vec2 texCoord;
	float height;
} OUT;

void main(void) {
	OUT.colour = colour;
	OUT.texCoord = texCoord;

	vec4 worldPos = (modelMatrix * vec4(position, 1));
	gl_Position = (projMatrix * viewMatrix) * worldPos;
	
	OUT.height = worldPos.y;
}