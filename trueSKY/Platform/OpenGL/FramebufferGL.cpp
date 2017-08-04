#include <GL/glew.h>
#include "FramebufferGL.h"
#include <iostream>
#include <string>
#include "SimulGLUtilities.h"
#include "Simul/Base/RuntimeError.h"
#include "Simul/Platform/CrossPlatform/DeviceContext.h"
#include "Simul/Platform/CrossPlatform/Macros.h"
#include "Simul/Platform/OpenGL/RenderPlatform.h"
#ifdef _MSC_VER
#include <windows.h>
#else
#define BREAK_IF_DEBUGGING 
#endif

using namespace simul;
using namespace opengl;
std::stack<GLuint> FramebufferGL::fb_stack;

FramebufferGL::FramebufferGL(const char *n,GLenum target,int samples,int coverageSamples)
	:crossplatform::BaseFramebuffer(n)
	,m_target(target)
	,m_samples(samples)
	,m_coverageSamples(coverageSamples)
	,m_fb(0)
	,m_noDepthFb(0)
	,initialized(false)
	,wrap_clamp(GL_CLAMP_TO_EDGE)
{
	if(fb_stack.size()==0)
		fb_stack.push((GLuint)0);
}

FramebufferGL::~FramebufferGL()
{
	InvalidateDeviceObjects();
}

void FramebufferGL::RestoreDeviceObjects(crossplatform::RenderPlatform *r)
{
	renderPlatform=r;
}

void FramebufferGL::InvalidateDeviceObjects()
{
	GL_ERROR_CHECK
	SAFE_DELETE(buffer_texture);
	SAFE_DELETE(buffer_depth_texture);
	GL_ERROR_CHECK
	//SAFE_DELETE_TEXTURE(m_tex_col[0]);
	//SAFE_DELETE_TEXTURE(m_tex_depth);
	//SAFE_DELETE_RENDERBUFFER(m_rb_depth);
	SAFE_DELETE_FRAMEBUFFER(m_fb);
	SAFE_DELETE_FRAMEBUFFER(m_noDepthFb);
	GL_ERROR_CHECK
}

void FramebufferGL::SetWidthAndHeight(int w,int h)
{
	if(w!=Width||h!=Height)
	{
		Width=w;
		Height=h;
		InvalidateDeviceObjects();
		if(initialized)
		{
			InitColor_Tex(0,target_format);
			if(depth_format)
				SetDepthFormat(depth_format);
		}
	}
}

void FramebufferGL::SetAsCubemap(int face_size)
{
	if(face_size!=Width||face_size!=Height)
	{
		Width=face_size;
		Height=face_size;
		InvalidateDeviceObjects();
		if(initialized)
		{
			InitColor_Tex(0,target_format);
			if(depth_format)
				SetDepthFormat(depth_format);
		}
	}
}

void FramebufferGL::SetCubeFace(int f)
{
	current_face=f;
}

void FramebufferGL::SetFormat(crossplatform::PixelFormat p)
{
//	GLenum f=opengl::RenderPlatform::ToGLFormat(p);
	if(p!=target_format)
		InvalidateDeviceObjects();
	target_format=p;
}

void FramebufferGL::SetDepthFormat(crossplatform::PixelFormat p)
{
	//GLenum f=opengl::RenderPlatform::ToGLFormat(p);
	if(p!=depth_format)
		InvalidateDeviceObjects();
	depth_format=p;
}

void FramebufferGL::SetWrapClampMode(GLint wr)
{
	wrap_clamp=wr;
}

bool FramebufferGL::InitColor_Tex(int , crossplatform::PixelFormat p)
{
	SetFormat(p);
	return CreateBuffers();
}

bool FramebufferGL::CreateBuffers()
{
	if(!Width||!Height)
		return false;
GL_ERROR_CHECK
	initialized=true;
	if(!m_fb)
	{
		glGenFramebuffers(1, &m_fb);
	}
GL_ERROR_CHECK
	SAFE_DELETE(buffer_texture);
	SAFE_DELETE(buffer_depth_texture);
	buffer_texture=renderPlatform->CreateTexture();
	buffer_texture->ensureTexture2DSizeAndFormat(renderPlatform,Width, Height,target_format,false,false,false,1,0,false);
	buffer_depth_texture=renderPlatform->CreateTexture();
	//buffer_texture.ensureTexture2DSizeAndFormat(renderPlatform,Width,Height,GL_RGBA,false,true,1,0);
	glBindFramebuffer(GL_FRAMEBUFFER, m_fb);
GL_ERROR_CHECK
	if(target_format!=crossplatform::UNKNOWN)
	{
		glBindTexture(GL_TEXTURE_2D, buffer_texture->AsGLuint());
		glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, buffer_texture->AsGLuint(), 0);
		GLenum status= (GLenum) glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if(status!=GL_FRAMEBUFFER_COMPLETE)
			return false;
	}
