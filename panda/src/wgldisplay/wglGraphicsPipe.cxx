// Filename: wglGraphicsPipe.cxx
// Created by:  drose (20Dec02)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) 2001, Disney Enterprises, Inc.  All rights reserved
//
// All use of this software is subject to the terms of the Panda 3d
// Software license.  You should have received a copy of this license
// along with this source code; you will also find a current copy of
// the license at http://www.panda3d.org/license.txt .
//
// To contact the maintainers of this program write to
// panda3d@yahoogroups.com .
//
////////////////////////////////////////////////////////////////////

#include "wglGraphicsPipe.h"
#include "config_wgldisplay.h"
#include "config_windisplay.h"
#include "wglGraphicsWindow.h"
#include "wglGraphicsBuffer.h"

typedef enum {Software, MCD, ICD} OGLDriverType;
static const char * const OGLDrvStrings[] = { "Software", "MCD", "ICD" };

TypeHandle wglGraphicsPipe::_type_handle;
  

////////////////////////////////////////////////////////////////////
//     Function: wglGraphicsPipe::Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
wglGraphicsPipe::
wglGraphicsPipe() {
}

////////////////////////////////////////////////////////////////////
//     Function: wglGraphicsPipe::Destructor
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
wglGraphicsPipe::
~wglGraphicsPipe() {
}

////////////////////////////////////////////////////////////////////
//     Function: wglGraphicsPipe::get_interface_name
//       Access: Published, Virtual
//  Description: Returns the name of the rendering interface
//               associated with this GraphicsPipe.  This is used to
//               present to the user to allow him/her to choose
//               between several possible GraphicsPipes available on a
//               particular platform, so the name should be meaningful
//               and unique for a given platform.
////////////////////////////////////////////////////////////////////
string wglGraphicsPipe::
get_interface_name() const {
  return "OpenGL";
}

////////////////////////////////////////////////////////////////////
//     Function: wglGraphicsPipe::pipe_constructor
//       Access: Public, Static
//  Description: This function is passed to the GraphicsPipeSelection
//               object to allow the user to make a default
//               wglGraphicsPipe.
////////////////////////////////////////////////////////////////////
PT(GraphicsPipe) wglGraphicsPipe::
pipe_constructor() {
  return new wglGraphicsPipe;
}

////////////////////////////////////////////////////////////////////
//     Function: wglGraphicsPipe::make_gsg
//       Access: Protected, Virtual
//  Description: Creates a new GSG to use the pipe (but no windows
//               have been created yet for the GSG).  This method will
//               be called in the draw thread for the GSG.
////////////////////////////////////////////////////////////////////
PT(GraphicsStateGuardian) wglGraphicsPipe::
make_gsg(const FrameBufferProperties &properties) {
  if (!_is_valid) {
    return NULL;
  }

  // Make a copy of the supplied properties so we can possibly modify
  // them to suit our available properties.
  FrameBufferProperties new_properties = properties;

  // We need a DC to examine the available pixel formats.  We'll use
  // the screen DC.
  HDC hdc = GetDC(NULL);
  int pfnum = choose_pfnum(new_properties, hdc);
  ReleaseDC(NULL, hdc);

  if (gl_force_pixfmt != 0) {
    wgldisplay_cat.info()
      << "overriding pixfmt choice algorithm (" << pfnum 
      << ") with gl-force-pixfmt(" << gl_force_pixfmt << ")\n";
    pfnum = gl_force_pixfmt;
  }

  if (wgldisplay_cat.is_debug()) {
    wgldisplay_cat.debug()
      << "config() - picking pixfmt #" << pfnum <<endl;
  }

  PT(wglGraphicsStateGuardian) gsg = 
    new wglGraphicsStateGuardian(properties, pfnum);

  return gsg.p();
}

