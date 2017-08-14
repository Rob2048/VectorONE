precision mediump float;

uniform vec4 u_color;
uniform sampler2D texMain;

varying vec2 v_uvs;
varying vec3 v_normal;
varying vec3 v_world_pos;

void main()
{
	vec4 diffuse = texture2D(texMain, vec2(v_uvs));

	vec3 lightPos = vec3(-10.0, -10.0, 10.0);
	vec3 lightDir = normalize(lightPos - v_world_pos);
	float lightIntensity = max(dot(v_normal, lightDir), 0.0);
	//vec3 d = lightIntensity * v_color * u_color.rgb;
	//vec3 a = 0.3 * v_color * u_color.rgb;

	//vec3 surfaceColor = u_color.rgb;
	vec3 surfaceColor = diffuse.rgb;

	vec3 d = lightIntensity * surfaceColor * 0.5;
	vec3 a = 0.7 * surfaceColor;

	gl_FragColor = vec4(surfaceColor, 1.0);
	//gl_FragColor = vec4(min(d + a, 1.0), 1.0);

	//gl_FragColor = vec4(v_uvs, 0.0, 1.0);
	//gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}