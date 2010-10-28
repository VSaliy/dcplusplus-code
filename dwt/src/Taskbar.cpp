/*
  DC++ Widget Toolkit

  Copyright (c) 2007-2010, Jacek Sieka

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

#include <dwt/Taskbar.h>

#include <dwt/dwt_dwmapi.h>
#include <dwt/LibraryLoader.h>
#include <dwt/util/check.h>
#include <dwt/util/win32/Version.h>
#include <dwt/widgets/Container.h>
#include <dwt/widgets/Window.h>

namespace dwt {

static const UINT taskbarButtonMsg = ::RegisterWindowMessage(_T("TaskbarButtonCreated"));

typedef HRESULT (WINAPI *t_DwmInvalidateIconicBitmaps)(HWND);
static t_DwmInvalidateIconicBitmaps DwmInvalidateIconicBitmaps;

typedef HRESULT (WINAPI *t_DwmSetIconicLivePreviewBitmap)(HWND, HBITMAP, POINT*, DWORD);
static t_DwmSetIconicLivePreviewBitmap DwmSetIconicLivePreviewBitmap;

typedef HRESULT (WINAPI *t_DwmSetIconicThumbnail)(HWND, HBITMAP, DWORD);
static t_DwmSetIconicThumbnail DwmSetIconicThumbnail;

typedef HRESULT (WINAPI *t_DwmSetWindowAttribute)(HWND, DWORD, LPCVOID, DWORD);
static t_DwmSetWindowAttribute DwmSetWindowAttribute;

Taskbar::Taskbar(const ActivateF& activateF_) :
taskbar(0),
window(0),
activateF(activateF_)
{
}

Taskbar::~Taskbar() {
	if(taskbar)
		taskbar->Release();
}

void Taskbar::initTaskbar(WindowPtr window_) {
	if(!util::win32::ensureVersion(util::win32::SEVEN))
		return;

	/// @todo call ChangeWindowMessageFilterEx on WM_DWMSENDICONICTHUMBNAIL & WM_DWMSENDICONICLIVEPREVIEWBITMAP

	static LibraryLoader lib(_T("dwmapi"), true);
	if(lib.loaded()) {

#define get(name) if(!name) { if(!(name = reinterpret_cast<t_##name>(lib.getProcAddress(_T(#name))))) { return; } }
		get(DwmInvalidateIconicBitmaps);
		get(DwmSetIconicLivePreviewBitmap);
		get(DwmSetIconicThumbnail);
		get(DwmSetWindowAttribute);
#undef get

		window = window_;
		dwtassert(window, _T("Taskbar: no widget set"));

		// init the COM pointer on reception of the "TaskbarButtonCreated" message.
		window->onRaw([this](WPARAM, LPARAM) -> LRESULT {
			if(!taskbar) {
				if(::CoCreateInstance(CLSID_TaskbarList, 0, CLSCTX_INPROC_SERVER, IID_ITaskbarList,
					reinterpret_cast<LPVOID*>(&taskbar)) != S_OK) { taskbar = 0; }
				if(taskbar && taskbar->HrInit() != S_OK) {
					taskbar->Release();
					taskbar = 0;
				}
			}
			return 0;
		}, Message(taskbarButtonMsg));
	}
}

void Taskbar::addToTaskbar(ContainerPtr tab) {
	/* for Windows to acknowledge that our tab window is worthy of having its own thumbnail in the
	taskbar, we have to create an invisible popup window that will act as a proxy between the
	taskbar and the actual tab window.
	this technique is illustrated in MFC as well as the Windows SDK sample at
	"Samples\winui\shell\appshellintegration\TabThumbnails". */

	ContainerPtr proxy;
	{
		Container::Seed seed;
		seed.caption = tab->getText();
		seed.style &= ~WS_VISIBLE;
		seed.style |= WS_POPUP | WS_CAPTION;
		seed.exStyle |= WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
		seed.location = Rectangle();
		proxy = window->addChild(seed);
	}
	tabs[tab] = proxy;

	// keep the proxy window in sync with the actual tab window.
	tab->onTextChanging([proxy](const tstring& text) { proxy->setText(text); });
	tab->onSized([proxy](const SizedEvent&) { DwmInvalidateIconicBitmaps(proxy->handle()); });

	proxy->onActivate([this, tab](bool activate) {
		if(activate)
			activateF(tab);
	});
	proxy->onClosing([tab]() -> bool {
		tab->close(true);
		return false; // don't close the proxy window just yet; wait for removeFromTaskbar.
	});

	proxy->onRaw([this, tab, proxy](WPARAM, LPARAM lParam) -> LRESULT {
		// generate a thumbnail to be displayed in the taskbar.
		BitmapPtr bitmap = getBitmap(tab, lParam);
		if(bitmap) {
			DwmSetIconicThumbnail(proxy->handle(), bitmap->handle(), 0);
		}
		return 0;
	}, Message(WM_DWMSENDICONICTHUMBNAIL));

	proxy->onRaw([this, tab, proxy](WPARAM, LPARAM) -> LRESULT {
		// generate a preview of the tab to be shown in "Aero Peek" when the user hovers the thumbnail.
		BitmapPtr bitmap = getBitmap(tab, 0);
		if(bitmap) {
			Point offset;
			::MapWindowPoints(tab->handle(), window->handle(), &offset, 1);
			MENUBARINFO info = { sizeof(MENUBARINFO) };
			if(::GetMenuBarInfo(window->handle(), OBJID_MENU, 0, &info))
				offset.y += Rectangle(info.rcBar).height();
			DwmSetIconicLivePreviewBitmap(proxy->handle(), bitmap->handle(), &offset, 0);
		}
		return 0;
	}, Message(WM_DWMSENDICONICLIVEPREVIEWBITMAP));

	// indicate to the window manager that it should always use the bitmaps we provide.
	BOOL attrib = TRUE;
	DwmSetWindowAttribute(proxy->handle(), DWMWA_FORCE_ICONIC_REPRESENTATION, &attrib, sizeof(attrib));
	DwmSetWindowAttribute(proxy->handle(), DWMWA_HAS_ICONIC_BITMAP, &attrib, sizeof(attrib));

	taskbar->RegisterTab(proxy->handle(), window->handle());
	moveOnTaskbar(tab);
}

