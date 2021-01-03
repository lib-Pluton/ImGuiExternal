#include "Library.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx9.h"
#include "ImGui/imgui_impl_win32.h"
#include "Overlay.h"

LPCWSTR TargetTitle = L"D3D TEST ENVIROMENT";
LPCSTR OverName = "DEVLOPED BY MAJDEV";
static bool CreateConsole = true;

void InputHandler()
{
	for (int i = 0; i < 5; i++) ImGui::GetIO().MouseDown[i] = false;
	int button = -1;
	if (GetAsyncKeyState(VK_LBUTTON)) button = 0;
	if (button != -1) ImGui::GetIO().MouseDown[button] = true;
}

void Draw() {
	char fpsinfo[64];
	sprintf(fpsinfo, "Overlay FPS: %0.f", ImGui::GetIO().Framerate);
	RGBA White = { 255,255,255,255 };
	DrawStrokeText(30, 44, &White, fpsinfo);
}

void Render() {
	if (GetAsyncKeyState(VK_INSERT) & 1) ShowMenu = !ShowMenu;
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	Draw();
	if (ShowMenu == true) {
		InputHandler();
		ImGui::ShowDemoWindow();
	    ImGui::GetIO().MouseDrawCursor = 1;
	}
	else {
		ImGui::GetIO().MouseDrawCursor = 0;
	}
	ImGui::EndFrame();
	pDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0, 0, 0, 0), 1.0f, 0);
	if (pDevice->BeginScene() >= 0) {
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		pDevice->EndScene();
	}
	HRESULT result = pDevice->Present(NULL, NULL, NULL, NULL);
	if (result == D3DERR_DEVICELOST && pDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET) {
		ImGui_ImplDX9_InvalidateDeviceObjects();
		pDevice->Reset(&pParams);
		ImGui_ImplDX9_CreateDeviceObjects();
	}
}

WPARAM MainLoop() {
	static RECT old_rc;
	ZeroMemory(&Message, sizeof(MSG));
	while (Message.message != WM_QUIT){
		if (PeekMessage(&Message, Wnd, 0, 0, PM_REMOVE)) {
			TranslateMessage(&Message);
			DispatchMessage(&Message);
		}
		HWND hwnd_active = GetForegroundWindow();
		if (hwnd_active == GameWnd) {
			HWND hwndtest = GetWindow(hwnd_active, GW_HWNDPREV);
			SetWindowPos(Wnd, hwndtest, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		}
		RECT rc;
		POINT xy;
		ZeroMemory(&rc, sizeof(RECT));
		ZeroMemory(&xy, sizeof(POINT));
		GetClientRect(GameWnd, &rc);
		ClientToScreen(GameWnd, &xy);
		rc.left = xy.x;
		rc.top = xy.y;
		ImGuiIO& io = ImGui::GetIO();
		io.ImeWindowHandle = GameWnd;
		io.DeltaTime = 1.0f / 60.0f;
		POINT p;
		GetCursorPos(&p);
		io.MousePos.x = p.x - xy.x;
		io.MousePos.y = p.y - xy.y;
		if (GetAsyncKeyState(0x1)) {
			io.MouseDown[0] = true;
			io.MouseClicked[0] = true;
			io.MouseClickedPos[0].x = io.MousePos.x;
			io.MouseClickedPos[0].x = io.MousePos.y;
		}
		else {
			io.MouseDown[0] = false;
		}	
		if (rc.left != old_rc.left || rc.right != old_rc.right || rc.top != old_rc.top || rc.bottom != old_rc.bottom){
			old_rc = rc;
			Width = rc.right;
			Height = rc.bottom;
			pParams.BackBufferWidth = Width;
			pParams.BackBufferHeight = Height;
			SetWindowPos(Wnd, (HWND)0, xy.x, xy.y, Width, Height, SWP_NOREDRAW);
			pDevice->Reset(&pParams);
		}
		Render();
	}
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	ClearD3D();
	DestroyWindow(Wnd);
	return Message.wParam;
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
		return true;
	}
	switch (msg)
	{
	case WM_DESTROY:
		ClearD3D();
		PostQuitMessage(0);
		exit(4);
		break;
	case WM_SIZE:
		if (pDevice != NULL && wParam != SIZE_MINIMIZED) {
			ImGui_ImplDX9_InvalidateDeviceObjects();
			pParams.BackBufferWidth = LOWORD(lParam);
			pParams.BackBufferHeight = HIWORD(lParam);
			HRESULT hr = pDevice->Reset(&pParams);
			if (hr == D3DERR_INVALIDCALL)
				IM_ASSERT(0);
			ImGui_ImplDX9_CreateDeviceObjects();
		}
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
		break;
	}
	return 0;
}

void SetupWindow() {
	WNDCLASSEX wClass{
		sizeof(WNDCLASSEX), 0, WndProc, 0, 0, nullptr, LoadIcon(nullptr, IDI_APPLICATION), LoadCursor(nullptr, IDC_ARROW), nullptr, nullptr, OverName, LoadIcon(nullptr, IDI_APPLICATION)
	};
	if (!RegisterClassEx(&wClass)) {
		exit(1);
	}
	GameWnd = FindWindowW(NULL, TargetTitle);
	if (GameWnd) {
		GetClientRect(GameWnd, &GameRect);
		POINT xy;
		ClientToScreen(GameWnd, &xy);
		GameRect.left = xy.x;
		GameRect.top = xy.y;
		Width = GameRect.right;
		Height = GameRect.bottom;
	}
	else {
		exit(2);
	}
	Wnd = CreateWindowExA(NULL, OverName, OverName, WS_POPUP | WS_VISIBLE, GameRect.left, GameRect.top, Width, Height, NULL, NULL, 0, NULL);
	DwmExtendFrameIntoClientArea(Wnd, &Margin);
	SetWindowLong(Wnd, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW);
	ShowWindow(Wnd, SW_SHOW);
	UpdateWindow(Wnd);
}

HRESULT DirectXInit(HWND hWnd) {
	if (FAILED(Direct3DCreate9Ex(D3D_SDK_VERSION, &pObject))) {
		exit(3);
	}
	D3DPRESENT_PARAMETERS pParams = { 0 };
	pParams.Windowed = TRUE;
	pParams.SwapEffect = D3DSWAPEFFECT_DISCARD;
	pParams.hDeviceWindow = hWnd;
	pParams.MultiSampleQuality = D3DMULTISAMPLE_NONE;
	pParams.BackBufferFormat = D3DFMT_A8R8G8B8;
	pParams.BackBufferWidth = Width;
	pParams.BackBufferHeight = Height;
	pParams.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	pParams.EnableAutoDepthStencil = TRUE;
	pParams.AutoDepthStencilFormat = D3DFMT_D16;
	pParams.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	if (FAILED(pObject->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &pParams, 0, &pDevice))) {
		pObject->Release();
		exit(4);
	}
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantTextInput || ImGui::GetIO().WantCaptureKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX9_Init(pDevice);
	pObject->Release();
	return S_OK;
}

int main() {
	if (CreateConsole == false) {ShowWindow(::GetConsoleWindow(), SW_HIDE);}else {ShowWindow(::GetConsoleWindow(), SW_SHOW);}
	SetupWindow();
	DirectXInit(Wnd);
	while (TRUE) {
		MainLoop();
	}
	return 0;
}
