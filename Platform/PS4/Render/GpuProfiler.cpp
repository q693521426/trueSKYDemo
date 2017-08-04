#include "GpuProfiler.h"
#include "Simul/Base/StringFunctions.h"
#include "Simul/Platform/CrossPlatform/RenderPlatform.h"
#include <stdint.h>

using namespace simul;
using namespace orbis;

using std::string;
using std::map;

#ifdef ENABLE_THREAD_TRACE
#include <sdk_version.h>
#include <kernel.h>
#include <razor_gpu_thread_trace.h>
#endif // ENABLE_THREAD_TRACE

#ifdef ENABLE_THREAD_TRACE
static enum{ kNotTracing, kTraceRequested, kTracing } s_threadTraceState = kNotTracing;

void GpuProfiler::SaveTrace()
{
	if( s_threadTraceState == kNotTracing )
	{
		s_threadTraceState = kTraceRequested;
	}
}
#endif


GpuProfiler::~GpuProfiler()
{
}

void GpuProfiler::Begin(crossplatform::DeviceContext &deviceContext,const char *name)
{
	sce::Gnmx::LightweightGfxContext* gfxc=deviceContext.asGfxContext();
	gfxc->pushMarker(name);
	crossplatform::GpuProfiler::Begin(deviceContext,name);
	
    if(!enabled)
        return;
#ifdef ENABLE_THREAD_TRACE // Thread trace currently has its own system of markers. ???
	if( s_threadTraceState == kTracing )
	{
		sceRazorGpuThreadTracePushMarker( &( gfxc->m_dcb ), n );
	}
#endif

}

void GpuProfiler::End(crossplatform::DeviceContext &deviceContext)
{
	sce::Gnmx::LightweightGfxContext* gfxc=deviceContext.asGfxContext();
	gfxc->popMarker();
	crossplatform::GpuProfiler::End(deviceContext);
    if(!enabled)
        return;
#ifdef ENABLE_THREAD_TRACE
	if( s_threadTraceState == kTracing )
	{
		sceRazorGpuThreadTracePopMarker( &( gfxc->m_dcb ) );
	}
#endif
}

volatile uint32_t* pGpuFinishedLabel = nullptr;

void GpuProfiler::StartFrame(crossplatform::DeviceContext &deviceContext)
{
	crossplatform::GpuProfiler::StartFrame(deviceContext);
	sce::Gnmx::LightweightGfxContext* gfxc=deviceContext.asGfxContext();
#ifdef ENABLE_THREAD_TRACE
			if( s_threadTraceState == kTraceRequested )
			{
                // Lets initialise the thread trace tool with some params.
                static SceRazorGpuThreadTraceParams params =
                {
                    sizeof( SceRazorGpuThreadTraceParams ), // sizeInBytes
                    {
                        sce::Gnm::kSqPerfCounterWaveCycles, // Measures the total number of WFs on the CU
                        sce::Gnm::kSqPerfCounterWaveReady, // See if we're stalled waiting for external dependencies by checking how many WFs are ready to execute instructions.
                        sce::Gnm::kSqPerfCounterInsts, // Total IPC to see if we're starving the execution units
                        sce::Gnm::kSqPerfCounterInstsValu, // This measures the utilization of the VALU, which will max out if we're compute bound.

                        sce::Gnm::kSqPerfCounterWaitCntAny, // Any external stall that we need to wait for using s_waitcnt.
                        sce::Gnm::kSqPerfCounterWaitCntVm, // Cycles spent stalling for textures and buffers.
                        sce::Gnm::kSqPerfCounterWaitCntExp, // Cycles spent stalling for exports.
                        sce::Gnm::kSqPerfCounterWaitExpAlloc, // Waiting for an export allocation to return.

                        sce::Gnm::kSqPerfCounterWaitAny, // This should flare up when and instruction other than s_waitcnt is stalled, e.g. due to TA/TD pressure (subtract kSqPerfCounterWaitCntAny)
                        sce::Gnm::kSqPerfCounterIfetch, // Number of instruction fetches.
                        sce::Gnm::kSqPerfCounterWaitIfetch, // Number of cycles WFs are stalled waiting for instruction fetch.
                        sce::Gnm::kSqPerfCounterSurfSyncs, // Syncronization primitives. I think those only show up in CU0 for each SE.

                        sce::Gnm::kSqPerfCounterEvents, // Counts event, such as the ones used to flush the CBDB metadata caches.
                        sce::Gnm::kSqPerfCounterInstsBranch, // See if some draws are very branchy, which often means bad lane utilization.
                        sce::Gnm::kSqPerfCounterValuDepStall, // Dependency stalls at instructions level. This is just me being curious.
                        sce::Gnm::kSqPerfCounterCbranchFork // Counts fork/join type branches, which should be uncommon but expensive.
                    }, // counters
                    16, // numCounters
                    SCE_RAZOR_GPU_THREAD_TRACE_COUNTER_RATE_MEDIUM, // counterRate
                    false, // enableInstructionIssueTracing
                    0, // shaderEngine0ComputeUnitIndex
                    9, // shaderEngine1ComputeUnitIndex
                };
                SceRazorGpuErrorCode result = sceRazorGpuThreadTraceInit( &params );

                //AssertMessage( result != (SceRazorGpuErrorCode)SCE_RAZOR_GPU_THREAD_TRACE_ERROR_PA_DEBUG_NOT_ENABLED, "pa debug needs to be enabled for threadTrace");
               // AssertMessage( result == SCE_OK, "sceRazorGpuThreadTraceInit returned %08x", result );
                sceRazorGpuThreadTraceStart( &( gfxc->m_dcb ) );
                s_threadTraceState = kTracing;
            }
            
#endif   // ENABLE_THREAD_TRACE

}


void GpuProfiler::EndFrame(crossplatform::DeviceContext &deviceContext)
{
	crossplatform::GpuProfiler::EndFrame(deviceContext);
#ifdef ENABLE_THREAD_TRACE
		
		if( s_threadTraceState == kTracing )
		{
			// Insert a label that we can wait on if we need to save the trace
			sceRazorGpuThreadTraceStop( &( gfxc->m_dcb ) );
			pGpuFinishedLabel = (uint32_t *) gfxc->allocateFromCommandBuffer(4,8);
			*pGpuFinishedLabel = 0;
			gfxc->writeAtEndOfPipe(sce::Gnm::kEopFlushAndInvalidateCbDbCaches,sce::Gnm::kEventWriteDestMemory, (void*) pGpuFinishedLabel,sce::Gnm::kEventWriteSource32BitsImmediate,1,sce::Gnm::kCacheActionWriteBackAndInvalidateL1andL2,sce::Gnm::kCachePolicyLru);
		}
#endif

}

void GpuProfiler::AfterEndFrame()
{
	#ifdef ENABLE_THREAD_TRACE
		if( s_threadTraceState == kTracing )
		{
			// Wait for the GPU to finish, then save the trace out to a file.
			while( *pGpuFinishedLabel == 0 )
			{
				sceKernelUsleep(1);
			}
			pGpuFinishedLabel = nullptr;
            sceRazorGpuThreadTraceSave( "/hostapp/trace.rtt" );
			sceRazorGpuThreadTraceReset();
			s_threadTraceState = kNotTracing;
			sceRazorGpuThreadTraceShutdown();
		}

#endif
}
