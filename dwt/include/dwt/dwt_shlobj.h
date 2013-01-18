/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2013, Jacek Sieka

  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

      * Redistributions of source code must retain the above copyright notice,
        this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright notice,
        this list of conditions and the following disclaimer in the documentation
        and/or other materials provided with the distribution.
      * Neither the name of the DWT nor SmartWin++ nor the names of its contributors
        may be used to endorse or promote products derived from this software
        without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef DWT_SHLOBJ_H
#define DWT_SHLOBJ_H

#include <shlobj.h>

#ifdef __GNUC__

// defs for ITaskbarList, ITaskbarList2, ITaskbarList3 (incomplete - just the parts that we need)
/// @todo MinGW-w64 seems to be implementing these, they have version 1 & 2, but not 3 yet... remove this when they have it.

#ifndef __ITaskbarList_FWD_DEFINED__
#define __ITaskbarList_FWD_DEFINED__
typedef struct ITaskbarList ITaskbarList;
#endif

#ifndef __ITaskbarList2_FWD_DEFINED__
#define __ITaskbarList2_FWD_DEFINED__
typedef struct ITaskbarList2 ITaskbarList2;
#endif

#ifndef __ITaskbarList3_FWD_DEFINED__
#define __ITaskbarList3_FWD_DEFINED__
typedef struct ITaskbarList3 ITaskbarList3;
#endif

#ifndef __ITaskbarList_INTERFACE_DEFINED__
#define __ITaskbarList_INTERFACE_DEFINED__
#if defined(__cplusplus) && !defined(CINTERFACE)
  struct ITaskbarList : public IUnknown {
  public:
    virtual HRESULT WINAPI HrInit(void) = 0;
    virtual HRESULT WINAPI AddTab(HWND hwnd) = 0;
    virtual HRESULT WINAPI DeleteTab(HWND hwnd) = 0;
    virtual HRESULT WINAPI ActivateTab(HWND hwnd) = 0;
    virtual HRESULT WINAPI SetActiveAlt(HWND hwnd) = 0;
  };
#endif
#endif

#ifndef __ITaskbarList2_INTERFACE_DEFINED__
#define __ITaskbarList2_INTERFACE_DEFINED__
#if defined(__cplusplus) && !defined(CINTERFACE)
  struct ITaskbarList2 : public ITaskbarList {
  public:
    virtual HRESULT WINAPI MarkFullscreenWindow(HWND hwnd,WINBOOL fFullscreen) = 0;
  };
#endif
#endif

#ifndef __ITaskbarList3_INTERFACE_DEFINED__
#define __ITaskbarList3_INTERFACE_DEFINED__
#if defined(__cplusplus) && !defined(CINTERFACE)
  struct ITaskbarList3 : public ITaskbarList2 {
  public:
        virtual HRESULT WINAPI SetProgressValue( 
            /* [in] */ HWND hwnd,
            /* [in] */ ULONGLONG ullCompleted,
            /* [in] */ ULONGLONG ullTotal) = 0;
        
        virtual HRESULT WINAPI SetProgressState( 
            /* [in] */ HWND hwnd,
            /* [in] */ /*TBPFLAG*/int tbpFlags) = 0;
        
        virtual HRESULT WINAPI RegisterTab( 
            /* [in] */ HWND hwndTab,
            /* [in] */ HWND hwndMDI) = 0;
        
        virtual HRESULT WINAPI UnregisterTab( 
            /* [in] */ HWND hwndTab) = 0;
        
        virtual HRESULT WINAPI SetTabOrder( 
            /* [in] */ HWND hwndTab,
            /* [in] */ HWND hwndInsertBefore) = 0;
        
        virtual HRESULT WINAPI SetTabActive( 
            /* [in] */ HWND hwndTab,
            /* [in] */ HWND hwndMDI,
            /* [in] */ DWORD dwReserved) = 0;
        
        virtual HRESULT WINAPI ThumbBarAddButtons( 
            /* [in] */ HWND hwnd,
            /* [in] */ UINT cButtons,
            /* [size_is][in] */ /*LPTHUMBBUTTON*/void* pButton) = 0;
        
        virtual HRESULT WINAPI ThumbBarUpdateButtons( 
            /* [in] */ HWND hwnd,
            /* [in] */ UINT cButtons,
            /* [size_is][in] */ /*LPTHUMBBUTTON*/void* pButton) = 0;
        
        virtual HRESULT WINAPI ThumbBarSetImageList( 
            /* [in] */ HWND hwnd,
            /* [in] */ /*HIMAGELIST*/void* himl) = 0;
        
        virtual HRESULT WINAPI SetOverlayIcon( 
            /* [in] */ HWND hwnd,
            /* [in] */ HICON hIcon,
            /* [string][unique][in] */ LPCWSTR pszDescription) = 0;
        
        virtual HRESULT WINAPI SetThumbnailTooltip( 
            /* [in] */ HWND hwnd,
            /* [string][unique][in] */ LPCWSTR pszTip) = 0;
        
        virtual HRESULT WINAPI SetThumbnailClip( 
            /* [in] */ HWND hwnd,
            /* [in] */ RECT *prcClip) = 0;
  };
#endif
#endif

#endif
#endif
