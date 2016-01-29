[[FX]]

sampler2D albedoMap = sampler_state
{
	Address = Clamp;
	Filter = None;
};

context OVERLAY
{
	VertexShader = compile GLSL VS_BASIC;
	PixelShader = compile GLSL FS_BASIC;
	CullMode = None;
	BlendMode = Blend;
}

[[VS_BASIC]]

uniform mat4 projMat;
attribute vec2 vertPos;
attribute vec2 texCoords0;
varying vec2 tex_coords;

void main(void)
{
  tex_coords = vec2(texCoords0.s, texCoords0.t); 
  gl_Position = projMat * vec4(vertPos.x, vertPos.y, 1, 1);
}

[[FS_BASIC]]

uniform sampler2D albedoMap;
uniform vec4 olayColor;

varying vec2 tex_coords;

void main(void)
{
	vec2 tex_coords_final;

	tex_coords_final.x = fract(tex_coords.x) * (olayColor[1] - olayColor[0]) + olayColor[0];
	tex_coords_final.y = fract(tex_coords.y) * (olayColor[3] - olayColor[2]) + olayColor[2];
	
  vec4 albedo = texture2D(albedoMap, tex_coords_final);

  gl_FragColor = albedo;
}
