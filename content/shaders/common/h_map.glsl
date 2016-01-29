
//heightmap transformation shader

/*
  the following flags can be used:
    TS_LIGHT_VEC
      - passes a light-vector in tangent space through varying vec3 lightVec
      - requires the world-space light vector to be passed in uniform vec3 light
      - sets NORMAL_INFO flag
      - THIS FLAG IS CURRENTLY FUCKED
    TS_EYE_VEC
      - passes an eye-vector in tangent space through varying vec3 eyeVec
      - requires the world-space viewer position in uniform vec3 viewerPos
      - set NORMAL_INFO flag
    NORMAL_INFO
      - passes normal information through varying vec3 variables: worldNormal,
        worldBinormal, worldTangent
    OVERWRITE_Y
      - produces y-coordinate by adding height-map sample to model-space y-coordinate
      - no world-space transformation applied to y-coordinate
*/

uniform sampler2D heightMap;

uniform vec4 hmap_size;

uniform mat4 viewProjMat;
uniform mat4 worldMat;

attribute vec3 vertPos;
attribute vec2 texCoords0;
attribute vec2 texCoords1;

#ifdef TS_LIGHT_VEC
#define NORMAL_INFO
uniform vec3 light;
varying vec3 lightVec;
#endif
#ifdef TS_EYE_VEC
#define NORMAL_INFO
uniform vec3 viewerPos;
varying vec3 eyeVec;
#endif

varying vec3 worldCoords;
#ifdef NORMAL_INFO
varying vec3 worldNormal;
varying vec3 worldBinormal;
varying vec3 worldTangent;
#endif

varying vec2 _texCoords0;
varying vec2 _texCoords1;

const float c_diff = 1.0 / 12.0;
const float height_multip = 5.0;
const float noise_freq = 30.0;

void main(void)
{
	vec3 pos;
	
	vec4 hmap_sample;
  
#ifdef NORMAL_INFO
  vec3 normal;
	vec3 tangent;
	vec3 binormal;
#endif
	
	pos = (worldMat * vec4(vertPos, 1.0)).xyz;
  
	hmap_sample = texture2D(heightMap, vec2(pos.x / hmap_size[0], pos.z / hmap_size[1]));
#ifdef OVERWRITE_Y
  pos.y = vertPos.y + hmap_sample.b * height_multip;
#else
	pos.y += hmap_sample.b * height_multip;
#endif

	worldCoords = pos;

	//normal & tangent & binormal
#ifdef NORMAL_INFO
	tangent = normalize(vec3(-0.25, (hmap_sample.r - 0.5) * height_multip, 0.0));
	binormal = normalize(vec3(0.0, (hmap_sample.g - 0.5) * height_multip, -0.25));
	normal = cross(binormal, tangent);
  if(normal.y > 0.95) normal = vec3(0.0, 1.0, 0.0);
	worldNormal = normal;
  worldBinormal = binormal;
  worldTangent = tangent;
#endif
  
	//light vector
#ifdef TS_LIGHT_VEC
	lightVec.x = dot(light, tangent);
	lightVec.y = dot(light, binormal);
	lightVec.z = dot(light, normal);
#endif

	//eye vector
#ifdef TS_EYE_VEC
	pos2 = normalize(pos - viewerPos);
	
	eyeVec.x = dot(pos2, tangent);
	eyeVec.y = dot(pos2, binormal);
	eyeVec.z = dot(pos2, normal);
#endif
  
	//output
	_texCoords0 = texCoords0;
	_texCoords1 = texCoords1;
	
	gl_Position = viewProjMat * vec4(pos, 1.0);

	return;
}
