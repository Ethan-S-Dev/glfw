//========================================================================
// GLFW - An OpenGL library
// Platform:    X11
// API version: 3.0
// WWW:         http://www.glfw.org/
//------------------------------------------------------------------------
// Copyright (c) 2010 Camilla Berglund <elmindreda@elmindreda.org>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would
//    be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not
//    be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source
//    distribution.
//
//========================================================================

#include "internal.h"

#include <limits.h>
#include <string.h>


//////////////////////////////////////////////////////////////////////////
//////                       GLFW internal API                      //////
//////////////////////////////////////////////////////////////////////////

//========================================================================
// Detect gamma ramp support and save original gamma ramp, if available
//========================================================================

void _glfwInitGammaRamp(void)
{
    // RandR gamma support is only available with version 1.2 and above
    if (_glfw.x11.randr.available &&
        (_glfw.x11.randr.versionMajor > 1 ||
         (_glfw.x11.randr.versionMajor == 1 &&
          _glfw.x11.randr.versionMinor >= 2)))
    {
        // FIXME: Assumes that all monitors have the same size gamma tables
        // This is reasonable as I suspect the that if they did differ, it
        // would imply that setting the gamma size to an arbitary size is
        // possible as well.
        XRRScreenResources* rr = XRRGetScreenResources(_glfw.x11.display,
                                                       _glfw.x11.root);

        _glfw.originalRampSize = XRRGetCrtcGammaSize(_glfw.x11.display,
                                                     rr->crtcs[0]);
        if (_glfw.originalRampSize == 0)
        {
            // This is probably older Nvidia RandR with broken gamma support
            // Flag it as useless and try Xf86VidMode below, if available
            _glfw.x11.randr.gammaBroken = GL_TRUE;
        }

        XRRFreeScreenResources(rr);
    }

    if (_glfw.x11.vidmode.available && !_glfw.originalRampSize)
    {
        // Get the gamma size using XF86VidMode
        XF86VidModeGetGammaRampSize(_glfw.x11.display,
                                    _glfw.x11.screen,
                                    &_glfw.originalRampSize);
    }

    if (_glfw.originalRampSize)
    {
        // Save the original gamma ramp
        _glfwPlatformGetGammaRamp(&_glfw.originalRamp);
    }
}


//========================================================================
// Restore original gamma ramp if necessary
//========================================================================

void _glfwTerminateGammaRamp(void)
{
    if (_glfw.originalRampSize && _glfw.rampChanged)
        _glfwPlatformSetGammaRamp(&_glfw.originalRamp);
}


//////////////////////////////////////////////////////////////////////////
//////                       GLFW platform API                      //////
//////////////////////////////////////////////////////////////////////////

void _glfwPlatformGetGammaRamp(GLFWgammaramp* ramp)
{
    // For now, don't support anything that is not GLFW_GAMMA_RAMP_SIZE
    if (_glfw.originalRampSize != GLFW_GAMMA_RAMP_SIZE)
    {
        _glfwInputError(GLFW_PLATFORM_ERROR,
                        "X11: Failed to get gamma ramp due to size "
                        "incompatibility");
        return;
    }

    if (_glfw.x11.randr.available && !_glfw.x11.randr.gammaBroken)
    {
        size_t size = GLFW_GAMMA_RAMP_SIZE * sizeof(unsigned short);

        XRRScreenResources* rr = XRRGetScreenResources(_glfw.x11.display,
                                                       _glfw.x11.root);

        XRRCrtcGamma* gamma = XRRGetCrtcGamma(_glfw.x11.display,
                                              rr->crtcs[0]);

        // TODO: Handle case of original ramp size having a size other than 256

        memcpy(ramp->red, gamma->red, size);
        memcpy(ramp->green, gamma->green, size);
        memcpy(ramp->blue, gamma->blue, size);

        XRRFreeGamma(gamma);
        XRRFreeScreenResources(rr);
    }
    else if (_glfw.x11.vidmode.available)
    {
        XF86VidModeGetGammaRamp(_glfw.x11.display,
                                _glfw.x11.screen,
                                GLFW_GAMMA_RAMP_SIZE,
                                ramp->red,
                                ramp->green,
                                ramp->blue);
    }
}

void _glfwPlatformSetGammaRamp(const GLFWgammaramp* ramp)
{
    // For now, don't support anything that is not GLFW_GAMMA_RAMP_SIZE
    if (_glfw.originalRampSize != GLFW_GAMMA_RAMP_SIZE)
    {
        _glfwInputError(GLFW_PLATFORM_ERROR,
                        "X11: Failed to set gamma ramp due to size "
                        "incompatibility");
        return;
    }

    if (_glfw.x11.randr.available && !_glfw.x11.randr.gammaBroken)
    {
        int i;
        size_t size = GLFW_GAMMA_RAMP_SIZE * sizeof(unsigned short);

        XRRScreenResources* rr = XRRGetScreenResources(_glfw.x11.display,
                                                       _glfw.x11.root);

        // Update gamma per monitor
        for (i = 0;  i < rr->ncrtc;  i++)
        {
            XRRCrtcGamma* gamma = XRRAllocGamma(GLFW_GAMMA_RAMP_SIZE);

            memcpy(gamma->red, ramp->red, size);
            memcpy(gamma->green, ramp->green, size);
            memcpy(gamma->blue, ramp->blue, size);

            XRRSetCrtcGamma(_glfw.x11.display, rr->crtcs[i], gamma);
            XRRFreeGamma(gamma);
        }

        XRRFreeScreenResources(rr);
    }
    else if (_glfw.x11.vidmode.available)
    {
        XF86VidModeSetGammaRamp(_glfw.x11.display,
                                _glfw.x11.screen,
                                GLFW_GAMMA_RAMP_SIZE,
                                (unsigned short*) ramp->red,
                                (unsigned short*) ramp->green,
                                (unsigned short*) ramp->blue);
    }
}