GL_ERROR_CHECK
	if(depth_format!=crossplatform::UNKNOWN)
	{
		buffer_depth_texture->ensureTexture2DSizeAndFormat(renderPlatform,Width,Height,depth_format,false,false,false);
GL_ERROR_CHECK
		glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,buffer_depth_texture->AsGLuint(),0);
GL_ERROR_CHECK
		GLenum status= (GLenum) glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if(status!=GL_FRAMEBUFFER_COMPLETE)
			return false;
		buffer_depth_texture->width=Width;
		buffer_depth_texture->length=Height;
	}
GL_ERROR_CHECK
	glBindFramebuffer(GL_FRAMEBUFFER,fb_stack.size()?fb_stack.top():0);
GL_ERROR_CHECK
	return true;
}

void FramebufferGL::NoDepth()
{
}

bool FramebufferGL::IsValid() const
{
GL_ERROR_CHECK
	return (m_fb!=0);
}

	static bool fliptarget=false;
void FramebufferGL::Activate(crossplatform::DeviceContext &)
{
	if(!m_fb)
		CreateBuffers();
	CheckFramebufferStatus();
    glBindFramebuffer(GL_FRAMEBUFFER, m_fb); 
	GL_ERROR_CHECK
	CheckFramebufferStatus();
	glGetIntegerv(GL_VIEWPORT,main_viewport);
	GL_ERROR_CHECK
	glViewport(0,0,Width,Height);
	GL_ERROR_CHECK
	fb_stack.push(m_fb);
	SIMUL_ASSERT(activate_count==0);
	activate_count++;
	if(m_fb!=0&&depth_format!=crossplatform::UNKNOWN)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,buffer_depth_texture->AsGLuint(),0);
GL_ERROR_CHECK
		CheckFramebufferStatus();
GL_ERROR_CHECK
	}
}

void FramebufferGL::ActivateDepth(crossplatform::DeviceContext &)
{
	if(m_fb!=0&&depth_format!=crossplatform::UNKNOWN)
	{
	   glBindFramebuffer(GL_FRAMEBUFFER, m_fb); 
GL_ERROR_CHECK
		glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,buffer_texture?buffer_texture->AsGLuint():0, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,buffer_depth_texture->AsGLuint(),0);
GL_ERROR_CHECK
		CheckFramebufferStatus();
GL_ERROR_CHECK
		//glBindFramebuffer(GL_FRAMEBUFFER, 0); 
GL_ERROR_CHECK
	}
}

void FramebufferGL::ActivateColour(crossplatform::DeviceContext &,const float /*viewportXYWH*/[4])
{
	SIMUL_THROW("ActivateColour does not yet work for FramebufferGL");
	if(!m_fb)
		CreateBuffers();
	CheckFramebufferStatus();
    glBindFramebuffer(GL_FRAMEBUFFER, m_fb); 
	GL_ERROR_CHECK
	CheckFramebufferStatus();
	glGetIntegerv(GL_VIEWPORT,main_viewport);
	GL_ERROR_CHECK
	glViewport(0,0,Width,Height);
	GL_ERROR_CHECK
	fb_stack.push(m_fb);
	SIMUL_ASSERT(activate_count==0);
	activate_count++;
}
// Activate the FBO as a render target
// The FBO needs to be deactivated when using the associated Textures.

void FramebufferGL::ActivateViewport(crossplatform::DeviceContext &,float viewportX, float viewportY, float viewportW, float viewportH)
{
	if(!m_fb)
		CreateBuffers();
	CheckFramebufferStatus();
    glBindFramebuffer(GL_FRAMEBUFFER, m_fb); 
	GL_ERROR_CHECK
	CheckFramebufferStatus();
	glGetIntegerv(GL_VIEWPORT,main_viewport);
	GL_ERROR_CHECK
	glViewport((int)((float)Width*viewportX),(int)((float)Height*viewportY)
			,(int)((float)Width*viewportW),(int)((float)Height*viewportH));
	GL_ERROR_CHECK
	fb_stack.push(m_fb);
	SIMUL_ASSERT(activate_count==0);
	activate_count++;
}

