precision mediump float;

uniform vec4 u_color;
uniform sampler2D texMain;

varying vec2 v_uvs;
varying vec4 v_color;

void main()
{
	vec4 diffuse = texture2D(texMain, vec2(v_uvs));
	float alpha = diffuse.r * u_color.a * v_color.a;

	//gl_FragColor = vec4(v_color.rgb, 1);	
	//gl_FragColor = vec4(alpha, alpha, alpha, 1);
	gl_FragColor = vec4(u_color.rgb * v_color.rgb, alpha);
}
