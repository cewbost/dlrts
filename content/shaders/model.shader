[[FX]]

context ATTRIB_PASS
{
  VertexShader = compile GLSL VS_AP;
  PixelShader = compile GLSL FS_AP;
  CullMode = None;
}

[[VS_AP]]

attribute vec3 vertPos;
attribute vec3 normal;

uniform mat4 worldMat;
uniform mat3 worldNormalMat;

uniform mat4 viewProjMat;

varying vec4 worldPos;
varying vec3 worldNormal;

void main(void)
{
  worldPos = worldMat * vec4(vertPos, 1.0);
  gl_Position = viewProjMat * worldPos;
  worldNormal = worldNormalMat * normal;
}

[[FS_AP]]

varying vec4 worldPos;
varying vec3 worldNormal;

void main(void)
{
  gl_FragData[0] = vec4(1.0, 0.0, 0.0, 1.0);
  gl_FragData[1] = worldPos;
  gl_FragData[2] = vec4(worldNormal, 1.0);
  gl_FragDepth = gl_FragCoord.z;
}
