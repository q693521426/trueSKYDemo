#include "GL/glew.h"
#include "Material.h"
#include "Texture.h"

using namespace simul;
using namespace opengl;


Material::Material()
{
}


Material::~Material()
{
}

void Material::Apply(crossplatform::DeviceContext &,crossplatform::PhysicalLightRenderData &/*physicalLightRenderData*/) 
{
	glActiveTexture(GL_TEXTURE0);
//	float zero[]	={0,0,0,0};
    glMaterialfv(GL_FRONT_AND_BACK	,GL_EMISSION	,mEmissive.mColor);
    glMaterialfv(GL_FRONT_AND_BACK	,GL_AMBIENT		,mAmbient.mColor);
    glMaterialfv(GL_FRONT_AND_BACK	,GL_DIFFUSE		,mDiffuse.mColor);
    glMaterialfv(GL_FRONT_AND_BACK	,GL_SPECULAR	,mSpecular.mColor);
    glMaterialf	(GL_FRONT_AND_BACK	,GL_SHININESS	,mShininess);
	if(mDiffuse.mTextureName)
	{
		glBindTexture(GL_TEXTURE_2D	,mDiffuse.mTextureName->AsGLuint());
		for(int i=1;i<2;i++)
		{
			glActiveTexture(GL_TEXTURE0+i);
			glBindTexture(GL_TEXTURE_2D	,mDiffuse.mTextureName->AsGLuint());
		}
	}
	else
		glBindTexture(GL_TEXTURE_2D	,0);

	glActiveTexture(GL_TEXTURE0);
}