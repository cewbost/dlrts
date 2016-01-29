[[FX]]

Sampler2D heightMap;

float4 hmap_size;

context GENERAL
{
    VertexShader = compile GLSL VS_GENERAL;
    PixelShader = compile GLSL FS_GENERAL;
    CullMode = None;
    ZWriteEnable = false;
    ZEnable = false;
    BlendMode = Blend;
}

[[VS_GENERAL]]

attribute vec3 vertPos;
attribute vec3 normal;

uniform mat4 worldMat;
uniform mat4 viewProjMat;

uniform sampler2D heightMap;

uniform vec4 hmap_size;

varying vec3 normalPass;

const float height_multip = 5.0;

void main(void)
{
    vec3 pos;
	vec4 hmap_sample;

	pos = (worldMat * vec4(vertPos, 1.0)).xyz;

	hmap_sample = texture2D(heightMap, vec2(pos.x / hmap_size[0], pos.z / hmap_size[1]));
	pos.y = hmap_sample.b * height_multip;

	normalPass = normal;
	gl_Position = viewProjMat * vec4(pos, 1.0);
}

[[FS_GENERAL]]

varying vec3 normalPass;

void main(void)
{
    float closeness = max(normalPass.x, normalPass.y);
    closeness = max(closeness, normalPass.z);
    gl_FragColor = vec4(1., 0., 0., pow(closeness, 7) * .5);
    //gl_FragColor = vec4(1., 1., 1., 1.);
}
