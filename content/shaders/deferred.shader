[[FX]]

sampler2D gColorMap = sampler_state
{
  Filter = None;
};
sampler2D gCoordMap = sampler_state
{
  Filter = None;
};
sampler2D gNormalMap = sampler_state
{
  Filter = None;
};

sampler2D gDepthBuf = sampler_state
{
  Filter = None;
};

context AMBIENT
{
  VertexShader = compile GLSL VS_FSQUAD;
  PixelShader = compile GLSL FS_AMBIENT;
  //ZEnable = false;
}

context LIGHTING
{
  VertexShader = compile GLSL VS_VOLUME;
  PixelShader = compile GLSL FS_LIGHTING;
  BlendMode = Add;
  ZWriteEnable = false;
}

[[VS_FSQUAD]]

uniform mat4 projMat;
attribute vec3 vertPos;
varying vec2 texCoords;

void main( void )
{
	texCoords = vertPos.xy; 
	gl_Position = projMat * vec4( vertPos, 1 );
}


[[VS_VOLUME]]

uniform mat4 viewProjMat;
uniform mat4 worldMat;
attribute vec3 vertPos;
varying vec4 vpos;

void main( void )
{
  vpos = viewProjMat * worldMat * vec4(vertPos, 1);
  gl_Position = vpos;
}


[[FS_AMBIENT]]

uniform sampler2D gColorMap;
uniform sampler2D gCoordMap;
uniform sampler2D gNormalMap;
uniform sampler2D gDepthBuf;

varying vec2 texCoords;

void main(void)
{
  //vec2 coord = vec2(gl_FragCoord.x / screenDim.x, gl_FragCoord.y / screenDim.y);
  vec4 color = texture2D(gColorMap, texCoords);
  float dot_p = dot(texture2D(gNormalMap, texCoords).xyz, vec3(0.0, 1.0, 0.0));
  gl_FragColor = color * (dot_p * 0.2 + 0.2);
  gl_FragDepth = texture2D(gDepthBuf, texCoords).r;
}

[[FS_LIGHTING]]

uniform sampler2D gColorMap;
uniform sampler2D gCoordMap;
uniform sampler2D gNormalMap;
uniform sampler2D gDepthBuf;

uniform vec3 viewerPos;
uniform vec4 lightPos;
uniform vec3 lightColor;
uniform vec4 lightDir;

varying vec4 vpos;

void main(void)
{
  //vec3 lightPos = vec3(0.0, 0.0, 0.0);
  //vec3 lightPos = vec3(8.0, 4.0, 8.0);
  //vec3 lightColor = vec3(1.0, 1.0, 1.0);
  
  vec2 coord = (vpos.xy / vpos.w) * 0.5 + 0.5;
  
  vec3 color = texture2D(gColorMap, coord).rgb;
  vec3 pos = texture2D(gCoordMap, coord).xyz;
  vec3 normal = texture2D(gNormalMap, coord).xyz;
  
  vec3 lightVec = lightPos.xyz - pos + vec3(0.0, 4.0, 0.0);
  
  float tsLightZ = dot(lightVec, normal);
  if(tsLightZ < 0.0) discard;
  
  float alpha = 1.0 - dot(lightVec.xz, lightVec.xz) / 25.0;
  if(alpha < 0.0) discard;
  alpha *= 30.0 / dot(lightVec, lightVec);
  
  //this might be reintroduced for specular hilights
  //vec3 eyeVecNorm = normalize(pos - viewerPos);
  //vec3 tangent = cross(normal, eyeVecNorm);
  //vec3 binormal = cross(normal, tangent);
  
  //vec3 tsEyeVec;
  //vec3 tsLightVec;
  //vec3 lightVecNorm = normalize(lightVec);
  //tsLightVec.z = dot(lightVecNorm, normal);
  //tsLightVec.x = dot(lightVecNorm, tangent);
	//tsLightVec.y = dot(lightVecNorm, binormal);
  
  color.r *= lightColor.r;
  color.g *= lightColor.g;
  color.b *= lightColor.b;
  
  alpha *= tsLightZ;
  
  color *= alpha;
  
  gl_FragColor = vec4(color, 1.0);
}
