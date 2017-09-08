#include "gnmx/config.h"
static_assert(SCE_GNMX_ENABLE_GFX_LCUE==1,"Attention: in C:/Program Files (x86)/SCE/ORBIS SDKs/3.000/target/include_common/gnmx/config.h, ensure that SCE_GNMX_ENABLE_GFX_LCUE is defined as 1 and not 0.");
#include "framework/framework.h"
#include "framework/sample_framework.h"
#include "toolkit/toolkit.h"
#include "toolkit/shader_common/shader_base.h"
#include "framework/gnf_loader.h"
#include "Simul/Clouds/BaseWeatherRenderer.h"
#include "Simul/Clouds/TrueSkyRenderer.h"
#include "Simul/Platform/PS4/Render/Framebuffer.h"
#include "Simul/Platform/PS4/Render/GpuProfiler.h"
#include "Simul/Platform/PS4/Render/RenderPlatform.h"
#include "Simul/Platform/CrossPlatform/HdrRenderer.h"
#include "Simul/Platform/CrossPlatform/Camera.h"
#include "Simul/Base/RuntimeError.h"

#include "perf.h"
#include <gnmx.h>

#ifdef ENABLE_THREAD_TRACE
#include <sdk_version.h>
#include <libsysmodule.h>
#include <razor_gpu_thread_trace.h>
#endif

unistruct Constants
{
	Matrix4Unaligned m_modelView;
	Matrix4Unaligned m_modelViewProjection;
	Vector4Unaligned m_lightPosition;
	Vector4Unaligned m_lightColor; // 1.3*float3(0.5,0.58,0.8);
	Vector4Unaligned m_ambientColor; // 0.3*float3(0.18,0.2,0.3);
	Vector4Unaligned m_lightAttenuation; // float4(1,0,0,0);
};
using namespace sce;
using namespace sce::Gnmx;
using namespace std;

#include <stdio.h>
#include <stdlib.h>

std::string filename("/app0/ps4_test.sq");

bool bufferSky=true;
bool showCompositing=true;
bool showFades=false;
bool secondViewport=false;
bool showCloudCrossSections=true;
bool showCloudDepth=false;
bool show2DCloudCrossSections=false;
bool showProfiling=false;
bool reloadShaders=false;
bool reloadSequence=false;
bool makeCubemaps=false;
bool resetTimings=false;
bool doThreadTrace=false;
bool quitSample=false;
bool showClouds=true;
bool show2DClouds=true;
bool showGodrays=true;
bool lightTables=false;

using namespace simul;
using namespace orbis;


class MenuCommandSetFlag : public Framework::MenuCommand
{
	bool &flag;
public:
	MenuCommandSetFlag(bool &f)
		:flag(f)
	{
	}
	virtual void adjust(int) const
	{
		flag=true;
	}
	virtual void printValue(sce::DbgFont::Window *window)
	{
	}
};

class SubMenu : public Framework::MenuCommand
{
	Framework::Menu m_menu;
public:
	SubMenu(int count,const Framework::MenuItem menuItem[])
	{
		m_menu.appendMenuItems(count, &menuItem[0]);
	}
	virtual Framework::Menu* getMenu()
	{
		return &m_menu;
	}
	virtual void adjust(int) const {}
};

float final_exposure=1.f;
static float sky_brightness=1.f;
int slices=80;
int downscale=4;
Framework::MenuCommandBool menuCommandBoolBufferSky(&bufferSky);
Framework::MenuCommandBool menuCommandBoolSecondViewport(&secondViewport);
Framework::MenuCommandBool menuCommandBoolCubemaps(&makeCubemaps);
Framework::MenuCommandFloat menuCommandFloatExposure(&final_exposure,0.1,5.0,0.1);
Framework::MenuCommandFloat menuCommandFloatSkyBrightness(&sky_brightness,0.1,5.0,0.1);
Framework::MenuCommandInt menuCommandIntSlices(&slices,40,120);
Framework::MenuCommandInt menuCommandIntDownScale(&downscale,1,12);

MenuCommandSetFlag menuCommandRecompileShaders(reloadShaders);
MenuCommandSetFlag menuCommandReloadSequence(reloadSequence);
MenuCommandSetFlag menuCommandResetTimings(resetTimings);
MenuCommandSetFlag menuCommandDoThreadTrace(doThreadTrace);
MenuCommandSetFlag menuCommandQuit(quitSample);

