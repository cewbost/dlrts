
#include "utils.h"

namespace Utils
{

void pickRay( H3DNode cameraNode, float nwx, float nwy,
              float *ox, float *oy, float *oz,
              float *dx, float *dy, float *dz )
{
	// Transform from normalized window [0, 1]
	// to normalized device coordinates [-1, 1]
	float cx( 2.0f * nwx - 1.0f );
	float cy( 2.0f * nwy - 1.0f );

	// Get projection matrix
	Matrix4f projMat;
	h3dGetCameraProjMat( cameraNode, projMat.x );

	// Get camera view matrix
	const float *camTrans;
	h3dGetNodeTransMats( cameraNode, 0x0, &camTrans );
	Matrix4f viewMat( camTrans );
	viewMat = viewMat.inverted();

	// Create inverse view-projection matrix for unprojection
	Matrix4f invViewProjMat = (projMat * viewMat).inverted();

	// Unproject
	Vec4f p0 = invViewProjMat * Vec4f( cx, cy, -1, 1 );
	Vec4f p1 = invViewProjMat * Vec4f( cx, cy, 1, 1 );
	p0.x /= p0.w; p0.y /= p0.w; p0.z /= p0.w;
	p1.x /= p1.w; p1.y /= p1.w; p1.z /= p1.w;

	if( h3dGetNodeParamI( cameraNode, H3DCamera::OrthoI ) == 1 )
	{
		float frustumWidth = h3dGetNodeParamF( cameraNode,  
								H3DCamera::RightPlaneF, 0 ) -
		                     	h3dGetNodeParamF( cameraNode,  
		                     	H3DCamera::LeftPlaneF, 0 );
		float frustumHeight = h3dGetNodeParamF( cameraNode, 
								H3DCamera::TopPlaneF, 0 ) -
		                      	h3dGetNodeParamF( cameraNode, 
		                      	H3DCamera::BottomPlaneF, 0 );

		Vec4f p2( cx, cy, 0, 1 );

		p2.x = cx * frustumWidth * 0.5f;
		p2.y = cy * frustumHeight * 0.5f;
		viewMat.x[12] = 0; viewMat.x[13] = 0; viewMat.x[14] = 0;
		p2 = viewMat.inverted() * p2;

		*ox = camTrans[12] + p2.x;
		*oy = camTrans[13] + p2.y;
		*oz = camTrans[14] + p2.z;
	}
	else
	{
		*ox = camTrans[12];
		*oy = camTrans[13];
		*oz = camTrans[14];
	}
	*dx = p1.x - p0.x;
	*dy = p1.y - p0.y;
	*dz = p1.z - p0.z;
}

void pickRayNormalized( H3DNode cameraNode, float nwx, float nwy,
                        float *ox, float *oy, float *oz,
                        float *dx, float *dy, float *dz )
{
	// Transform from normalized window [0, 1]
	// to normalized device coordinates [-1, 1]
	float cx( 2.0f * nwx - 1.0f );
	float cy( 2.0f * nwy - 1.0f );

	// Get projection matrix
	Matrix4f projMat;
	h3dGetCameraProjMat( cameraNode, projMat.x );

	// Get camera view matrix
	const float *camTrans;
	h3dGetNodeTransMats( cameraNode, 0x0, &camTrans );
	Matrix4f viewMat( camTrans );
	viewMat = viewMat.inverted();

	// Create inverse view-projection matrix for unprojection
	Matrix4f invViewProjMat = (projMat * viewMat).inverted();

	// Unproject
	Vec4f p0 = invViewProjMat * Vec4f( cx, cy, -1, 1 );
	Vec4f p1 = invViewProjMat * Vec4f( cx, cy, 1, 1 );
	p0.x /= p0.w; p0.y /= p0.w; p0.z /= p0.w;
	p1.x /= p1.w; p1.y /= p1.w; p1.z /= p1.w;

	if( h3dGetNodeParamI( cameraNode, H3DCamera::OrthoI ) == 1 )
	{
		float frustumWidth = h3dGetNodeParamF( cameraNode,
								H3DCamera::RightPlaneF, 0 ) -
		                     	h3dGetNodeParamF( cameraNode,
		                     	H3DCamera::LeftPlaneF, 0 );
		float frustumHeight = h3dGetNodeParamF( cameraNode,
								H3DCamera::TopPlaneF, 0 ) -
		                      	h3dGetNodeParamF( cameraNode,
		                      	H3DCamera::BottomPlaneF, 0 );

		Vec4f p2( cx, cy, 0, 1 );

		p2.x = cx * frustumWidth * 0.5f;
		p2.y = cy * frustumHeight * 0.5f;
		viewMat.x[12] = 0; viewMat.x[13] = 0; viewMat.x[14] = 0;
		p2 = viewMat.inverted() * p2;

		*ox = camTrans[12] + p2.x;
		*oy = camTrans[13] + p2.y;
		*oz = camTrans[14] + p2.z;
	}
	else
	{
		*ox = camTrans[12];
		*oy = camTrans[13];
		*oz = camTrans[14];
	}

    Vec3f result = (p1.vec3f() - p0.vec3f()).normalized();

    /*p1.x -= p0.x;
    p1.y -= p0.y;
    p1.z -= p0.z;*/

	*dx = result.x;
	*dy = result.y;
	*dz = result.z;
}

__SET_NUMERATOR_DUMMY set_numerator;

}
