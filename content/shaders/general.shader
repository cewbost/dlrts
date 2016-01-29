[[FX]]

sampler2D albedoMap;
sampler2D heightMap;
sampler2D topTex;
sampler2D wallTex;
sampler2D topNormalMap;
sampler2D wallNormalMap;
sampler2D normalMap;
sampler2D visionMap;

float4 hmap_size;
float4 vision_phase;
float4 wavePos;
float4 HiliteColor;

context HEIGHT_MAP
{
	VertexShader = compile GLSL VS_H_MAP;
	PixelShader = compile GLSL FS_TEST_LIGHT;
	CullMode = None;
}

context WATER
{
	VertexShader = compile GLSL VS_PLANE;
	PixelShader = compile GLSL FS_WATER;
	BlendMode = Blend;
  //ZEnable = false;
}


[[VS_PLANE]]

uniform mat4 viewProjMat;
uniform mat4 worldMat;

uniform vec4 wavePos;

uniform vec3 viewerPos;

attribute vec3 vertPos;
attribute vec3 normal;
attribute vec2 texCoords0;

varying vec2 tex_coords;
varying vec3 lightVec;
varying vec3 eyeVec;
varying vec2 planeCoords;

const float PI = 3.1415926;

void main(void)
{
	vec4 pos;
	vec4 pos2;

	pos = worldMat * vec4(vertPos, 1.0);
	pos.y += sin(wavePos.x * PI * 2.0 + pos.x) * 0.1;

	//normal & tangent & binormal
	vec3 normal, tangent, binormal;

	normal = vec3(0.0, 1.0, 0.0);
	tangent = vec3(1.0, 0.0, 0.0);
	binormal = vec3(0.0, 0.0, 1.0);

	//light vector
	vec3 light = normalize(vec3(0.3, -0.5, 0.3));
	lightVec.x = dot(light, tangent);
	lightVec.y = dot(light, binormal);
	lightVec.z = dot(light, normal);

	pos2 = normalize(pos - vec4(viewerPos, 1.0));

	eyeVec.x = dot(pos2, vec4(tangent, 1.0));
	eyeVec.y = dot(pos2, vec4(binormal, 1.0));
	eyeVec.z = dot(pos2, vec4(normal, 1.0));

	//output
	gl_Position = viewProjMat * pos;

	tex_coords = texCoords0;
	planeCoords = pos.xz;

	return;
}



[[FS_WATER]]

uniform sampler2D heightMap;
uniform sampler2D normalMap;

uniform vec4 hmap_size;
uniform vec4 wavePos;

varying vec2 tex_coords;
varying vec3 lightVec;
varying vec3 eyeVec;

varying vec2 planeCoords;

const float shine = 8.0;
const float PI = 3.1415926;
const float noise_freq = 20.0;

void main(void)
{
	//lighting calculations
	vec3 normal = texture2D(normalMap, tex_coords + vec2(wavePos.x, wavePos.x)).rgb;
	vec3 normal2 = texture2D(normalMap, tex_coords + vec2(wavePos.x, -wavePos.x) + vec2(0.2, 0.0)).rgb;
	normal = normal * 2.0 - vec3(1.0, 1.0, 1.0);
	normal2 = normal2 * 2.0 - vec3(1.0, 1.0, 1.0);
	normal.x += normal2.x;
	normal.y += normal2.y;
	normal = normalize(normal);

	//normal = vec3(0.0, 0.0, 1.0);

	float lv;
	float spec;

	vec4 tex_col = vec4(0.0, 0.5, 1.0, 0.3);

	//dyningar
	vec2 coords2 = planeCoords;
	//coords2 -= vec2(0.375, 0.375);
	//coords2 += (texture2D(noiseMap, vec2(fract(planeCoords.x / noise_freq),
	//	fract(planeCoords.y / noise_freq))).rg - vec2(0.5, 0.5)) / 4;

	//float depth = clamp(texture2D(heightMap,
	//	vec2(coords2.x / hmap_size.x, coords2.y / hmap_size.y)).b, 0.0, 0.25) * 8 - 1.0;
	//depth *= (sin(depth * 20 + wavePos.x * PI * 4) + 1);
	//if(depth < 0.0) depth = 0.0;
	//else depth /= 2;

	float depth = clamp(texture2D(heightMap,
		vec2(coords2.x / hmap_size.x, coords2.y /
			hmap_size.y)).b * 3.0 - 0.2, 0.0, 1.0);
	depth *= clamp(sin(depth * 20.0 + wavePos.x * PI * 8.0) + 1.0, 0.0, 0.8);

	tex_col.r = clamp(tex_col.r + depth, 0.0, 1.0);
	tex_col.g = clamp(tex_col.g + depth, 0.0, 1.0);
	tex_col.a = clamp(tex_col.a + depth, 0.0, 1.0);

	//light contribution
	lv = -dot(normalize(lightVec), normal);
	spec = pow(clamp(-dot(reflect(lightVec, normal), eyeVec), 0.0, 1.0), shine);
	spec *= 0.8;

	lv = clamp(lv, 0.0, 1.0);

	tex_col.r *= lv;
	tex_col.g *= lv;
	tex_col.b *= lv;
	//add specular hilights
	tex_col += vec4(spec, spec, spec, spec);

	gl_FragColor = tex_col;
	gl_FragDepth = gl_FragCoord.z;
}