Framework::MenuCommandBool menuCommandBoolShowClouds(&showClouds);
Framework::MenuCommandBool menuCommandBoolShow2DClouds(&show2DClouds);
Framework::MenuCommandBool menuCommandBoolShowGodrays(&showGodrays);
Framework::MenuCommandBool menuCommandBoolShowCloudDepth(&showCloudDepth);
const Framework::MenuItem showHideMenuItem[] =
{
	{{"Clouds"		, "Toggle the Clouds"	}, &menuCommandBoolShowClouds},
	{{"2D Clouds"	, "Toggle the 2D Clouds"}, &menuCommandBoolShow2DClouds},
	{{"Godrays"		, "Toggle Godrays"		}, &menuCommandBoolShowGodrays},
};
SubMenu	menuCommandShowHide(sizeof(showHideMenuItem)/sizeof(showHideMenuItem[0]),showHideMenuItem);

Framework::MenuCommandBool menuCommandBoolShowFades(&showFades);
Framework::MenuCommandBool menuCommandBoolShowCompositing(&showCompositing);
Framework::MenuCommandBool menuCommandBoolShowCloudCrossSections(&showCloudCrossSections);
Framework::MenuCommandBool menuCommandBoolShow2DCloudCrossSections(&show2DCloudCrossSections);
Framework::MenuCommandBool menuCommandBoolShowProfiling(&showProfiling);
const Framework::MenuItem osdMenuItem[] =
{
	{{"2nd Viewport", "Show a secondary viewport"}, &menuCommandBoolSecondViewport},
	{{"Show Compositing", "Show compositing"}, &menuCommandBoolShowCompositing},
	{{"Show Fades", "Show fade tables"}, &menuCommandBoolShowFades},
	{{"Cloud Cross-sections", "Show cloud cross-sections"}, &menuCommandBoolShowCloudCrossSections},
	{{"Cloud Depth", "Show cloud depth"}, &menuCommandBoolShowCloudDepth},
	{{"2D cloud textures", "Show 2D cloud textures"}, &menuCommandBoolShow2DCloudCrossSections},
	{{"Profiling", "Show Profiling data"}, &menuCommandBoolShowProfiling},
	{{"Reset timings", "Reset the maximum timings"}, &menuCommandResetTimings},
	{{"Do Thread Trace", "Do a Thread Trace"}, &menuCommandDoThreadTrace},
};
SubMenu	menuCommandOSD(sizeof(osdMenuItem)/sizeof(osdMenuItem[0]),osdMenuItem);


Framework::MenuCommandBool menuCommandBoolLightTables(&lightTables);
const Framework::MenuItem renderMenuItem[] =
{
	{{"GPU Light Tables", "GPU Light Tables"}, &menuCommandBoolLightTables},
	{{"Downscale", "Downscale"}, &menuCommandIntDownScale},
	{{"Slices", "Slices"}, &menuCommandIntSlices},
	{{"Buffer Sky", "Toggle the low-res sky buffer"}, &menuCommandBoolBufferSky},
	{{"Exposure", "Final exposure"}, &menuCommandFloatExposure},
	{{"Sky Brightness", "Sky Brightness"}, &menuCommandFloatSkyBrightness},
};
SubMenu	menuCommandRender(sizeof(renderMenuItem)/sizeof(renderMenuItem[0]),renderMenuItem);

const Framework::MenuItem menuItem[] =
{
	{{"Rendering", "Rendering options"}, &menuCommandRender},
	{{"Show/Hide", "Toggle visibility of elements"}, &menuCommandShowHide},
	{{"OSD's", "Toggle visibility of OSDs"}, &menuCommandOSD},
	{{"Reload Shaders", "Reload the shaders"}, &menuCommandRecompileShaders},
	{{"Reload Sequence", "Reload the sequence"}, &menuCommandReloadSequence},
	{{"Cubemap", "Make a cubemap"}, &menuCommandBoolCubemaps},
	{{"Quit", "Quit the sample"}, &menuCommandQuit},
};
enum { kMenuItems = sizeof(menuItem)/sizeof(menuItem[0]) };

#define GLOBAL_TRACE_CACHE_SIZE (32*1024*1024)
uint64_t m_traceBuffer[GLOBAL_TRACE_CACHE_SIZE / sizeof(uint64_t)];

