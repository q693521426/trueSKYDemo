// Attention: in C:/Program Files (x86)/SCE/ORBIS SDKs/3.000/target/include_common/gnmx/config.h, ensure that SCE_GNMX_ENABLE_GFX_LCUE is defined as 1
// and not 0.
#include "framework/sample_framework.h"
#include "framework/simple_mesh.h"
#include "framework/gnf_loader.h"
#include "framework/frame.h"
#include "std_cbuffer.h"
#include <vector>
#include "Simul/Clouds/TrueSkyRenderer.h"
#include "Simul/Platform/PS4/Render/RenderPlatform.h"
#include "Simul/Platform/PS4/Render/Framebuffer.h"
#include "Simul/Platform/CrossPlatform/Camera.h"
#include "MemoryAllocator.h"

using namespace sce;
using namespace sce::Gnmx;
using namespace std;
using namespace simul;

#include <stdio.h>
#include <stdlib.h>

std::string filename("/app0/Sample/ps4_test.sq");

namespace
{
	Framework::GnmSampleFramework framework;
}

int main(int argc, const char *argv[])
{
	framework.processCommandLine(argc, argv);

	framework.m_config.m_lightingIsMeaningful = true;

	framework.initialize( "Basic", 
		"trueSKY Sample.",
		"This sample program displays a sky.");

	class Frame
	{
	public:
		sce::Gnmx::GnmxGfxContext m_commandBuffer;
		Constants *m_constants;
	};
	Frame frames[3];
	SCE_GNM_ASSERT(framework.m_config.m_buffers <= 3);
	for(unsigned buffer = 0; buffer < framework.m_config.m_buffers; ++buffer)
	{
		Frame *frame = &frames[buffer];
		createCommandBuffer(&frame->m_commandBuffer,&framework,buffer);
		framework.m_allocators.allocate((void**)&frame->m_constants,SCE_KERNEL_WB_ONION,sizeof(*frame->m_constants),4,Gnm::kResourceTypeConstantBufferBaseAddress,"Buffer %d Command Buffer",buffer);
	}

	AllocatorMemoryInterface mem(&framework.m_garlicAllocator,&framework.m_onionAllocator);
	void *fetchShader;
	Gnmx::VsShader *vertexShader = Framework::LoadVsMakeFetchShader(&fetchShader, "/app0/Sample/shader_vv.sb", &framework.m_allocators);
	Gnmx::InputOffsetsCache vertexShader_offsetsTable;
	Gnmx::generateInputOffsetsCache(&vertexShader_offsetsTable, Gnm::kShaderStageVs, vertexShader);

	Gnmx::PsShader *pixelShader = Framework::LoadPsShader("/app0/Sample/shader_p.sb", &framework.m_allocators);
	Gnmx::InputOffsetsCache pixelShader_offsetsTable;
	Gnmx::generateInputOffsetsCache(&pixelShader_offsetsTable, Gnm::kShaderStagePs, pixelShader);

	Gnm::Texture textures[2];
    Framework::GnfError loadError = Framework::kGnfErrorNone;
	loadError = Framework::loadTextureFromGnf(&textures[0], "/app0/Sample/icelogo-color.gnf", 0, &framework.m_allocators);
    SCE_GNM_ASSERT(loadError == Framework::kGnfErrorNone );
	loadError = Framework::loadTextureFromGnf(&textures[1], "/app0/Sample/icelogo-normal.gnf", 0, &framework.m_allocators);
    SCE_GNM_ASSERT(loadError == Framework::kGnfErrorNone );

	simul::orbis::RenderPlatform renderPlatformPs4(&mem,NULL);

	renderPlatformPs4.SetShaderBinaryPath("/app0/Render/shaderbin");
	renderPlatformPs4.PushTexturePath("/app0/Render/Textures");
	simul::base::MemoryInterface *memoryInterface=&mem;
	
	clouds::Environment *env=new clouds::Environment();
	simul::crossplatform::TextFileInput inp;
	textures[0].setResourceMemoryType(Gnm::kResourceMemoryTypeRO); // this texture is never bound as an RWTexture, so it's OK to mark it as read-only.
	textures[1].setResourceMemoryType(Gnm::kResourceMemoryTypeRO); // this texture is never bound as an RWTexture, so it's OK to mark it as read-only.

	clouds::TrueSkyRenderer *trueSkyRenderer=new clouds::TrueSkyRenderer(env,NULL,&mem);
	
	trueSkyRenderer->GetSimulWeatherRenderer()->SetShowCompositing(false);
	trueSkyRenderer->GetSimulWeatherRenderer()->SetShowCloudCrossSections(false);
	trueSkyRenderer->SetMakeCubemap(false);

	simul::crossplatform::Camera *cam=new crossplatform::Camera();
	int view_id=trueSkyRenderer->AddView(false);
	base::DefaultProfiler cpuProfiler;
	cpuProfiler.SetMaxLevel(3);
	base::SetProfilingInterface(GET_THREAD_ID(),&cpuProfiler);
	Gnm::Sampler trilinearSampler;
	trilinearSampler.init();
	trilinearSampler.setMipFilterMode(Gnm::kMipFilterModeLinear);
	trilinearSampler.setXyFilterMode(Gnm::kFilterModeBilinear, Gnm::kFilterModeBilinear);

	Framework::SimpleMesh torusMesh;
	BuildTorusMesh(&framework.m_allocators, "Torus", &torusMesh, 0.8f, 0.2f, 64, 32, 4, 1);

	sce::Vectormath::Scalar::Aos::Vector3 eyePosition(0,0.f,0.f);
	float azimuth=0.f;
	float elevation=0.f;
	float tilt=0.f;

	bool initialized=false;

	while (!framework.m_shutdownRequested)
	{
		Framework::GnmSampleFramework::Buffer *bufferCpuIsWriting = framework.m_buffer + framework.getIndexOfBufferCpuIsWriting();
		Frame *frame = frames + framework.getIndexOfBufferCpuIsWriting();
		Gnmx::GnmxGfxContext *gfxc = &frame->m_commandBuffer;

		Matrix4 viewMatrix;
		// Here we will update the camera using the sticks:
		{
			static float dt=0.01f;
			static float rot_speed=2.0f;
			static float lastTouchX=0,lastTouchY=0;
			float touchX	=lastTouchX,touchY=lastTouchY;
			if(framework.m_controllerContext.getFingers())
			{
				touchX		=framework.m_controllerContext.getFinger(0).x;
				touchY		=framework.m_controllerContext.getFinger(0).y;
				if(isinf(touchX)||isinf(touchY))
					touchX=touchY=0.f;
				if(lastTouchX==0.f&&lastTouchY==0.f)
				{
					lastTouchX=touchX;
					lastTouchY=touchY;
				}
			}
			else
			{
				touchX			=lastTouchX=0.f;
				touchY			=lastTouchY=0.f;
			}
			static float touchScale=100.0f;
			float T_right_left	=(float)(touchX-lastTouchX)*touchScale;
			float T_up_down		=(float)(touchY-lastTouchY)*touchScale;
			if(framework.m_controllerContext.getFingers())
			{
				lastTouchX		=touchX;
				lastTouchY		=touchY;
			}

			elevation			-=T_up_down*rot_speed*dt;
			azimuth				-=T_right_left*rot_speed*dt;

			float pi			=3.1415926536f;
			viewMatrix			=Matrix4::rotationZ(azimuth);
			viewMatrix			*=Matrix4::rotationX(pi/2.f);
			float RS_right_left	=framework.m_controllerContext.getRightStick().getX();
			float RS_up_down	=framework.m_controllerContext.getRightStick().getY();
			float LS_right_left	=framework.m_controllerContext.getLeftStick().getX();
			float LS_up_down	=-framework.m_controllerContext.getLeftStick().getY();

			static float speed	=20.f;
			eyePosition			+=viewMatrix.getCol0().getXYZ()*speed*RS_right_left;
			viewMatrix			*=Matrix4::rotationX(elevation);
			
			tilt				+=rot_speed*dt*LS_right_left;

			viewMatrix			*=Matrix4::rotationZ(tilt);

			eyePosition			+=viewMatrix.getCol2().getXYZ()*speed*RS_up_down;
			eyePosition.setZ(eyePosition.getZ()+speed*LS_up_down);
			viewMatrix.setTranslation(eyePosition);
			cam->SetOrientationAsMatrix((const float *)&viewMatrix);
		}
		gfxc->reset();
		framework.BeginFrame(*gfxc);
		// Render the scene:
		Gnm::PrimitiveSetup primSetupReg;
		primSetupReg.init();
		primSetupReg.setCullFace(Gnm::kPrimitiveSetupCullFaceBack);
		primSetupReg.setFrontFace(Gnm::kPrimitiveSetupFrontFaceCcw);
		gfxc->setPrimitiveSetup(primSetupReg);
		// Clear
		Gnmx::Toolkit::SurfaceUtil::clearRenderTarget(*gfxc, &bufferCpuIsWriting->m_renderTarget, framework.getClearColor());
		Gnmx::Toolkit::SurfaceUtil::clearDepthTarget(*gfxc, &bufferCpuIsWriting->m_depthTarget, 1.f);
		gfxc->setRenderTargetMask(0xF);
		gfxc->setActiveShaderStages(Gnm::kActiveShaderStagesVsPs);
		gfxc->setRenderTarget(0, &bufferCpuIsWriting->m_renderTarget);
		gfxc->setDepthRenderTarget(&bufferCpuIsWriting->m_depthTarget);

		Gnm::DepthStencilControl dsc;
		dsc.init();
		dsc.setDepthControl(Gnm::kDepthControlZWriteEnable, Gnm::kCompareFuncLess);
		dsc.setDepthEnable(true);
		gfxc->setDepthStencilControl(dsc);
		gfxc->setupScreenViewport(0, 0, bufferCpuIsWriting->m_renderTarget.getWidth(), bufferCpuIsWriting->m_renderTarget.getHeight(), 0.5f, 0.5f);

		gfxc->setVsShader(vertexShader, 0, fetchShader, &vertexShader_offsetsTable);
		gfxc->setPsShader(pixelShader, &pixelShader_offsetsTable);

		torusMesh.SetVertexBuffer(*gfxc, Gnm::kShaderStageVs);

		const float radians = framework.GetSecondsElapsedApparent() * 0.5f;
		const Matrix4 m = Matrix4::rotationZYX(Vector3(radians,radians,0.f));
		Constants *constants = frame->m_constants;
		constants->m_modelView = transpose(framework.m_worldToViewMatrix*m);
		constants->m_modelViewProjection = transpose(framework.m_viewProjectionMatrix*m);
		constants->m_lightPosition = framework.getLightPositionInViewSpace();
		constants->m_lightColor = framework.getLightColor();
		constants->m_ambientColor = framework.getAmbientColor();
		constants->m_lightAttenuation = Vector4(1, 0, 0, 0);

		Gnm::Buffer constantBuffer;
		constantBuffer.initAsConstantBuffer(constants, sizeof(Constants));
		constantBuffer.setResourceMemoryType(Gnm::kResourceMemoryTypeRO); // it's a constant buffer, so read-only is OK
		gfxc->setConstantBuffers(Gnm::kShaderStageVs, 0, 1, &constantBuffer);
		gfxc->setConstantBuffers(Gnm::kShaderStagePs, 0, 1, &constantBuffer);

		gfxc->setTextures(Gnm::kShaderStagePs, 0, 2, textures);
		gfxc->setSamplers(Gnm::kShaderStagePs, 0, 1, &trilinearSampler);
		
		gfxc->setPrimitiveType(torusMesh.m_primitiveType);
		gfxc->setIndexSize(torusMesh.m_indexType);

		//gfxc->initializeToDefaultContextState();
#ifdef _DEBUG
		sce::Gnm::GraphicsShaderControl graphicsShaderControl;
		graphicsShaderControl.init();
		graphicsShaderControl.setPsCuMask(0x1FF);
		gfxc->setGraphicsShaderControl(graphicsShaderControl);
		if(gfxc->validate())
		{
	//		_SCE_BREAK();
		}
#endif
		gfxc->drawIndex(torusMesh.m_indexCount, torusMesh.m_indexBuffer);

		if(!initialized)
		{
			renderPlatformPs4.RestoreDeviceObjects(gfxc);
			if(trueSkyRenderer)
			{
				trueSkyRenderer->RestoreDeviceObjects(&renderPlatformPs4);
			}
			initialized=true;
		}
		renderPlatformPs4.SetImmediateContext(gfxc);
		
		static double real_time_seconds=0.0;
		real_time_seconds+=1.0f/60.0f;
		env->SetRealTime(real_time_seconds);
		env->Update();
		
		trueSkyRenderer->SetCamera(view_id,cam);
		crossplatform::DeviceContext deviceContext;
		deviceContext.viewStruct.view_id=view_id;
		deviceContext.platform_context=gfxc;
		deviceContext.renderPlatform=&renderPlatformPs4;

		//simul::orbis::Framebuffer::setDefaultRenderTargets(&framework.m_backBuffer->m_renderTarget,&framework.m_backBuffer->m_depthTarget
		//	,0, 0, framework.m_backBuffer->m_renderTarget.getWidth(), framework.m_backBuffer->m_renderTarget.getHeight());
		orbis::Framebuffer::setDefaultRenderTargets(&bufferCpuIsWriting->m_renderTarget,
										&bufferCpuIsWriting->m_depthTarget,
										0,
										0,
										1920,
										1080
										);

		if(trueSkyRenderer)
		{
			trueSkyRenderer->Render(deviceContext);
		}
		//renderPlatformPs4.GetImmediateContext().asGfxContext()->submit();
	//	gfxc_i.submit();
#if 0
		if(simul::base::GetProfilingInterface(GET_THREAD_ID()))
		{
			static int I=18;
			static int X1=5;
			static int X2=50;
			static int X3=70;
			static int D=100;
			int i=0;
			int n=I;
			int x1=X1;
			int x2=X2;
			int x3=X3;
			std::string str;
			float t,maxt;
			int level=0;
			framework.m_backBuffer->m_window.setCursor(x1,I);
			framework.m_backBuffer->m_window.printf("CPU Profiling");
			framework.m_backBuffer->m_window.setCursor(x2,I);
			framework.m_backBuffer->m_window.printf("time, ms");
			framework.m_backBuffer->m_window.setCursor(x3,I);
			framework.m_backBuffer->m_window.printf("max");
			const char *txt=base::GetProfilingInterface(GET_THREAD_ID())->GetDebugText();
			const char *line=txt;
			char l[120];
			while(*line)
			{
				const char *u=line;
				char *v=l;
				while(*u!=0&&*u!='\n')
				{
					*v++=*u++;
				}
				*v=0;
				if(!*u)
					break;
				if(i>100)
					break;
				line=++u;
				l[79]=0;				

				framework.m_backBuffer->m_window.setCursor(x1+level,(i++)+I+1);
				framework.m_backBuffer->m_window.printf("%s",l);
			}
			i=0;
			x1=X1+D;
			x2=X2+D;
			x3=X3+D;
			framework.m_backBuffer->m_window.setCursor(x1,I);
			framework.m_backBuffer->m_window.printf("GPU Profiling");
			framework.m_backBuffer->m_window.setCursor(x2,I);
			framework.m_backBuffer->m_window.printf("time, ms");
			framework.m_backBuffer->m_window.setCursor(x3,I);
			framework.m_backBuffer->m_window.printf("max");
		}
#endif
		framework.EndFrame(*gfxc);
	}
	
	Frame *frame = frames + framework.getIndexOfBufferCpuIsWriting();
	Gnmx::GnmxGfxContext *gfxc = &frame->m_commandBuffer;
	framework.terminate(*gfxc);
    return 0;
}
