 /*
 * Copyright (C) 2001-2010 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "stdafx.h"

#include "SingleInstance.h"
#include "WinUtil.h"
#include "MainWindow.h"
#include "SplashWindow.h"
#include "UPnP_COM.h"
#include "UPnP_MiniUPnPc.h"

#include <dcpp/MerkleTree.h>
#include <dcpp/File.h>
#include <dcpp/Text.h>
#include <dcpp/ResourceManager.h>
#include <dcpp/UPnPManager.h>

#define WMU_WHERE_ARE_YOU_MSG _T("WMU_WHERE_ARE_YOU-{885D4B75-6606-4add-A8DE-EEEDC04181F1}")

const UINT SingleInstance::WMU_WHERE_ARE_YOU = ::RegisterWindowMessage(WMU_WHERE_ARE_YOU_MSG);

static void sendCmdLine(HWND hOther, const tstring& cmdLine) {
	COPYDATASTRUCT cpd;
	cpd.dwData = 0;
	cpd.cbData = sizeof(TCHAR)*(cmdLine.length() + 1);
	cpd.lpData = (void *)cmdLine.c_str();
	::SendMessage(hOther, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&cpd));
}

BOOL CALLBACK searchOtherInstance(HWND hWnd, LPARAM lParam) {
	ULONG_PTR result;
	if(::SendMessageTimeout(hWnd, SingleInstance::WMU_WHERE_ARE_YOU, 0, 0, SMTO_BLOCK | SMTO_ABORTIFHUNG, 5000, &result) &&
		result == SingleInstance::WMU_WHERE_ARE_YOU) {
		// found it
		HWND *target = (HWND *)lParam;
		*target = hWnd;
		return FALSE;
	}
	return TRUE;
}

void callBack(void* ptr, const string& a) {
	SplashWindow& splash = *((SplashWindow*)ptr);
	splash(a);
}

#ifdef _DEBUG
void (*old_handler)();

// Dummy function to have something to break at
void term_handler() {
	old_handler();
}
#endif

int SmartWinMain(dwt::Application& app) {
	dcdebug("StartWinMain\n");

	WinUtil::enableDEP();

	// http://www.kb.cert.org/vuls/id/707943 part III, "For Developers".
	::SetDllDirectory(_T(""));

	Util::initialize();

	string configPathHash;
	{
		std::string configPath = Util::getPath(Util::PATH_USER_CONFIG);
		TigerTree tth(TigerTree::calcBlockSize(configPath.size(), 1));
		tth.update(configPath.data(), configPath.size());
		tth.finalize();
		configPathHash = tth.getRoot().toBase32();
	}
#ifndef _DEBUG
	SingleInstance dcapp(Text::toT(string("{DCPLUSPLUS-AEE8350A-B49A-4753-AB4B-E55479A48351}") + configPathHash).c_str());
	if(dcapp.isRunning()) {
#else
	SingleInstance dcapp(Text::toT(string("{DCPLUSPLUS-AEE8350A-B49A-4753-AB4B-E55479A48350}") + configPathHash).c_str());
	if(dcapp.isRunning() && app.getCommandLine().getParams().size() > 1) {
#endif
		HWND hOther = NULL;
		::EnumWindows(&searchOtherInstance, (LPARAM)&hOther);

		if(hOther) {
			// pop up
			::SetForegroundWindow(hOther);

			if( ::IsIconic(hOther)) {
				// restore
				::ShowWindow(hOther, SW_RESTORE);
			}
			sendCmdLine(hOther, app.getCommandLine().getParamsRaw());
		}

		return 1;
	}

#ifdef _DEBUG
	old_handler = set_terminate(&term_handler);

#ifdef __MINGW32__
	// For debugging
	::LoadLibrary(_T("exchndl.dll"));
#endif
#endif

	// For SHBrowseForFolder, UPnP_COM
	HRESULT hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if(FAILED(hr))
		return hr;

	try {
		std::string module = File(Text::fromT(app.getModuleFileName()), File::READ, File::OPEN).read();
		TigerTree tth(TigerTree::calcBlockSize(module.size(), 1));
		tth.update(module.data(), module.size());
		tth.finalize();
		WinUtil::tth = Text::toT(tth.getRoot().toBase32());
	} catch(const FileException&) {
		dcdebug("Failed reading exe\n");
	}

	try {
		SplashWindow* splash(new SplashWindow);
		startup(&callBack, splash);

		bindtextdomain(PACKAGE, LOCALEDIR);
		textdomain(PACKAGE);

		if(ResourceManager::getInstance()->isRTL()) {
			SetProcessDefaultLayout(LAYOUT_RTL);
		}

		UPnPManager::getInstance()->addImplementation(new UPnP_MiniUPnPc());
		UPnPManager::getInstance()->addImplementation(new UPnP_COM());

		WinUtil::init();
		MainWindow* wnd = new MainWindow;
		WinUtil::mainWindow = wnd;
		//WinUtil::mdiParent = wnd->getMDIParent();
		splash->close();
		app.run();
	} catch(const std::exception& e) {
		printf("Exception: %s\n", e.what());
	} catch(...) {
		printf("Unknown exception");
	}
	WinUtil::uninit();

	shutdown();

	::CoUninitialize();

	return 0;
}

