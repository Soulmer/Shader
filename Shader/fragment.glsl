#version 460

in float tex;
uniform vec4 inColor;
uniform sampler2D inTex;
out vec4 color;

// MAIN ----------
void main () {
	color = vec4(texture(inTex, gl_PointCoord) * inColor);
	color *= vec4(1.0f, 1.0f, 1.0f, tex);
}