#include <GL/glew.h>
#include "Light.h"

using namespace simul;

namespace
{

    const float DEFAULT_LIGHT_POSITION[]				={0.0f, 0.0f, 0.0f, 1.0f};
    const float DEFAULT_DIRECTION_LIGHT_POSITION[]	={0.0f, 0.0f, 1.0f, 0.0f};
    const float DEFAULT_SPOT_LIGHT_DIRECTION[]		={0.0f, 0.0f, -1.0f};
    const float DEFAULT_LIGHT_COLOR[]					={1.0f, 1.0f, 1.0f, 1.0f};
    const float DEFAULT_LIGHT_SPOT_CUTOFF				=180.0f;
}


opengl::Light::Light()// : crossplatform::Light()
{
    mLightIndex = GL_LIGHT0 + sLightCount++;
}

opengl::Light::~Light()
{
    glDisable(mLightIndex);
}

		enum EType
		{
			ePoint, 
			eDirectional, 
			eSpot,
			eArea,
			eVolume
		};
void opengl::Light::UpdateLight(const double * /*lLightGlobalPosition*/,float lConeAngle,const float lLightColor[4]) const
{
	//
   // 
   // glMultMatrixd((const double*)lLightGlobalPosition);
    glColor3fv(lLightColor);
    
    
    // Visible for double side.
    glDisable(GL_CULL_FACE);
  //  glEnable(GL_LIGHTING);
    // Draw wire-frame geometry.
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
const float ANGLE_TO_RADIAN	= 3.1415926f / 180.f;
#if 0
    if (mType == eSpot)
    {
        // Draw a cone for spot light.
        
        glScalef(1.0f, 1.0f, -1.0f);
        const double lRadians = ANGLE_TO_RADIAN * lConeAngle;
        const double lHeight = 15.0;
        const double lBase = lHeight * tan(lRadians / 2);
        GLUquadricObj * lQuadObj = gluNewQuadric();
        gluCylinder(lQuadObj, 0.0, lBase, lHeight, 18, 1);
        gluDeleteQuadric(lQuadObj);
        
    }
    else
    {
        // Draw a sphere for other types.
        GLUquadricObj * lQuadObj = gluNewQuadric();
#if 0
		static float light_radius=10.f;
        gluSphere(lQuadObj, light_radius,10, 10);
#endif
        gluDeleteQuadric(lQuadObj);
    }
#endif
    
    
#if 1
    // The transform have been set, so set in local coordinate.
    if (mType == eDirectional)
    {
        glLightfv(mLightIndex, GL_POSITION, DEFAULT_DIRECTION_LIGHT_POSITION);
    }
    else
    {
        glLightfv(mLightIndex, GL_POSITION, DEFAULT_LIGHT_POSITION);
    }

    glLightfv(mLightIndex, GL_DIFFUSE, lLightColor);
    glLightfv(mLightIndex, GL_SPECULAR, lLightColor);
    
    if (mType == eSpot && lConeAngle != 0.0)
    {
        glLightfv(mLightIndex, GL_SPOT_DIRECTION, DEFAULT_SPOT_LIGHT_DIRECTION);

        // If the cone angle is 0, equal to a point light.
        if (lConeAngle != 0.0f)
        {
            // OpenGL use cut off angle, which is half of the cone angle.
            glLightf(mLightIndex, GL_SPOT_CUTOFF, lConeAngle/2);
        }
    }
    glEnable(mLightIndex);
#endif
	//
  //  
}