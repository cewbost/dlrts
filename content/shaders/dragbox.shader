[[FX]]

context OVERLAY
{
	VertexShader = compile GLSL VS_OVERLAY;
	PixelShader = compile GLSL FS_OVERLAY;
	CullMode = None;
	BlendMode = Blend;
}

[[VS_OVERLAY]]

uniform mat4 projMat;
attribute vec2 vertPos;

void main()
{
	gl_Position = projMat * vec4(vertPos.x, vertPos.y, 1, 1);
}

[[FS_OVERLAY]]

uniform vec4 olayColor;

void main()
{
	vec4 col = vec4(0.0, 1.0, 0.5, 0.1);
	
	if(abs(gl_FragCoord.x - olayColor.r) < 1.0)
		col = vec4(0.2, 1.0, 0.6, 0.3);
	if(abs(gl_FragCoord.x - olayColor.g) < 1.0)
		col = vec4(0.2, 1.0, 0.6, 0.3);
	if(abs(gl_FragCoord.y - olayColor.b) < 1.0)
		col = vec4(0.2, 1.0, 0.6, 0.3);
	if(abs(gl_FragCoord.y - olayColor.a) < 1.0)
		col = vec4(0.2, 1.0, 0.6, 0.3);
		
	gl_FragColor = col;
}
