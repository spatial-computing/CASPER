// Copyright 2010 ESRI
//
// All rights reserved under the copyright laws of the United States
// and applicable international laws, treaties, and conventions.
//
// You may freely redistribute and use this sample code, with or
// without modification, provided you include the original copyright
// notice and use restrictions.
//
// See the use restrictions at http://help.arcgis.com/en/sdk/10.0/usageRestrictions.htm

// ===============================================================================================
// Evacuation Solver: Project-wide macros, includes, and imports
// Description: Can be used with visual studio pre-compiled header option
//
// Copyright (C) 2014 Kaveh Shahabi
// Distributed under the Apache Software License, Version 2.0. (See accompanying file LICENSE.txt)
//
// Author: Kaveh Shahabi
// URL: http://github.com/spatial-computing/CASPER
// ===============================================================================================

#pragma once

#ifndef STRICT
#define STRICT
#endif

#define _CRTDBG_MAP_ALLOC

// ref: http://msdn.microsoft.com/en-us/library/windows/desktop/ms683219%28v=vs.85%29.aspx
#define PSAPI_VERSION 1

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER				// Allow use of features specific to Windows XP or later.
#define WINVER 0x0501		// Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE			// Allow use of features specific to IE 6.0 or later.
#define _WIN32_IE 0x0600	// Change this to the appropriate value to target other versions of IE.
#endif

#define _ATL_APARTMENT_THREADED
#define _ATL_NO_AUTOMATIC_NAMESPACE

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

#pragma component(browser, off, references)
#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <atltypes.h>
#include <atlctl.h>
#include <atlhost.h>
#pragma component(browser, on, references)

// So I did all of these extra 'using's just so I can avoid doinbg 'using namespace ATL'
using ATL::CComCoClass;
using ATL::CComObjectRootEx;
using ATL::CComSingleThreadModel;
using ATL::IProvideClassInfoImpl;
using ATL::IDispatchImpl;
using ATL::CComCreator;
using ATL::CComObject;
using ATL::RGSDWORD;
using ATL::RGSBinary;
using ATL::RGSStrings;
using ATL::CRegistryVirtualMachine;

#pragma warning(push)
#pragma warning(disable : 4336) /* Ignore warnings for order of imports */

#ifdef __INTELLISENSE__
	#ifdef _WIN64
		#import "obj\Debug\x64\esrisystem.tlh"
		#import "obj\Debug\x64\esrisystemui.tlh"
		#import "obj\Debug\x64\esriFramework.tlh"
		#import "obj\Debug\x64\esriGeometry.tlh"
		#import "obj\Debug\x64\esriGeoDatabase.tlh"
		#import "obj\Debug\x64\esriNetworkAnalyst.tlh"
		#import "obj\Debug\x64\esriDisplay.tlh"
		#import "obj\Debug\x64\esriCarto.tlh"
	#else
		#import "obj\Debug\Win32\esrisystem.tlh"
		#import "obj\Debug\Win32\esrisystemui.tlh"
		#import "obj\Debug\Win32\esriFramework.tlh"
		#import "obj\Debug\Win32\esriGeometry.tlh"
		#import "obj\Debug\Win32\esriGeoDatabase.tlh"
		#import "obj\Debug\Win32\esriNetworkAnalyst.tlh"
		#import "obj\Debug\Win32\esriDisplay.tlh"
		#import "obj\Debug\Win32\esriCarto.tlh"
	#endif
#else
    // Be sure to set these paths to the version of the software against which you want to run
    #import "C:\Program Files (x86)\ArcGIS\Desktop10.2\com\esrisystem.olb" named_guids no_namespace raw_interfaces_only no_implementation rename("GetJob", "GetJobESRI") rename("GetObject", "GetObjectESRI") rename("max", "maxESRI") rename("min", "minESRI") exclude("IErrorInfo", "ISupportErrorInfo", "IPersistStream", "IPersist", "tagRECT", "OLE_COLOR", "OLE_HANDLE", "VARTYPE", "XMLSerializer", "IStream", "ISequentialStream", "_LARGE_INTEGER", "_ULARGE_INTEGER", "tagSTATSTG", "_FILETIME")
	#import "C:\Program Files (x86)\ArcGIS\Desktop10.2\com\esrisystemui.olb" named_guids no_namespace raw_interfaces_only no_implementation rename("ICommand", "ICommandESRI")
	#import "C:\Program Files (x86)\ArcGIS\Desktop10.2\com\esriFramework.olb" named_guids no_namespace raw_interfaces_only no_implementation exclude("UINT_PTR", "IPropertyPageSite", "tagMSG", "wireHWND", "_RemotableHandle", "__MIDL_IWinTypes_0009", "IPropertyPage", "tagPROPPAGEINFO", "tagSIZE")
    #import "C:\Program Files (x86)\ArcGIS\Desktop10.2\com\esriGeometry.olb" named_guids no_namespace raw_interfaces_only no_implementation rename("ISegment", "ISegmentESRI")  exclude("IClassFactory")
    #import "C:\Program Files (x86)\ArcGIS\Desktop10.2\com\esriGeoDatabase.olb" raw_interfaces_only, raw_native_types, no_namespace, named_guids rename("IRow", "IRowESRI") rename("GetMessage", "GetMessageESRI") exclude("IPersistStreamInit")
	#import "C:\Program Files (x86)\ArcGIS\Desktop10.2\com\esriNetworkAnalyst.olb" named_guids no_namespace raw_interfaces_only no_implementation
    #import "C:\Program Files (x86)\ArcGIS\Desktop10.2\com\esriDisplay.olb" raw_interfaces_only, raw_native_types, no_namespace, named_guids rename("ResetDC", "ResetDCESRI") rename("DrawText", "DrawTextESRI") rename("CMYK", "CMYKESRI") rename("RGB", "RGBESRI") exclude("tagPOINT", "IConnectionPointContainer", "IEnumConnectionPoints", "IConnectionPoint", "IEnumConnections", "tagCONNECTDATA")
    #import "C:\Program Files (x86)\ArcGIS\Desktop10.2\com\esriCarto.olb" raw_interfaces_only, raw_native_types, no_namespace, named_guids exclude("UINT_PTR") rename("ITableDefinition", "ITableDefinitionESRI") rename("PostMessage", "PostMessageESRI")
#endif

#pragma warning(pop)

// This is included below so we can refer to CLSID_, IID_, etc. defined within this project.
#include "_CustomSolver_i.c"

// other needed libraries
#include "float.h"  // for FLT_MAX, etc.
#include <cmath>    // for HUGE_VAL
#include <algorithm>
#include <ctime>
#include <string>
#include <sstream>
#include <windows.h>
#include <Windowsx.h>
#include <psapi.h>
#include <vector>
#include <deque>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <fstream>
#include <functional>
#include <memory>
#include <iterator>

#pragma warning(push)
#pragma warning(disable : 4521) /* Ignore warning for boost::heap multiple copy constructors  */

// boost includes
#include <boost\heap\fibonacci_heap.hpp>

#pragma warning(pop)

// memory leak detection in DEBUG mode
// ref: http://msdn.microsoft.com/en-us/library/e5ewb1h3%28v=vs.80%29.aspx
#include <stdlib.h>
#include <crtdbg.h>

// ref: http://stackoverflow.com/questions/3202520/c-memory-leak-testing-with-crtdumpmemoryleaks-does-not-output-line-numb
#ifdef _DEBUG
#define DEBUG_NEW_PLACEMENT (_NORMAL_BLOCK, __FILE__, __LINE__)
#else
#define DEBUG_NEW_PLACEMENT
#endif