class AllocatorMemoryInterface:public simul::base::MemoryInterface
{
	Gnmx::Toolkit::IAllocator *allocator;
	std::map<void*,size_t> bytes_allocated;
	std::map<void*,std::string> callers;
	std::map<void*,size_t> video_bytes_allocated;
	std::map<void*,std::string> video_callers;
public:
	AllocatorMemoryInterface(Gnmx::Toolkit::IAllocator *a)
		:allocator(a)
	{
	}
	~AllocatorMemoryInterface()
	{
		if(bytes_allocated.size())
		{
			std::cerr<<"Unfreed general allocations: "<<bytes_allocated.size()<<std::endl;
			for(std::map<void*,std::string>::iterator i=callers.begin();i!=callers.end();i++)
			{
				std::cerr<<"0x"<<std::hex<<i->first<<": "<<i->second.c_str()<<" "<<bytes_allocated[i->first]<<" bytes"<<std::endl;
			}
		}
		if(video_bytes_allocated.size())
		{
			std::cerr<<"Unfreed video allocations: "<<video_bytes_allocated.size()<<std::endl;
			for(std::map<void*,std::string>::iterator i=video_callers.begin();i!=video_callers.end();i++)
			{
				std::cerr<<"0x"<<std::hex<<i->first<<": "<<i->second.c_str()<<" "<<video_bytes_allocated[i->first]<<" bytes"<<std::endl;
			}
		}
	}
	//! Allocate \a nbytes bytes of memory, aligned to \a align and return a pointer to them.
	virtual void* AllocateTracked(size_t nbytes,size_t align,const char *fn)
	{
		void *ptr;
		if(align==0)
			align=1;
		ptr=allocator->allocate(nbytes,(sce::Gnm::AlignmentType)align);
		if(fn)
		{
			//memalign(align,nbytes);//
			if(ptr==(void*)0x21362f580)
			{
				if(bytes_allocated.find(ptr)!=bytes_allocated.end())
					SIMUL_ASSERT(false);
			}
			bytes_allocated[ptr]=nbytes;
			callers[ptr]		=std::string(fn);
		}
		return ptr;
	}
	//! De-allocate the memory at \param address (requires that this memory was allocated with Allocate()).
	virtual void Deallocate(void* ptr)
	{
		if(ptr)
		{
			if(callers.find(ptr)==callers.end())
			{
				return;
			}
			if(ptr==(void*)0x21362f580)
			{
				bytes_allocated.find(ptr);
			}
			bytes_allocated.erase(bytes_allocated.find(ptr));
			callers.erase(callers.find(ptr));
			//free(ptr);
			allocator->release(ptr);
		}
	}
	//! Allocate \a nbytes bytes of memory, aligned to \a align and return a pointer to them.
	virtual void* AllocateVideoMemoryTracked(size_t nbytes,size_t align,const char *fn)
	{
		void *ptr;
		if(align==0)
			align					=1;
		ptr							=allocator->allocate(nbytes,(sce::Gnm::AlignmentType)align);
		if(fn)
		{
			if(video_callers.find(ptr)!=video_callers.end())
			{
				std::cerr<<"Already got this address from "<<video_callers[ptr].c_str()<<std::endl;
			}
			if(ptr==(void*)0x215e20000)
			{
				video_bytes_allocated[ptr]=nbytes;
			}
			video_bytes_allocated[ptr]	=nbytes;
			video_callers[ptr]			=std::string(fn);
		}
		return ptr;
	}
	//! De-allocate the memory at \param address (requires that this memory was allocated with Allocate()).
	virtual void DeallocateVideoMemory(void* address)
	{
		if(address)
		{
			if(address==(void*)0x215e20000)
			{
				video_bytes_allocated.find(address);
			}
			if(video_callers.find(address)==video_callers.end())
			{
				//std::cerr<<"Not got this address"<<std::endl;
				return;
			}
			video_bytes_allocated.erase(video_bytes_allocated.find(address));
			video_callers.erase(video_callers.find(address));
			allocator->release(address);
		}
	}
};
class Profiler:public simul::base::ProfilingInterface
{
	struct Timing
	{
		Timing():start(0.0),time(0.0),max_time(0.0),level(0)
		{
		}
		double start,time,max_time;
		int level;
	};
	typedef std::map<std::string,Timing> Map;
	Map timings;
	uint64_t performanceCounter()
	{
		uint64_t result = 0;
		timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		result = (uint64_t)ts.tv_sec * 1000000000LL + (uint64_t)ts.tv_nsec;
		return result;
	}
	uint64_t performanceFrequency()
	{
		uint64_t result = 1;
		result = 1000000000LL;
		return result;
	}
	std::stack<std::string> last_str;
public:
	void StartFrame(){}
	void EndFrame(){}
	//! Mark the start of a profiling block.
	virtual void Begin(const char *txt)
	{
		std::string str(txt);
		timings[str].start=(double)(1000.0*(double)performanceCounter()/(double)performanceFrequency());
		//sceRazorCpuPushMarker(txt, SCE_RAZOR_COLOR_CYAN, SCE_RAZOR_MARKER_ENABLE_HUD);
		last_str.push(str);
	}
	//! Allocate \a nbytes bytes of memory, aligned to \a align and return a pointer to them.
	virtual void End()
	{
		static double retain=0.1;
		std::string str=last_str.top();//(txt);
		Map::iterator i=timings.find(str);
		if(i!=timings.end())
		{
			i->second.time	*=retain;
			double t		=(double)(1000.0*(double)performanceCounter()/(double)performanceFrequency());
			t				-=i->second.start;
			i->second.time	+=(1.f-retain)*(t);
			if(t>i->second.max_time)
				i->second.max_time=t;
		}
		last_str.pop();
		i->second.level=last_str.size();
		//sceRazorCpuPopMarker();
	}
	void ResetMaximums()
	{
		for(Map::iterator i=timings.begin();i!=timings.end();i++)
		{
			i->second.max_time=0.0;
		}
	}
	bool GetCounter(int i,std::string &str,double &t,double &maxt,int &level)
	{
		if(i>=timings.size())
			return false;
		static Map::iterator it;
		if(i==0)
			it=timings.begin();
		str		=it->first;
		t		=it->second.time;
		maxt	=it->second.max_time;
		level	=it->second.level;
		it++;
		return true;
	}
};

