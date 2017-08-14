uniform mat4 u_mvp_mat;

attribute vec3 a_position;
attribute vec2 a_uvs;

varying vec2 v_uvs;

void main()
{
	gl_Position = u_mvp_mat * vec4(a_position, 1);
	v_uvs = a_uvs;
	v_uvs.y = 1.0 - v_uvs.y;
}