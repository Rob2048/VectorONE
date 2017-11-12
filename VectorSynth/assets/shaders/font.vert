attribute vec3 a_position;
attribute vec2 a_uvs;
attribute vec4 a_color;

varying vec2 v_uvs;
varying vec4 v_color;

void main()
{
	vec2 pos = a_position.xy * vec2(2, 2) - vec2(1, -1);	
	gl_Position = vec4(pos, 0.0, 1.0);
	v_uvs = a_uvs;
	v_color = a_color;
}