////////////////////////////////////////////////////////////////////
//     Function: wglGraphicsPipe::make_window
//       Access: Protected, Virtual
//  Description: Creates a new window on the pipe, if possible.
////////////////////////////////////////////////////////////////////
PT(GraphicsWindow) wglGraphicsPipe::
make_window(GraphicsStateGuardian *gsg) {
  return new wglGraphicsWindow(this, gsg);
}

////////////////////////////////////////////////////////////////////
//     Function: wglGraphicsPipe::make_buffer
//       Access: Protected, Virtual
//  Description: Creates a new offscreen buffer on the pipe, if possible.
////////////////////////////////////////////////////////////////////
PT(GraphicsBuffer) wglGraphicsPipe::
make_buffer(GraphicsStateGuardian *gsg, int x_size, int y_size, 
            bool want_texture) {
  return new wglGraphicsBuffer(this, gsg, x_size, y_size, want_texture);
}

////////////////////////////////////////////////////////////////////
//     Function: wglGraphicsPipe::choose_pfnum
//       Access: Private, Static
//  Description: Selects a suitable pixel format number for the given
//               frame buffer properties.  Returns the selected number
//               if successful, or 0 otherwise.
//
//               If successful, this may modify properties to reflect
//               the actual visual chosen.
////////////////////////////////////////////////////////////////////
int wglGraphicsPipe::
choose_pfnum(FrameBufferProperties &properties, HDC hdc) {
  int pfnum;

  if (force_software_renderer) {
    pfnum = find_pixfmtnum(properties, hdc, false);
    
    if (pfnum == 0) {
      wgldisplay_cat.error()
        << "Couldn't find compatible software-renderer OpenGL pixfmt, check your window properties!\n";
      return 0;
    }

  } else {
    pfnum = find_pixfmtnum(properties, hdc, true);
    if (pfnum == 0) {
      if (allow_software_renderer) {
        pfnum = find_pixfmtnum(properties, hdc, false);
        if (pfnum == 0) {
          wgldisplay_cat.error()
            << "Couldn't find HW or Software OpenGL pixfmt appropriate for this desktop!!\n";
        } else {
          wgldisplay_cat.info()
            << "Couldn't find compatible OGL HW pixelformat, using software rendering.\n";
        }

      } else {
        wgldisplay_cat.error()
          << "Couldn't find HW-accelerated OpenGL pixfmt appropriate for this desktop!!\n";
      }
      
      if (pfnum == 0) {
        wgldisplay_cat.error()
          << "make sure OpenGL driver is installed, and try reducing the screen size, reducing the\n"
          << "desktop screen pixeldepth to 16bpp,and check your panda window properties\n";
        return 0;
      }
    }
  }
  
  return pfnum;
}

