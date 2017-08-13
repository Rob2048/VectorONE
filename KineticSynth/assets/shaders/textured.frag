precision mediump float;

uniform vec4 u_color;
uniform sampler2D texMain;

varying vec2 v_uvs;

void main()
{
	vec4 diffuse = texture2D(texMain, vec2(v_uvs));
	float alpha = diffuse.r * u_color.a;

	gl_FragColor = vec4(u_color.rgb * alpha, alpha);
}
