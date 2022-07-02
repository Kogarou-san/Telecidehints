/*
	KernelDeint() plugin for Avisynth.

	Copyright (C) 2003 Donald A. Graft

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "internal.h"
#include "utilities.h"

class KillHints : public GenericVideoFilter
{

public:
    KillHints(PClip _child, IScriptEnvironment* env) :
				GenericVideoFilter(_child)
	{
		if (!vi.IsYUY2() && !vi.IsYV12()) env->ThrowError("KillHints: YUY2 or YV12 data required");
	}


    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

PVideoFrame __stdcall KillHints::GetFrame(int n, IScriptEnvironment* env)
{
	PVideoFrame src = child->GetFrame(n, env);

	unsigned int hint;
	const unsigned char *srcp;
	srcp = src->GetReadPtr(PLANAR_Y);

	if (!GetHintingData((unsigned char *) srcp, &hint))
	{
	env->MakeWritable(&src);
	srcp = src->GetReadPtr(PLANAR_Y);
    PutHintingData((unsigned char *) srcp, PROGRESSIVE, 0x11111111);
	}

	return src;
}

bool clipcompat(VideoInfo v1, VideoInfo v2) {
	return (v1.width==v2.width) && (v1.height == v2.height) && ((v1.IsYV12() && v2.IsYV12()) || (v1.IsYUY2() && v2.IsYUY2()));
}

class KernelDeint : public GenericVideoFilter
{
	PClip alt,altclean,altunknown;

public:
    KernelDeint(PClip _child, PClip _alt, PClip _altclean, PClip _altunknown, IScriptEnvironment* env) :
	  GenericVideoFilter(_child), alt(_alt), altclean(_altclean), altunknown(_altunknown)
	{
		if (!vi.IsYUY2() && !vi.IsYV12()) env->ThrowError("TelecideHints: YUY2 or YV12 data required");

		if (!(clipcompat(child->GetVideoInfo(),alt->GetVideoInfo()) && clipcompat(child->GetVideoInfo(),altclean->GetVideoInfo()) && clipcompat(child->GetVideoInfo(),altunknown->GetVideoInfo())))
			env->ThrowError("TelecideHints: incompatible clips");
	
	}


    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};


PVideoFrame __stdcall KernelDeint::GetFrame(int n, IScriptEnvironment* env)
{
	PVideoFrame src = child->GetFrame(n, env);

    const unsigned char *srcp;

	unsigned int hint;

		srcp = src->GetReadPtr(PLANAR_Y);
		if (GetHintingData((unsigned char *) srcp, &hint)) 
			return altunknown->GetFrame(n, env);
		else if (hint & PROGRESSIVE)
			return altclean->GetFrame(n, env);
		else 
			return alt->GetFrame(n, env);

}

AVSValue __cdecl Create_KernelDeint(AVSValue args, void* user_data, IScriptEnvironment* env)
{
PClip C;
PClip D;

	if (args[2].Defined())
		C = args[2].AsClip();
	else
	    C = args[0].AsClip();

	if (args[3].Defined())
		D = args[3].AsClip();
	else
	    D = args[0].AsClip();

    return new KernelDeint(
		args[0].AsClip(),
		args[1].AsClip(),
		C,
		D,
		env);
}

AVSValue __cdecl Create_IsHinted(AVSValue args, void* user_data, IScriptEnvironment* env) {
	if (!args[0].AsClip()->GetVideoInfo().IsYUV())
		env->ThrowError("IsHinted: Only YUY2 and YV12 data supported.");

	AVSValue cn = env->GetVar("current_frame");
	if (!cn.IsInt())
		env->ThrowError("IsHinted: This form of the filter can only be used within ConditionalFilter");
	int n = cn.AsInt();

	PClip srcclip = args[0].AsClip();
	PVideoFrame src = srcclip->GetFrame(n,env);
    const unsigned char *srcp = src->GetReadPtr(PLANAR_Y);
	unsigned int hint;

    AVSValue isHinted;

	if (GetHintingData((unsigned char *) srcp, &hint)) 
		isHinted = 0; //no hint
	else if (hint & PROGRESSIVE)
		isHinted = 1; //progressive
	else
		isHinted = 2; //interlaced
	
	return isHinted;
}

AVSValue __cdecl Create_KillHints(AVSValue args, void* user_data, IScriptEnvironment* env) {

	return new KillHints(
		args[0].AsClip(),
		env);

}

extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* env)
{
    env->AddFunction("TelecideHints", "cc[cleanclip]c[unknownclip]c", Create_KernelDeint, 0);
    env->AddFunction("IsHinted", "c", Create_IsHinted, 0);
    env->AddFunction("KillHints", "c", Create_KillHints, 0);
    return 0;
}