////////////////////////////////////////////////////////////////////
//     Function: wglGraphicsPipe::find_pixfmtnum
//       Access: Private, Static
//  Description: This helper routine looks for either HW-only or
//               SW-only format, but not both.  Returns the
//               pixelformat number, or 0 if a suitable format could
//               not be found.
////////////////////////////////////////////////////////////////////
int wglGraphicsPipe::
find_pixfmtnum(FrameBufferProperties &properties, HDC hdc,
               bool bLookforHW) {
  int frame_buffer_mode = properties.get_frame_buffer_mode();
  bool want_depth_bits = properties.has_depth_bits();
  bool want_color_bits = properties.has_color_bits();
  OGLDriverType drvtype;

  PIXELFORMATDESCRIPTOR pfd;
  ZeroMemory(&pfd,sizeof(PIXELFORMATDESCRIPTOR));
  pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
  pfd.nVersion = 1;

  // We have to call DescribePixelFormat() once just to get the
  // highest pfnum available.  Then we can iterate through all of the
  // pfnums.
  int max_pfnum = DescribePixelFormat(hdc, 1, 0, NULL);
  int cur_bpp = GetDeviceCaps(hdc, BITSPIXEL);
  int pfnum = 0;

  for (pfnum = 1; pfnum <= max_pfnum; pfnum++) {
    DescribePixelFormat(hdc, pfnum, sizeof(PIXELFORMATDESCRIPTOR), &pfd);

    // official, nvidia sanctioned way.
    if ((pfd.dwFlags & PFD_GENERIC_FORMAT) != 0) {
      drvtype = Software;
    } else if (pfd.dwFlags & PFD_GENERIC_ACCELERATED) {
      drvtype = MCD;
    } else {
      drvtype = ICD;
    }

    // skip driver types we are not looking for
    if (bLookforHW) {
      if (drvtype == Software) {
        continue;
      }
    } else {
      if (drvtype != Software) {
        continue;
      }
    }

    if ((pfd.iPixelType == PFD_TYPE_COLORINDEX) && 
        (frame_buffer_mode & FrameBufferProperties::FM_index) == 0) {
      continue;
    }

    DWORD dwReqFlags = (PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW);
    if ((frame_buffer_mode & FrameBufferProperties::FM_double_buffer) != 0) {
      dwReqFlags|= PFD_DOUBLEBUFFER;
    }

    if (wgldisplay_cat.is_debug()) {
      wgldisplay_cat.debug()
        << "----------------" << endl;

      if ((frame_buffer_mode & FrameBufferProperties::FM_alpha) != 0) {
        wgldisplay_cat.debug()
          << "want alpha, pfd says '"
          << (int)(pfd.cAlphaBits) << "'" << endl;
      }
      if ((frame_buffer_mode & FrameBufferProperties::FM_depth) != 0) {
        wgldisplay_cat.debug()
          << "want depth, pfd says '"
          << (int)(pfd.cDepthBits) << "'" << endl;
      }
      if ((frame_buffer_mode & FrameBufferProperties::FM_stencil) != 0) {
        wgldisplay_cat.debug()
          << "want stencil, pfd says '"
          << (int)(pfd.cStencilBits) << "'" << endl;
      }
      wgldisplay_cat.debug()
        << "final flag check " << (int)(pfd.dwFlags & dwReqFlags) << " =? "
        << (int)dwReqFlags << endl;
      wgldisplay_cat.debug() 
        << "pfd bits = " << (int)(pfd.cColorBits) << endl;
      wgldisplay_cat.debug() 
        << "cur_bpp = " << cur_bpp << endl;
    }

    if ((frame_buffer_mode & FrameBufferProperties::FM_alpha) != 0 && 
        (pfd.cAlphaBits==0)) {
      wgldisplay_cat.debug() 
        << "  rejecting because alpha is missing.\n";
      continue;
    }
    if ((frame_buffer_mode & FrameBufferProperties::FM_depth) != 0 && 
        (pfd.cDepthBits==0)) {
      wgldisplay_cat.debug() 
        << "  rejecting because depth is missing.\n";
      continue;
    }
    if ((frame_buffer_mode & FrameBufferProperties::FM_stencil) != 0 && 
        (pfd.cStencilBits==0)) {
      wgldisplay_cat.debug() 
        << "  rejecting because stencil is missing.\n";
      continue;
    }

    if ((pfd.dwFlags & dwReqFlags) != dwReqFlags) {
      wgldisplay_cat.debug() 
        << "  rejecting because some other required flags are missing.\n";
      continue;
    }

    // now we ignore the specified want_color_bits for windowed mode
    // instead we use the current screen depth

    // drose: Does this help anything?  Checking the current screen
    // depth doesn't make sense if we are rendering offscreen or if we
    // are planning to open a fullscreen window, and it seems like it
    // shouldn't be necessary anyway.
    if ((pfd.cColorBits!=cur_bpp) && 
        (!((cur_bpp==16) && (pfd.cColorBits==15))) && 
        (!((cur_bpp==32) && (pfd.cColorBits==24)))) {
      continue;
    }

    // We've passed all the tests, go ahead and pick this fmt.
    // Note: could go continue looping looking for more alpha bits or
    // more depth bits so this would pick 16bpp depth buffer, probably
    // not 24bpp
    return pfnum;
  }

  // No pfnum was acceptable.
  return 0;
}
