#version 460

in vec4 pos;
out float tex;

// MAIN ----------
void main () {
	vec3 newPos = pos.xyz;
	float newTex = pos.w;
	gl_Position = vec4(newPos, 1.0);
	tex = newTex;
}