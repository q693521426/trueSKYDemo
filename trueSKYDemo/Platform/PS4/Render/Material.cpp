#include "Material.h"
#include "Texture.h"
#include "Utilities.h"
#include "Simul/Platform/CrossPlatform/Effect.h"

using namespace simul;
using namespace orbis;

Material::Material()
	:effect(NULL)
{
}


Material::~Material()
{
}

void Material::Apply(crossplatform::DeviceContext &deviceContext,crossplatform::PhysicalLightRenderData &)
{
/*	glActiveTexture(GL_TEXTURE0);
	float zero[]	={0,0,0,0};
    glMaterialfv(GL_FRONT_AND_BACK	,GL_EMISSION	,mEmissive.mColor);
    glMaterialfv(GL_FRONT_AND_BACK	,GL_AMBIENT		,mAmbient.mColor);
    glMaterialfv(GL_FRONT_AND_BACK	,GL_DIFFUSE		,mDiffuse.mColor);
    glMaterialfv(GL_FRONT_AND_BACK	,GL_SPECULAR	,mSpecular.mColor);
    glMaterialf	(GL_FRONT_AND_BACK	,GL_SHININESS	,mShininess);
	if(mDiffuse.mTextureName)
		glBindTexture(GL_TEXTURE_2D	,((orbis::Texture *)mDiffuse.mTextureName)->shaderResourceView);
	else
		glBindTexture(GL_TEXTURE_2D	,0);
	for(int i=1;i<2;i++)
	{
		glActiveTexture(GL_TEXTURE0+i);
		glBindTexture(GL_TEXTURE_2D	,((orbis::Texture *)mDiffuse.mTextureName)->shaderResourceView);
	}
	glActiveTexture(GL_TEXTURE0);*/
	if(mDiffuse.mTextureName)
		effect->SetTexture(deviceContext,"diffuseTexture",mDiffuse.mTextureName);
}