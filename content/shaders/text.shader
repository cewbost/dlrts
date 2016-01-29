[[FX]]

sampler2D albedoMap;

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
attribute vec2 texCoords0;
varying vec2 tex_coords;

void main(void)
{
    tex_coords = vec2(texCoords0.s, -texCoords0.t); 
    gl_Position = projMat * vec4(vertPos.x, vertPos.y, 1, 1);
}

[[FS_OVERLAY]]

uniform sampler2D albedoMap;
varying vec2 tex_coords;
uniform vec4 olayColor;

void main(void)
{
	if(gl_FragCoord.x < olayColor.x ||
		gl_FragCoord.x > olayColor.y ||
		gl_FragCoord.y < olayColor.z ||
		gl_FragCoord.y > olayColor.w)
	discard;

    vec4 albedo = texture2D(albedoMap, vec2(tex_coords.x, -tex_coords.y));
	
	albedo.a = smoothstep(0.5, 0.6, albedo.a);
	albedo.r = albedo.g = albedo.b = 0.0;	
	
    gl_FragColor = albedo;
}