Matrix4 MakeProjectionMatrix(float hrad,float vrad,float zNear,float zFar)
{
	float xScale= 1.f/tan(hrad/2.f);
	float yScale = 1.f/tan(vrad/2.f);
	Matrix4 m(0.f);
	m[0][0]=xScale;
	m[1][1]=yScale;
	m[2][2]=	zFar/(zNear-zFar);
	m[2][3]=	-1.f;
	m[3][2]=	zNear*zFar/(zNear-zFar);
	return m;
}

Matrix4 MakeDepthReversedProjectionMatrix(float hrad,float aspect,float zNear,float zFar)
{
	float vrad		=tan(atan(hrad)/aspect);
	float xScale	=1.f/tan(hrad/2.f);
	float yScale	=1.f/tan(vrad/2.f);
	Matrix4 m(0.f);
	m[0][0]		=xScale;
	m[1][1]		=yScale;
	m[2][2]		=zNear/(zFar-zNear);
	m[2][3]		=-1.f;
	m[3][2]		=zNear*zFar/(zFar-zNear);
	return m;
}

Vector4Unaligned ToVector4Unaligned( const simul::sky::float4& r )
{
	const Vector4Unaligned result = { r.x, r.y, r.z,r.w};
	return result;
}


Matrix4Unaligned ToMatrix4Unaligned( const simul::math::Matrix4x4 &m )
{
	const Matrix4Unaligned result = *((Matrix4Unaligned*)&m);
	return result;
}

