uniform mat4 u_mvp_mat;
uniform mat4 u_model_mat;

attribute vec3 a_position;
attribute vec2 a_uvs;
attribute vec3 a_normal;

varying vec2 v_uvs;
varying vec3 v_normal;
varying vec3 v_world_pos;

void main()
{
	gl_Position = u_mvp_mat * vec4(a_position, 1);
	v_uvs = a_uvs;
	v_uvs.y = 1.0 - v_uvs.y;
	v_normal = normalize((u_model_mat * vec4(a_normal, 0)).xyz);
	v_world_pos = (u_model_mat * vec4(a_position, 1)).xyz;
}