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
// Evacuation Solver: DDL entry implementation
// Description: Implementation of DLL Exports.
//
// Copyright (C) 2014 Kaveh Shahabi
// Distributed under the Apache Software License, Version 2.0. (See accompanying file LICENSE.txt)
//
// Author: Kaveh Shahabi
// URL: http://github.com/spatial-computing/CASPER
// ===============================================================================================

#include "stdafx.h"
#include "resource.h"

[importlib("C:\Program Files (x86)\ArcGIS\Desktop10.2\com\esrinetworkanalyst.olb")];

// The module attribute causes DllMain, DllRegisterServer and DllUnregisterServer to be automatically implemented for you
[ module(dll, uuid = "{F25947D1-9C81-48A6-9BFF-CF9EB158FFD7}",
         name = "CustomSolver",
         helpstring = "CustomSolver 1.0 Type Library",
         resource_name = "IDR_CUSTOMSOLVER") ]
class CustomSolverModule
{
public:
	// Override CAtlDllModuleT members

	BOOL WINAPI DllMain(DWORD dwReason, LPVOID lpReserved)
	{
		// Perform actions based on the reason for calling.
		switch(dwReason)
		{
			case DLL_PROCESS_ATTACH:
			 // Initialize once for each new process.
			 // Return FALSE to fail DLL load.
			 // set program start for memory leak detection (DEBUG Mode)
				_ASSERTE(_CrtCheckMemory());
				// turn the next line on to activate heap checking during debug
				// CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_EVERY_1024_DF);
				break;

			case DLL_THREAD_ATTACH:
			 // Do thread-specific initialization.
				break;

			case DLL_THREAD_DETACH:
			 // Do thread-specific cleanup.
				break;

			case DLL_PROCESS_DETACH:
			 // Perform any necessary cleanup.
				_ASSERT_EXPR(_CrtDumpMemoryLeaks() == FALSE, L"EvcSolver memory leak detected");
				_CrtSetDbgFlag(_CRTDBG_CHECK_DEFAULT_DF);
				_ASSERTE(_CrtCheckMemory());
				break;
		}
		return TRUE;  // Successful DLL_PROCESS_ATTACH.
	}
};
