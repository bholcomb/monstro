/*-----------------------------------------------------------------------------
Copyright 2017
Bob Holcomb
-----------------------------------------------------------------------------*/

#pragma once

#ifdef _WIN32
#ifdef UI_EXPORTS
#define UI_DLL __declspec ( dllexport )
#else
#define UI_DLL __declspec ( dllimport )
#endif
#else
#define UI_DLL
#endif