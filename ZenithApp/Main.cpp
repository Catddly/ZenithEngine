#include "ZenithEngine.h"

#include <windows.h>

//int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
int main()
{
	ZE::RunEngineScoped scopedEngine;
	scopedEngine.Run();
}