#define PS4_VALIDATE gfxc.validate();
int main(int argc, const char *argv[])
{
	if(errno!=0)
	{
		simul::base::RuntimeError(strerror(errno));
		errno=0;
	}
	sceRazorCpuInit(m_traceBuffer, sizeof(m_traceBuffer));
	simul::base::DefaultProfiler profiler;
	simul::base::SetProfilingInterface(GET_THREAD_ID(),&profiler);
	Framework::GnmSampleFramework framework;
	framework.processCommandLine(argc, argv);
	framework.m_config.m_targetHeight=1080;
	framework.m_config.m_targetWidth=1920;
	framework.m_config.m_garlicMemoryInBytes=1024*1024*1024;
	if(errno!=0)
	{
		simul::base::RuntimeError(strerror(errno));
		errno=0;
	}
	
	framework.initialize("trueSKY",(filename+".").c_str(),"This sample program shows sky rendering.",argc,argv);
	if(errno!=0)
	{
		simul::base::RuntimeError(strerror(errno));
		errno=0;
	}
	framework.m_depthFar=300000.f;
	framework.m_depthNear = 1.f;
	const float aspect = (float)framework.m_config.m_targetWidth / (float)framework.m_config.m_targetHeight;
	//framework.SetProjectionMatrix(Matrix4::frustum( -aspect, aspect, -1, 1, framework.m_depthNear, framework.m_depthFar ));

	framework.removeAllMenuItems();
	framework.appendMenuItems(kMenuItems, menuItem);

	// Set up gfx context
	Gnmx::LightweightGfxContext gfxc;
	AllocatorMemoryInterface mem(&framework.m_garlicAllocator);
	//RenderPlatform renderPlatformPs4(&mem,&gfxc);
	simul::base::MemoryInterface *memoryInterface=&mem;
#ifdef DH_MEMORY
	simul::base::memory::SetMemInterface(mem);
#endif
	void *globalResourceTablePtr= mem.AllocateVideoMemory(SCE_GNM_SHADER_GLOBAL_TABLE_SIZE, sce::Gnm::kAlignmentOfBufferInBytes);
	gfxc.setGlobalResourceTableAddr(globalResourceTablePtr);
	const uint32_t kNumRingEntries		=16;
//	const uint32_t cueCpRamShadowSize	=Gnmx::ConstantUpdateEngine::computeCpRamShadowSize();
	const uint32_t cueHeapSize			=Gnmx::ConstantUpdateEngine::computeHeapSize(kNumRingEntries);
	void *cueHeapAddr					=mem.AllocateVideoMemory(cueHeapSize, Gnm::kAlignmentOfBufferInBytes);
	gfxc.init(framework.m_onionAllocator.allocate(Gnm::kIndirectBufferMaximumSizeInBytes, Gnm::kAlignmentOfBufferInBytes)
		, Gnm::kIndirectBufferMaximumSizeInBytes
		,framework.m_onionAllocator.allocate(Gnm::kIndirectBufferMaximumSizeInBytes, Gnm::kAlignmentOfBufferInBytes)
		, Gnm::kIndirectBufferMaximumSizeInBytes			// Constant command buffer
		,globalResourceTablePtr
		);
	simul::clouds::Environment *env=new simul::clouds::Environment(NULL,&mem);

	
	std::string sq_filename2( filename );

	reloadSequence=true;
	clouds::TrueSkyRenderer *trueSkyRenderer=new clouds::TrueSkyRenderer(env,NULL,&mem);

	simul::base::SetProfilingInterface(GET_THREAD_ID(),&profiler);
	
	// 
	gfxc.reset();
	gfxc.initializeDefaultHardwareState();
//	renderPlatformPs4.RestoreDeviceObjects(&gfxc);

	simul::crossplatform::Camera *cam=new crossplatform::Camera();
	int view_id=0;
	if(trueSkyRenderer)
	{
	//	trueSkyRenderer->RestoreDeviceObjects(&renderPlatformPs4);
	}
	sce::Vectormath::Scalar::Aos::Vector3 eyePosition(0,0.f,0.f);
	float azimuth=0.f;
	float elevation=0.f;
	float tilt=0.f;

	simul::orbis::Texture depthTexture;
	float depth_init[32*32];
	memset(depth_init,0,32*32*sizeof(float));
	//depthTexture.ensureTexture2DSizeAndFormat(&renderPlatformPs4,32,32,crossplatform::D_32_FLOAT,false,false,true);
	
				//env->skyKeyframer->SetTime(.8);
	ERRNO_CHECK

#ifdef ENABLE_THREAD_TRACE
	sceSysmoduleLoadModule(SCE_SYSMODULE_RAZOR_GPU_THREAD_TRACE);
#endif


	// Main Loop:
	while (!framework.m_shutdownRequested)
	{
		if(resetTimings)
		{
			profiler.ResetMaximums();
			resetTimings = false;
		}
#ifdef ENABLE_THREAD_TRACE
		if (doThreadTrace)
		{
			gpuProfiler.SaveTrace();
			doThreadTrace = false;
		}
#endif
		if(quitSample)
		{
			framework.RequestTermination();
		}
		if(reloadSequence)
		{
			//TODO[ST]: We need this changing so that we can either implement a pure virtual interface or at the very least pass in a void* so that we can you our file system
			::simul::crossplatform::TextFileInput ifs;
	ERRNO_CHECK
			ifs.Load(sq_filename2.c_str());
	ERRNO_CHECK
			if(ifs.Good())
			{
				env->LoadFromText(ifs);
			}
	ERRNO_CHECK
			reloadSequence=false;
	
			if(trueSkyRenderer)
			{
				trueSkyRenderer->InvalidateDeviceObjects();
			//	trueSkyRenderer->RestoreDeviceObjects(&renderPlatformPs4);
			}
			view_id=trueSkyRenderer->AddView(false);
			trueSkyRenderer->SetCamera(view_id,cam);
		}
	ERRNO_CHECK
		gfxc.reset();
		gfxc.initializeDefaultHardwareState();
		framework.BeginFrame(gfxc);
		crossplatform::DeviceContext deviceContext;
		deviceContext.viewStruct.view_id=view_id;
		deviceContext.platform_context=&gfxc;
		//deviceContext.renderPlatform=&renderPlatformPs4;
	//simul::crossplatform::SetGpuProfilingInterface(deviceContext,renderPlatformPs4.GetGpuProfiler());
	//	renderPlatformPs4.GetGpuProfiler()->StartFrame(deviceContext);
		
		if(reloadShaders)
		{
			simul::orbis::Utilities::RecompileShaders();
			trueSkyRenderer->RecompileShaders();
				env->skyKeyframer->Reset();
				env->cloudKeyframer->Reset();
			reloadShaders=false;
		}
		// What overlays to enable:
		{
			trueSkyRenderer->GetSimulWeatherRenderer()->SetShowFades(showFades);
			trueSkyRenderer->GetSimulWeatherRenderer()->SetShowCloudCrossSections(showCloudCrossSections);
			trueSkyRenderer->GetSimulWeatherRenderer()->SetShowCompositing(showCompositing);
		}
	ERRNO_CHECK
		Vector4 clearColour(.35f,0.f,0.55f,0.f);
		// simul Framebuffer class needs to know the default rt's in order to restore them, and the Orbis API is missing
		// a GetCurrentRenderTarget implementation, so:
		simul::orbis::Framebuffer::setDefaultRenderTargets(&framework.m_backBuffer->m_renderTarget,&framework.m_backBuffer->m_depthTarget
			,0, 0, framework.m_backBuffer->m_renderTarget.getWidth(), framework.m_backBuffer->m_renderTarget.getHeight());
		gfxc.setRenderTarget(0, &framework.m_backBuffer->m_renderTarget);
		gfxc.setDepthRenderTarget(&framework.m_backBuffer->m_depthTarget);
		// Clear
		Gnmx::Toolkit::SurfaceUtil::clearRenderTarget(gfxc,&framework.m_backBuffer->m_renderTarget,clearColour);
		// Clear depth to ZERO for reversed depth buffer
		Gnmx::Toolkit::SurfaceUtil::clearDepthTarget(gfxc,&framework.m_backBuffer->m_depthTarget,0.f);
	PS4_VALIDATE
		gfxc.setupScreenViewport(0, 0, framework.m_backBuffer->m_renderTarget.getWidth(), framework.m_backBuffer->m_renderTarget.getHeight(), 1.0f, 0.0f);
	ERRNO_CHECK
	PS4_VALIDATE
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
			framework.SetViewToWorldMatrix(viewMatrix);
		//	viewMatrix			=orthoInverse(viewMatrix);
		}
		static float fov=90.f;
		Matrix4 proj=MakeDepthReversedProjectionMatrix(3.14159f*fov/180.f,aspect,framework.m_depthNear,framework.m_depthFar);
		framework.SetProjectionMatrix(proj);
		// Time controls
		{
			float dt_real=1.f/60.f;
			static float change_rate=40.0f/24.f/60.f/60.f;
			float right_left=(float)framework.m_controllerContext.isButtonDown(Framework::Input::BUTTON_R2)
							-(float)framework.m_controllerContext.isButtonDown(Framework::Input::BUTTON_L2);
			float fast		=(float)framework.m_controllerContext.isButtonDown(Framework::Input::BUTTON_R1)
							-(float)framework.m_controllerContext.isButtonDown(Framework::Input::BUTTON_L1);
			right_left		+=fast*100.f;
			float t=env->skyKeyframer->GetTime();
			if(fabs(right_left)>0.001f)
			{
				t+=right_left*dt_real*change_rate;
			}
			t+=dt_real/24.f/60.f/60.f;
			env->skyKeyframer->SetTime(t);
		}
	ERRNO_CHECK
#if 0
		// Render the scene:
		Gnm::PrimitiveSetup primSetupReg;
		primSetupReg.init();
		primSetupReg.setCullFace(Gnm::kPrimitiveSetupCullFaceNone);
		primSetupReg.setFrontFace(Gnm::kPrimitiveSetupFrontFaceCcw);
		gfxc.setPrimitiveSetup(primSetupReg);

		gfxc.setActiveShaderStages(Gnm::kActiveShaderStagesVsPs);
		Gnm::DepthStencilControl dsc;
		dsc.init();
		dsc.setDepthControl(Gnm::kDepthControlZWriteEnable, Gnm::kCompareFuncAlways);
		dsc.setDepthEnable(true);
		gfxc.setDepthStencilControl(dsc);
	
	ERRNO_CHECK
		env->Update(0.f);
		
		Matrix4Unaligned v=ToMatrix4Unaligned(viewMatrix);
		Matrix4Unaligned p=ToMatrix4Unaligned(framework.m_viewProjectionMatrix);
		cam->SetOrientationAsMatrix((const float *)&v);
		
		simul::sky::float4 viewportRegion(0,0,1.f,1.f);
		simul::sky::float4 viewportRegion2(0,0,1.f,1.f);
		if(trueSkyRenderer)
		{
		//	trueSkyRenderer->Render(deviceContext);
		//	renderPlatformPs4.DrawTexture(deviceContext	,500,500,50,25,NULL,vec4(1.0,1.0,0,1.0));
		}
		//simul::orbis::Utilities::DisableBlend(&gfxc);
		//simul::orbis::Utilities::EnableDepth(&gfxc,sce::Gnm::kCompareFuncGreaterEqual);
		//simul::orbis::Utilities::DisableDepth(&gfxc);
		//simul::orbis::Utilities::SetScreenSize(framework.m_config.m_targetWidth,framework.m_config.m_targetHeight);

		if(secondViewport)
		{
		//	gfxc.setTextures(sce::Gnm::kShaderStagePs,0,1,(sce::Gnm::Texture*)secondaryViewportFb.GetColorTex());
		//	simul::orbis::Utilities::DrawQuad(&gfxc
		//									,(int)framework.m_config.m_targetWidth/2,(int)framework.m_config.m_targetHeight/2
		//									,(int)framework.m_config.m_targetWidth/2,(int)framework.m_config.m_targetHeight/2);
		}

		//effect->Apply(deviceContext,effect->GetTechniqueByName("simple"),0);
		//renderPlatformPs4.DrawQuad(deviceContext);
		//effect->Unapply(deviceContext);

		//renderPlatformPs4.GetGpuProfiler()->EndFrame(deviceContext);
		{
			framework.m_backBuffer->m_window.setCursor(0,4);
			float t=env->skyKeyframer->GetTime();
			t*=24.f;
			int hour=t;
			t-=hour;
			t*=60.f;
			int min=t;
			t-=min;
			t*=60.f;
			int sec=t;
			framework.m_backBuffer->m_window.printf("Time is %02d:%02d:%02d",hour,min,sec);
			framework.m_backBuffer->m_window.setCursor(25,4);
		}
		if(showProfiling)
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
			const char *txt=profiler.GetDebugText();
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
		/*	while(gpuProfiler.GetCounter(i++,str,t))//,maxt))
			{
				framework.m_backBuffer->m_window.setCursor(x1,i+I+1);
				framework.m_backBuffer->m_window.printf("%s",str.c_str());
				framework.m_backBuffer->m_window.setCursor(x2,i+I+1);
				framework.m_backBuffer->m_window.printf("%4.3g",t);
				framework.m_backBuffer->m_window.setCursor(x3,i+I+1);
				framework.m_backBuffer->m_window.printf("%4.3g",maxt);
			}(*/
		}
#endif
		framework.EndFrame(gfxc);
	}
	del(trueSkyRenderer,&mem);
	del(env,&mem);
	//meshTechnique.release();
	mem.DeallocateVideoMemory(cueHeapAddr);
	mem.DeallocateVideoMemory(globalResourceTablePtr);
	framework.terminate(gfxc);
    return 0;
}
