#version 330 core

uniform sampler2D sceneTex;

uniform int isVertical;

in Vertex {
	vec4 colour;
	vec2 texCoord;
	float height;
} IN;

out vec4 fragColour;

const float scaleFactors[7] = float[](0.06, 0.061, 0.242, 0.383, 0.242, 0.061, 0.006);

void main(void) {
	fragColour = vec4(0,0,0,1);

	vec2 delta = vec2(0,0);

	if(isVertical == 1) {
		delta = dFdy(IN.texCoord);
	}
	else {
		delta = dFdx(IN.texCoord);
	}

	for(int i = 0; i < 7; i++) {
		vec2 offset = delta * (i - 3);
		vec4 tmp = texture2D(sceneTex, IN.texCoord.xy + offset);
		fragColour += tmp * scaleFactors[i];
	}
}
