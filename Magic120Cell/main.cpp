#include "stdafx.h"
#include "cubeFrame.h"
#include <tchar.h>

using namespace Magic120Cell;

[STAThreadAttribute]
int APIENTRY _tWinMain(
    HINSTANCE hInstance, 
    HINSTANCE hPrevInstance, 
    LPTSTR lpCmdLine, int nCmdShow)
{
	// Enabling Windows XP visual effects before any controls are created
	Application::EnableVisualStyles();
	Application::SetCompatibleTextRenderingDefault(false); 

	// Create the main window and run it
	(gcnew CubeFrame())->Run();
	return 0;
}
