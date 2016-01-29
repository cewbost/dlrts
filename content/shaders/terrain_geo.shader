[[FX]]

sampler2D topTex;
sampler2D wallTex;
sampler2D topNormalMap;
sampler2D wallNormalMap;

context HEIGHT_MAP
{
	VertexShader = compile GLSL VS_H_MAP;
	PixelShader = compile GLSL FS_TEST_LIGHT;
	CullMode = None;
}


[[VS_H_MAP]]

/*
	This shader only changes vertex positions.
	Texture coordinates are passed straight through.
*/
/*
uniform sampler2D heightMap;
uniform sampler2D noiseMap;
*/
uniform vec4 hmap_size;

uniform vec3 viewerPos;

uniform mat4 viewProjMat;
uniform mat4 worldMat;
uniform mat3 worldNormalMat;

attribute vec3 vertPos;
attribute vec3 normal;

varying vec3 lightVec;
varying vec3 eyeVec;
varying vec3 space_coords;
varying float height;

const float c_diff = 1.0 / 12;
const float height_multip = 1.0;
const float noise_freq = 30.0;

void main(void)
{
	//float hmap_val;
	vec3 pos;
	vec3 pos2;
	vec3 world_normal;
	vec3 world_tangent;
	vec3 world_binormal;
	
	//position
	pos = (worldMat * vec4(vertPos, 1.0)).xyz;
	
	//normal & tangent & binormal
	//world_normal = (worldNormalMat * vec4(normal, 1.0)).xyz;
	//world_tangent = (worldNormalMat * vec4(0.0, normal.z, -normal.y, 1.0)).xyz;
	//world_tangent = (worldNormalMat * vec4(1.0, 0.0, 0.0, 1.0)).xyz;
	//world_binormal = cross(world_normal, world_tangent);
	
	world_normal = worldNormalMat * -normal;
	world_tangent = worldNormalMat * vec3(1.0, 0.0, 0.0);
	world_binormal = cross(world_normal, world_tangent);
	
	//light vector
	vec3 light = normalize(vec3(0.3, 0.3, -0.5));
	lightVec.x = dot(light, world_tangent);
	lightVec.y = dot(light, world_binormal);
	lightVec.z = dot(light, world_normal);
	
	//eye vector
	pos2 = normalize(pos[0] - viewerPos);
	eyeVec.x = dot(pos2, world_tangent);
	eyeVec.y = dot(pos2, world_binormal);
	eyeVec.z = dot(pos2, world_normal);

	//output
	gl_Position = viewProjMat * vec4(pos, 1.0);
	
	space_coords = pos;

	return;
}

[[FS_TEST_LIGHT]]

/*
void main(void)
{
	vec3 normal = vec3(0.0, 0.0, 1.0);

	tex_col = vec4(0.0, 0.5, 0.0, 1.0);
	
	lv = -dot(normalize(lightVec), normal);
	spec = pow(clamp(dot(reflect(lightVec, normal), -eyeVec), 0.0, 1.0), shine);
	spec *= 0.15;
	
	lv = clamp(lv, 0.0, 1.0);
	
	tex_col *= lv;
	
	//add specular hilights
	//tex_col += vec4(spec, spec, spec, 0.0);

	gl_FragColor = tex_col;
	gl_FragDepth = gl_FragCoord.z;

	return;
}
*/

uniform sampler2D topTex;
uniform sampler2D wallTex;
uniform sampler2D topNormalMap;
uniform sampler2D wallNormalMap;

uniform sampler2D noiseMap;

varying vec3 lightVec;
varying vec3 eyeVec;
varying vec3 space_coords;
varying float height;

const float shine = 8.0;

void main(void)
{
	vec2 tex_coords = vec2(fract(space_coords.x), fract(space_coords.y));
	
	//lighting calculations
	vec3 normal1 = texture2D(topNormalMap, tex_coords).rgb;
	normal1 = normalize(normal1 * 2 - vec3(1.0, 1.0, 1.0));
	vec3 normal2 = texture2D(wallNormalMap, tex_coords).rgb;
	normal2 = normalize(normal2 * 2 - vec3(1.0, 1.0, 1.0));

	float lv;
	float spec;
	vec4 tex_col1;
	vec4 tex_col2;

	tex_col1 = texture2D(topTex, space_coords.xy);
	tex_col2 = texture2D(wallTex, space_coords.xy);
	
	//tex_col1 = vec4(0.4, 0.4, 0.4, 1.0);

	spec = clamp(spec * 2 - 0.5, 0.0, 1.0);
	//tex_col1 = (tex_col1 * spec) + tex_col2 * (1.0 - spec);
	//normal1 = (normal1 * spec) + normal2 * (1.0 - spec);
	normal1 = normal2;
	
	spec = fract(space_coords.z * 4) > 0.35 && fract(space_coords.z * 4) < 0.65? 0.1 : 0.15;
	//spec = 0.5;
	
	tex_col1 = vec4(spec + 0.2, spec + 0.1, spec, 1.0);
	//normal1 = vec3(0.0, 0.0, 1.0);
	
	lv = dot(normalize(lightVec), normal1);
	spec = pow(clamp(dot(reflect(lightVec, normal1), -eyeVec), 0.0, 1.0), shine);
	spec *= 0.15;
	
	lv = clamp(lv, 0.0, 1.0);
	
	//green color terrain
	//tex_col1 = vec4(0.35, 0.25, 0.25, 1.0);
	
	tex_col1 *= lv;
	
	//add specular hilights
	tex_col1 += vec4(spec, spec, spec, 0.0);

	gl_FragColor = tex_col1;
	gl_FragDepth = gl_FragCoord.z;

	return;
}

