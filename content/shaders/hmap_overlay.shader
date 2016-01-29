[[FX]]

Sampler2D heightMap;

float4 hmap_size;
float4 HiliteColor;

context GENERAL
{
	VertexShader = compile GLSL VS_H_MAP;
	PixelShader = compile GLSL FS_OVERLAY;
	BlendMode = Blend;
	ZWriteEnable = false;
	ZEnable = false;
}

[[VS_H_MAP]]

#define OVERWRITE_Y
#include "h_map.glsl"

[[FS_OVERLAY]]

uniform vec4 HiliteColor;

varying vec2 _texCoords0;
varying vec2 _texCoords1;

void main(void)
{
	float alpha_multiplier = 1.0;

#ifdef _F01_Radial
	vec2 texCoords = _texCoords0 - vec2(0.5, 0.5);
	float rad_val = texCoords.x * texCoords.x + texCoords.y * texCoords.y;
	
	if(rad_val > 0.25) discard;

#ifdef _F02_Circle
	alpha_multiplier = clamp(rad_val * 8.0 - 1.0, 0.0, 1.0);
#endif
#endif

    gl_FragColor = vec4(HiliteColor.rgb, HiliteColor.a * alpha_multiplier);
}