void FramebufferGL::Deactivate(crossplatform::DeviceContext &)
{
//	if(fliptarget)
	{
		//fliptarget=false;
		//deviceContext.viewStruct.proj.m22*=-1.f;
	}
	GL_ERROR_CHECK
	CheckFramebufferStatus();
	GL_ERROR_CHECK
	// remove m_fb from the stack and...
	fb_stack.pop();
	activate_count--;
	SIMUL_ASSERT(activate_count==0);
	// ..restore the n one down.
	GLuint last_fb=fb_stack.top();
	GL_ERROR_CHECK
    glBindFramebuffer(GL_FRAMEBUFFER,last_fb);
	GL_ERROR_CHECK
	CheckFramebufferStatus();
	GL_ERROR_CHECK
	glViewport(0,0,main_viewport[2],main_viewport[3]);
	GL_ERROR_CHECK
}
void FramebufferGL::CopyDepthFromFramebuffer() 
{
	glBindFramebuffer (GL_READ_FRAMEBUFFER, m_fb);
	GL_ERROR_CHECK
	glBindFramebuffer (GL_DRAW_FRAMEBUFFER, fb_stack.top());
	GL_ERROR_CHECK
	glBlitFramebuffer (0, 0, Width, Height, 0, 0, Width, Height,  GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	GL_ERROR_CHECK
	glBindFramebuffer (GL_FRAMEBUFFER, fb_stack.top());
	GL_ERROR_CHECK
}

void FramebufferGL::DeactivateAndRender(crossplatform::DeviceContext &d,bool blend)
{
	GL_ERROR_CHECK
	Deactivate(d);
	Render(d.platform_context,blend);
}

void FramebufferGL::DeactivateDepth(crossplatform::DeviceContext &)
{
	if(m_fb!=0&&depth_format!=crossplatform::UNKNOWN)
	{
	   glBindFramebuffer(GL_FRAMEBUFFER, m_fb); 
GL_ERROR_CHECK
		glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, buffer_texture?buffer_texture->AsGLuint():0, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,0,0);
GL_ERROR_CHECK
		CheckFramebufferStatus();
GL_ERROR_CHECK
	}
	glDisable(GL_DEPTH_TEST);
	glDepthMask(0);
}

void FramebufferGL::Render(void *,bool blend)
{
GL_ERROR_CHECK
	
  
    glActiveTexture(GL_TEXTURE0);
    Bind();
    glDisable(GL_ALPHA_TEST);
	if(!blend)
	{
		glDisable(GL_DEPTH_TEST);
	}
	else
	{
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
	}
	glDepthMask(GL_FALSE);
GL_ERROR_CHECK
	::DrawQuad(0,0,1,1);
	
GL_ERROR_CHECK
	Release();
}


void FramebufferGL::Clear(crossplatform::DeviceContext &deviceContext,float r,float g,float b,float a,float depth,int mask)
{
	if(!mask)
		mask=GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT;
	ClearColour(deviceContext,r,g,b,a);
	if(mask&GL_COLOR_BUFFER_BIT)
		 glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
	// AMAZINGLY, OpenGL requires depth mask to be set to clear the depth buffer.
	if(mask&GL_DEPTH_BUFFER_BIT)
		glDepthMask(GL_TRUE);
	glClearDepth(depth);
	glClear(mask);			
//glClearBufferfv(GLenum buffer,buffnum,const GLFloat *);
//glClearBufferfi(GLenum buffer,buffnum,depth,stencil);
}

void FramebufferGL::ClearColour(crossplatform::DeviceContext &,float r,float g,float b,float a)
{
	glClearColor(r,g,b,a);
}

void FramebufferGL::CheckFramebufferStatus()
{
    GLenum status;
    status = (GLenum) glCheckFramebufferStatus(GL_FRAMEBUFFER);
    switch(status)
    {
        case GL_FRAMEBUFFER_COMPLETE:
            break;
        case GL_FRAMEBUFFER_UNSUPPORTED:
			SIMUL_BREAK("Unsupported framebuffer format\n");
			BREAK_IF_DEBUGGING;
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            SIMUL_BREAK("Framebuffer incomplete attachment\n");
			BREAK_IF_DEBUGGING;
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            SIMUL_BREAK("Framebuffer incomplete, missing attachment\n");
			BREAK_IF_DEBUGGING;
            break;
    /*    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
            std::cerr<<("Framebuffer incomplete, attached images must have same dimensions\n");
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_FORMATS:
            std::cerr<<("Framebuffer incomplete, attached images must have same format\n");
            break;*/
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
            SIMUL_BREAK("Framebuffer incomplete, missing draw buffer\n");
			BREAK_IF_DEBUGGING;
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
            SIMUL_BREAK("Framebuffer incomplete, missing read buffer\n");
			BREAK_IF_DEBUGGING;
            break;
        default:
			SIMUL_BREAK("Unknown Framebuffer error");
    }
}

void FramebufferGL::CalcSphericalHarmonics(crossplatform::DeviceContext &)
{
}