[[VS_H_MAP]]

#define TS_LIGHT_VEC
#define TS_EYE_VEC

#include "h_map.glsl"

[[FS_TEST_LIGHT]]

uniform sampler2D topTex;
uniform sampler2D wallTex;
uniform sampler2D topNormalMap;
uniform sampler2D wallNormalMap;
uniform sampler2D visionMap;

uniform vec4 hmap_size;
uniform vec4 vision_phase;

varying vec3 lightVec;
varying vec3 eyeVec;
varying vec3 worldCoords;
varying vec3 worldNormal;

const float shine = 8.0;

void main(void)
{
	vec4 tex_col1 = texture2D(topTex, worldCoords.xz);
	vec4 tex_col2 = texture2D(wallTex, worldCoords.xy);
	vec4 tex_col3 = texture2D(wallTex, worldCoords.yz);

	vec3 normal1 = texture2D(topNormalMap, worldCoords.xz).rgb;
	normal1 = normalize(normal1 * 2.0 - vec3(1.0, 1.0, 1.0));
	vec3 normal2 = texture2D(wallNormalMap, worldCoords.xy).rgb;
	normal2 = normalize(normal2 * 2.0 - vec3(1.0, 1.0, 1.0));
	vec3 normal3 = texture2D(wallNormalMap, worldCoords.yz).rgb;
	normal3 = normalize(normal3 * 2.0 - vec3(1.0, 1.0, 1.0));

	float wall;
	float lv;
	float spec;

	//triplanar texturing
	vec3 planeWeight = worldNormal;
	if(planeWeight.x < 0.0) planeWeight.x *= -1.0;
	if(planeWeight.y < 0.0) planeWeight.y *= -1.0;
	if(planeWeight.z < 0.0) planeWeight.z *= -1.0;
	planeWeight /= (planeWeight.x + planeWeight.y + planeWeight.z);

	tex_col1 *= planeWeight.y;
	tex_col1 += tex_col2 * planeWeight.x;
	tex_col1 += tex_col3 * planeWeight.z;

	normal1 *= planeWeight.y;
	normal1 += normal2 * planeWeight.x;
	normal1 += normal3 * planeWeight.z;

	//activate this shit to make blue terrain
	//tex_col1 = vec4(0.0, 0.0, 1.0, 1.0);
	//vec3 normal1 = vec3(0.0, 0.0, 1.0);

	lv = -dot(normalize(lightVec), normal1);
	//this shit is for specular hilights
	//spec = pow(clamp(dot(reflect(lightVec, normal1), -eyeVec), 0.0, 1.0), shine);
	//spec *= 0.15/* * (1.0 - tex * 0.6) * (1.0 - tex * 0.5)*/;

	lv = clamp(lv, 0.0, 1.0);

	tex_col1 *= lv;

	//add specular hilights
	//tex_col1 += vec4(spec, spec, spec, 0.0);

	//darken under water
	tex_col1 *= clamp(worldCoords.y, 0.0, 1.0);

  //darken out of view
  vec2 vision = texture2D(visionMap,
                vec2(worldCoords.x / hmap_size[0],
                      worldCoords.z / hmap_size[1])).bg;
  vision.x = max(vision.x, vision.y);
  vision.x = clamp(vision.x * 10.0 - 4.5, 0.0, 1.0);
  
  tex_col1 *= 0.01;//vision.x;
  
	gl_FragColor = tex_col1;
	gl_FragDepth = gl_FragCoord.z;

	return;
}
