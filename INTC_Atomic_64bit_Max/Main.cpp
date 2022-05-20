//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#include "stdafx.h"
#include "INTC_Atomics_64bit_Max.h"

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    AllocConsole();

    FILE* fConsole;
    freopen_s(&fConsole, "CONIN$", "r", stdin);
    freopen_s(&fConsole, "CONOUT$", "w", stderr);
    freopen_s(&fConsole, "CONOUT$", "w", stdout);

    INTC_Atomics_64bit_Max sample(1280, 720, L"Intel Atomics 64bit Max Extension Sample");
    return Win32Application::Run(&sample, hInstance, nCmdShow);
}

