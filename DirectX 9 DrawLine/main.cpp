#include "pch.h"
#include "mem/mem.h"

#if defined(ARCH_X86)
#define ENDSCENE_HOOK_LENGTH 7
#elif defined(ARCH_X64)
#define ENDSCENE_HOOK_LENGTH 15
#endif

typedef long(__stdcall* EndScene_t)(LPDIRECT3DDEVICE9);
EndScene_t oEndScene;
HWND window;
void* vtable[119];

struct iVec2
{
	int x, y;
};

bool GetD3D9Device(void** pTable, size_t Size);
void DrawLine(iVec2 src, iVec2 dst, int thickness, D3DCOLOR color, LPDIRECT3DDEVICE9 pDevice);

long __stdcall hkEndScene(LPDIRECT3DDEVICE9 pDevice)
{
	DrawLine({ 10, 10 }, { 400, 10 }, 2, D3DCOLOR_RGBA(255, 0, 0, 255), pDevice);
	return oEndScene(pDevice);
}

DWORD WINAPI MainThread(LPVOID lpReserved)
{
	if (!GetD3D9Device(vtable, sizeof(vtable))) return false;
	oEndScene = (EndScene_t)Memory::In::Hook::TrampolineHook((byte_t*)vtable[42], (byte_t*)hkEndScene, ENDSCENE_HOOK_LENGTH);
	return TRUE;
}

void DrawLine(iVec2 src, iVec2 dst, int thickness, D3DCOLOR color, LPDIRECT3DDEVICE9 pDevice)
{
	ID3DXLine* dxLine;
	if (D3DXCreateLine(pDevice, &dxLine) != S_OK) return;
	D3DXVECTOR2 Line[2];
	Line[0] = D3DXVECTOR2(src.x, src.y);
	Line[1] = D3DXVECTOR2(dst.x, dst.y);
	dxLine->SetWidth(thickness);
	dxLine->Begin();
	dxLine->Draw(Line, 2, color);
	dxLine->End();
	dxLine->Release();
}

BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam)
{
	DWORD wndProcId;
	GetWindowThreadProcessId(handle, &wndProcId);

	if (GetCurrentProcessId() != wndProcId)
		return TRUE;

	window = handle;
	return FALSE;
}

HWND GetProcessWindow()
{
	window = NULL;
	EnumWindows(EnumWindowsCallback, NULL);
	return window;
}

bool GetD3D9Device(void** pTable, size_t Size)
{
	if (!pTable)
		return false;

	IDirect3D9* pD3D = Direct3DCreate9(D3D_SDK_VERSION);

	if (!pD3D)
		return false;

	IDirect3DDevice9* pDummyDevice = NULL;

	// options to create dummy device
	D3DPRESENT_PARAMETERS d3dpp = {};
	d3dpp.Windowed = false;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = GetProcessWindow();

	HRESULT dummyDeviceCreated = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDummyDevice);

	if (dummyDeviceCreated != S_OK)
	{
		// may fail in windowed fullscreen mode, trying again with windowed mode
		d3dpp.Windowed = !d3dpp.Windowed;

		dummyDeviceCreated = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDummyDevice);

		if (dummyDeviceCreated != S_OK)
		{
			pD3D->Release();
			return false;
		}
	}

	memcpy(pTable, *reinterpret_cast<void***>(pDummyDevice), Size);

	//pDummyDevice->Release();
	pD3D->Release();
	return true;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hModule);
		CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr);
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}