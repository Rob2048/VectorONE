precision mediump float;

uniform vec4 u_color;

varying vec4 v_color;

void main()
{
	gl_FragColor = v_color * u_color;
	//gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}