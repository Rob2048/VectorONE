uniform mat4 u_mvp_mat;

attribute vec3 a_position;

void main()
{
	gl_Position = u_mvp_mat * vec4(a_position, 1);
}