void Taskbar::removeFromTaskbar(ContainerPtr tab) {
	ContainerPtr proxy = tabs[tab];
	taskbar->UnregisterTab(proxy->handle());
	::DestroyWindow(proxy->handle());
	tabs.erase(tab);
}

void Taskbar::moveOnTaskbar(ContainerPtr tab, ContainerPtr rightNeighbor) {
	taskbar->SetTabOrder(tabs[tab]->handle(), rightNeighbor ? tabs[rightNeighbor]->handle() : 0);
}

void Taskbar::setActiveOnTaskbar(ContainerPtr tab) {
	taskbar->SetTabActive(tabs[tab]->handle(), window->handle(), 0);
}

void Taskbar::setTaskbarIcon(ContainerPtr tab, const IconPtr& icon) {
	tabs[tab]->sendMessage(WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(icon->handle()));
}

BitmapPtr Taskbar::getBitmap(ContainerPtr tab, LPARAM thumbnailSize) {
	UpdateCanvas canvas(tab);

	// get the actual size of the tab.
	const Point size_full(tab->getClientSize());
	if(size_full.x <= 0 || size_full.y <= 0)
		return 0;

	// this DIB will hold a full capture of the tab.
	BITMAPINFO info = { { sizeof(BITMAPINFOHEADER), size_full.x, size_full.y, 1, 32, BI_RGB } };
	BitmapPtr bitmap_full(new Bitmap(::CreateDIBSection(canvas.handle(), &info, DIB_RGB_COLORS, 0, 0, 0)));

	FreeCanvas canvas_full(::CreateCompatibleDC(canvas.handle()));
	Canvas::Selector select_full(canvas_full, *bitmap_full);

	tab->sendMessage(WM_PRINT, reinterpret_cast<WPARAM>(canvas_full.handle()), PRF_CLIENT | PRF_NONCLIENT | PRF_CHILDREN | PRF_ERASEBKGND);

	// get rid of some transparent bits.
	::BitBlt(canvas_full.handle(), 0, 0, size_full.x, size_full.y, canvas_full.handle(), 0, 0, MERGECOPY);

	if(!thumbnailSize)
		return bitmap_full;

	// compute the size of the thumbnail, must not exceed the LPARAM values.
	double factor = std::min(static_cast<double>(HIWORD(thumbnailSize)) / static_cast<double>(size_full.x),
		static_cast<double>(LOWORD(thumbnailSize)) / static_cast<double>(size_full.y));
	const Point size_thumb(size_full.x * factor, size_full.y * factor);
	if(size_thumb.x <= 0 || size_thumb.y <= 0)
		return 0;

	// this DIB will hold a resized view of the tab, to be used as a thumbnail.
	info.bmiHeader.biWidth = size_thumb.x;
	info.bmiHeader.biHeight = size_thumb.y;
	BitmapPtr bitmap_thumb(new Bitmap(::CreateDIBSection(canvas.handle(), &info, DIB_RGB_COLORS, 0, 0, 0)));

	FreeCanvas canvas_thumb(::CreateCompatibleDC(canvas.handle()));
	Canvas::Selector select_thumb(canvas_thumb, *bitmap_thumb);

	::SetStretchBltMode(canvas_thumb.handle(), HALFTONE);
	::SetBrushOrgEx(canvas_thumb.handle(), 0, 0, 0);
	::StretchBlt(canvas_thumb.handle(), 0, 0, size_thumb.x, size_thumb.y,
		canvas_full.handle(), 0, 0, size_full.x, size_full.y, SRCCOPY);

	return bitmap_thumb;
}

}
