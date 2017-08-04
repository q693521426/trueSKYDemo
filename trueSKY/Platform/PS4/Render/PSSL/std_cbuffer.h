/* SCE CONFIDENTIAL
ORBIS Programmer Tool Runtime Library Release 00.810.030
* Copyright (C) 2012 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef __STDCBUFFER_H__
#define __STDCBUFFER_H__

#include "shader_common/shader_base.h"


unistruct cbStandardShaderConstants
{
	Matrix4Unaligned m_mv;
	Matrix4Unaligned m_mvp;
	Vector3Unaligned m_lp;
	float m_fPad0;
};

#endif
