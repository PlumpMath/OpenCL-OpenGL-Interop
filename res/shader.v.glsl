//#version 420
//laptop only supports 330
//Actually is now able to support 400
#version 330

in vec3 position;
in vec3 texIn;
out vec2 uv;
uniform mat4 mvp;

void main(){
	uv = texIn.xy;
	gl_Position = mvp * vec4(position, 1.0);
}
