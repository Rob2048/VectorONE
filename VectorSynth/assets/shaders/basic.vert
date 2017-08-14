uniform mat4 mvp_matrix;

attribute vec3 a_position;
attribute vec4 a_color;

varying vec4 v_color;

void main()
{
	gl_PointSize = 3.0;
	gl_Position = mvp_matrix * vec4(a_position, 1);
	v_color = a_color;
}