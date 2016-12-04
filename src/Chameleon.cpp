/*

	chameleon-ng - skin & knife changer for Counter-Strike: Global Offensive.
	Copyright (C) 2014 - 2016, TEAM SKYENET. (www.skyenet.org)

	Contributors:
		* Emma N. Skye <emma@skyenet.org>
		* aixxe <aixxe@skyenet.org>
	
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with chameleon-ng. If not, see <http://www.gnu.org/licenses/>.

*/

#include "Chameleon.hpp"

IBaseClientDLL* clientdll = nullptr;
IClientEntityList* entitylist = nullptr;
IVEngineClient* engine = nullptr;
IVModelInfoClient* modelinfo = nullptr;
IGameEventManager2* gameevents = nullptr;

std::unique_ptr<VMTHook> clientdll_hook;
std::unique_ptr<VMTHook> gameevents_hook;
std::unique_ptr<VMTHook> d3d9_hook;

NetVars netvars;
Renderer renderer;

void WINAPI Chameleon_Init(LPVOID dll_instance) {
	// Get important class pointers from game DLLs using interface versions.
	clientdll = CaptureInterface<IBaseClientDLL>("client.dll", CLIENT_DLL_INTERFACE_VERSION);
	entitylist = CaptureInterface<IClientEntityList>("client.dll", VCLIENTENTITYLIST_INTERFACE_VERSION);
	engine = CaptureInterface<IVEngineClient>("engine.dll", VENGINE_CLIENT_INTERFACE_VERSION);
	modelinfo = CaptureInterface<IVModelInfoClient>("engine.dll", VMODELINFO_CLIENT_INTERFACE_VERSION);
	gameevents = CaptureInterface<IGameEventManager2>("engine.dll", INTERFACEVERSION_GAMEEVENTSMANAGER2);

	// Hook 'FrameStageNotify' from IBaseClientDLL.
	clientdll_hook = std::make_unique<VMTHook>(clientdll);
	clientdll_hook->HookFunction(hkFrameStageNotify, 36);

	// Hook 'FireEventClientSide' from IGameEventManager2.
	gameevents_hook = std::make_unique<VMTHook>(gameevents);
	gameevents_hook->HookFunction(hkFireEventClientSide, 9);

	// Scan for the IDirect3DDevice9 pointer in 'shaderapidx9.dll'.
	IDirect3DDevice9* d3d9_device = **reinterpret_cast<IDirect3DDevice9***>(
		FindPattern("shaderapidx9.dll", PBYTE("\xA1\x00\x00\x00\x00\x50\x8B\x08\xFF\x51\x0C"), "x????xxxxxx") + 1
	);

	// Initialize the renderer.
	renderer.Initialize(FindWindowA("Valve001", NULL), d3d9_device);

	// Hook 'EndScene' and 'Reset' for our custom rendering.
	d3d9_hook = std::make_unique<VMTHook>(d3d9_device);
	d3d9_hook->HookFunction(hkReset, 16);
	d3d9_hook->HookFunction(hkEndScene, 42);
}

bool __stdcall DllMain(HINSTANCE dll_instance, DWORD call_reason, LPVOID reserved) {
	if (call_reason == DLL_PROCESS_ATTACH)
		CreateThread(0, 0, LPTHREAD_START_ROUTINE(Chameleon_Init), dll_instance, 0, 0);

	return true;
}