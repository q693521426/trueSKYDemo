#include "Layout.h"
#include "Simul/Platform/CrossPlatform/DeviceContext.h"
#include "Simul/Platform/PS4/Render/RenderPlatform.h"
#include "Simul/Base/RuntimeError.h"
#include <gnmx/gfxcontext.h>
using namespace simul;
using namespace orbis;

Layout::Layout()
{
}

Layout::~Layout()
{
	InvalidateDeviceObjects();
}

void Layout::InvalidateDeviceObjects()
{
}

void Layout::Apply(crossplatform::DeviceContext &deviceContext)
{
	if(apply_count!=0)
		SIMUL_BREAK("Layout::Apply without a corresponding Unapply!");
	apply_count++;
	//deviceContext.asGfxContext()->IAGetInputLayout( &previousInputLayout );
//	deviceContext.asD3D11DeviceContext()->IAGetPrimitiveTopology(&previousTopology);
//	deviceContext.asD3D11DeviceContext()->IASetInputLayout(AsD3D11InputLayout());
//	deviceContext.asD3D11DeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
//	deviceContext.asGfxContext()->setPrimitiveType(RenderPlatform::ToGnmTopology(topology));
}

void Layout::Unapply(crossplatform::DeviceContext &deviceContext)
{
	if(apply_count<=0)
		SIMUL_BREAK("Layout::Unapply without a corresponding Apply!")
	else if(apply_count>1)
		SIMUL_BREAK("Layout::Apply has been called too many times!")
	apply_count--;
	//deviceContext.asD3D11DeviceContext()->IASetPrimitiveTopology(previousTopology);
	//deviceContext.asD3D11DeviceContext()->IASetInputLayout( previousInputLayout );
	//SAFE_RELEASE(previousInputLayout);
}