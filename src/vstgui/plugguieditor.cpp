//-----------------------------------------------------------------------------
// VST Plug-Ins SDK
// VSTGUI: Graphical User Interface Framework for VST plugins
//
// Version 3.6
//
//-----------------------------------------------------------------------------
// VSTGUI LICENSE
// (c) 2008, Steinberg Media Technologies, All Rights Reserved
//-----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
//   * Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation 
//     and/or other materials provided with the distribution.
//   * Neither the name of the Steinberg Media Technologies nor the names of its
//     contributors may be used to endorse or promote products derived from this 
//     software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A  PARTICULAR PURPOSE ARE DISCLAIMED. 
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE  OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#ifndef __plugguieditor__
#include "plugguieditor.h"
#endif

#include <chrono>
#include <thread>

#define kIdleRate    100 // host idle rate in ms
#define kIdleRate2    50
#define kIdleRateMin   4 // minimum time between 2 idles in ms

#if WINDOWS
static struct tagMSG windowsMessage;
#endif

//-----------------------------------------------------------------------------
// PluginGUIEditor Implementation
//-----------------------------------------------------------------------------
/*! @class PluginGUIEditor
This is the same as the AEffGUIEditor class except that this one allows
the VSTGUI lib to build without VST dependencies.
*/
PluginGUIEditor::PluginGUIEditor (void *pEffect) 
	: effect (pEffect), inIdleStuff (false)
{
	systemWindow = 0;
	lLastTicks   = getTicks ();

	#if WINDOWS
	OleInitialize (0);
	#endif
	#if MAC
	void InitMachOLibrary ();
	InitMachOLibrary ();
	#endif
}

//-----------------------------------------------------------------------------
PluginGUIEditor::~PluginGUIEditor () 
{
	#if WINDOWS
	OleUninitialize ();
	#endif
	#if MAC
	void ExitMachOLibrary ();
	ExitMachOLibrary ();
	#endif
}

//-----------------------------------------------------------------------------
void PluginGUIEditor::draw (ERect *ppErect)
{
	if (frame)
	{
		CDrawContext context (frame, NULL, systemWindow);
		if (ppErect)
		{
			CRect r (ppErect->left, ppErect->top, ppErect->right, ppErect->bottom);
			frame->drawRect (&context, r);
		}
		else
			frame->draw (&context);
	}
}

//-----------------------------------------------------------------------------
bool PluginGUIEditor::open (void *ptr)
{
	systemWindow = ptr;
	return true;
}

//-----------------------------------------------------------------------------
void PluginGUIEditor::idle ()
{
	if (frame)
		frame->idle ();
}

//-----------------------------------------------------------------------------
long PluginGUIEditor::knobMode = kCircularMode;

//-----------------------------------------------------------------------------
long PluginGUIEditor::setKnobMode (int val) 
{
	PluginGUIEditor::knobMode = val;
	return 1;
}

//-----------------------------------------------------------------------------
void PluginGUIEditor::wait (unsigned long ms)
{
	#if MAC
	std::this_thread::sleep_for (std::chrono::milliseconds (ms));

	#elif WINDOWS
	Sleep (ms);

	#endif
}

//-----------------------------------------------------------------------------
unsigned long PluginGUIEditor::getTicks ()
{
	#if MAC
	return (unsigned long)std::chrono::duration_cast<std::chrono::milliseconds> (
		std::chrono::steady_clock::now ().time_since_epoch ()).count ();

	#elif WINDOWS
	return (unsigned long)GetTickCount ();

	#endif

	return 0;
}

//-----------------------------------------------------------------------------
void PluginGUIEditor::doIdleStuff ()
{
	// get the current time
	unsigned long currentTicks = getTicks ();

	// YG TEST idle ();
	if (currentTicks < lLastTicks)
	{
		wait (kIdleRateMin);
		currentTicks += kIdleRateMin;
		if (currentTicks < lLastTicks - kIdleRate2)
			return;
	}
	idle (); // TEST

	#if WINDOWS
	if (PeekMessage (&windowsMessage, NULL, WM_PAINT, WM_PAINT, PM_REMOVE))
		DispatchMessage (&windowsMessage);
	#endif

	// save the next time
 	lLastTicks = currentTicks + kIdleRate;
}

//-----------------------------------------------------------------------------
long PluginGUIEditor::getRect (ERect **ppErect)
{
	*ppErect = &rect;
	return true;
}

#if MAC
// -----------------------------------------------------------------------------
// The plugin is shipped as a bare .dylib (not a bundle), so there is no
// CFBundle to derive resource paths from. Bitmaps are resolved relative to the
// dylib's own directory by CBitmap::loadFromResource, so no bundle reference is
// required here.
// -----------------------------------------------------------------------------
BEGIN_NAMESPACE_VSTGUI

void* gBundleRef = 0;

END_NAMESPACE_VSTGUI

void InitMachOLibrary () {}
void ExitMachOLibrary () {}
#endif
