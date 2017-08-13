precision mediump float;

uniform vec4 u_color;

void main()
{
	gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0) * u_color;
}