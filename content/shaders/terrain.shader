[[FX]]

sampler2D albedoMap;
sampler2D heightMap;
sampler2D topTex;
sampler2D wallTex;
sampler2D topNormalMap;
sampler2D wallNormalMap;
sampler2D normalMap;
sampler2D visionMap = sampler_state
{
  Filter = Bilinear;
};

float4 hmap_size;
float4 vision_phase;
float4 wavePos;
float4 HiliteColor;

context ATTRIB_PASS
{
  VertexShader = compile GLSL VS_AP;
  PixelShader = compile GLSL FS_AP;
  CullMode = None;
}

[[VS_AP]]

#ifndef _F01_NoTPT
#define NORMAL_INFO
#endif
#include "h_map.glsl"

[[FS_AP]]

uniform sampler2D topTex;
uniform sampler2D topNormalMap;

#ifndef _F01_NoTPT
uniform sampler2D wallTex;
uniform sampler2D wallNormalMap;
#endif

uniform sampler2D visionMap;

uniform vec4 hmap_size;
uniform vec4 vision_phase;

varying vec3 worldCoords;

#ifndef _F01_NoTPT
varying vec3 worldNormal;
varying vec3 worldBinormal;
varying vec3 worldTangent;
#endif

const float shine = 8.0;

void main(void)
{
	vec4 tex_col1 = texture2D(topTex, worldCoords.xz);
  vec3 normal1 = texture2D(topNormalMap, worldCoords.xz).rgb;
  normal1 = normalize(normal1 * 2.0 - vec3(1.0, 1.0, 1.0));
#ifndef _F01_NoTPT
  vec4 tex_col2 = texture2D(wallTex, worldCoords.zy);
  vec4 tex_col3 = texture2D(wallTex, worldCoords.xy);
  vec3 normal2 = texture2D(wallNormalMap, worldCoords.zy).rgb;
  normal2 = normalize(normal2 * 2.0 - vec3(1.0, 1.0, 1.0));
  vec3 normal3 = texture2D(wallNormalMap, worldCoords.xy).rgb;
  normal3 = normalize(normal3 * 2.0 - vec3(1.0, 1.0, 1.0));

	//triplanar texturing
	vec3 planeWeight = worldNormal;
  planeWeight.x = abs(planeWeight.x);
  planeWeight.y = abs(planeWeight.y);
  planeWeight.z = abs(planeWeight.z);
	planeWeight /= (planeWeight.x + planeWeight.y + planeWeight.z);

	tex_col1 *= planeWeight.y;
	tex_col1 += tex_col2 * planeWeight.x;
	tex_col1 += tex_col3 * planeWeight.z;
  
	normal1 *= planeWeight.y;
	normal1 += normal2 * planeWeight.x;
	normal1 += normal3 * planeWeight.z;

  //compose world normal
  vec3 w_normal = worldNormal * normal1.b;
  w_normal += worldBinormal * normal1.r;
  w_normal += worldTangent * normal1.g;
  w_normal = normalize(w_normal);
#else
  vec3 w_normal = vec3(-normal1.g, normal1.b, -normal1.r);
#endif
  
	//darken under water
	tex_col1 *= clamp(worldCoords.y, 0.0, 1.0);

  //darken out of view
  vec2 vision = texture2D(visionMap,
                vec2(worldCoords.x / hmap_size[0],
                      worldCoords.z / hmap_size[1])).bg;
  vision.x = max(vision.x, vision.y);
  //vision.x = vision.y;
  vision.x = clamp(vision.x * 4.0 - 1.5, 0.0, 1.0);
  
  tex_col1 *= vision.x;

  gl_FragData[0] = tex_col1;
  gl_FragData[1] = vec4(worldCoords, 1.0);
  gl_FragData[2] = vec4(w_normal, 1.0);
  gl_FragDepth = gl_FragCoord.z;

	return;
}
