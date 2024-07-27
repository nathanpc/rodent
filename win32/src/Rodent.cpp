/**
 * Application.cpp
 * The Windows port of the Rodent project.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "Rodent.h"

// Common definitions.
#define MAX_LOADSTRING 100

// Global variables.
HINSTANCE hInst;
HWND hwndMain;
TCHAR szWindowClass[MAX_LOADSTRING];
TCHAR szAppTitle[MAX_LOADSTRING];

/**
 * Application's main entry point.
 *
 * @param hInstance     Program instance.
 * @param hPrevInstance Ignored: Leftover from Win16.
 * @param lpCmdLine     String with command line text.
 * @param nShowCmd      Initial state of the program's main window.
 *
 * @return wParam of the WM_QUIT message.
 */
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
					  LPTSTR lpCmdLine, int nCmdShow) {
	MSG msg;
	HACCEL hAccel;
	int rc;
	
	// Ensure we specify parameters not in use.
	UNREFERENCED_PARAMETER(hPrevInstance);

	// Load the application class and title.
	LoadString(hInstance, IDC_RODENT, szWindowClass, MAX_LOADSTRING);
	LoadString(hInstance, IDS_APP_TITLE, szAppTitle, MAX_LOADSTRING);

	// Initialize the application.
	rc = RegisterApplication(hInstance);
	if (rc == 0) {
		MsgBoxError(NULL, _T("Error Registering Class"),
			_T("An error occurred while trying to register the application's ")
			_T("window class."));
		return 0;
	}

	// Initialize this single instance.
	hwndMain = InitializeInstance(hInstance, lpCmdLine, nCmdShow);
	if (hwndMain == 0) {
		MsgBoxError(NULL, _T("Error Initializing Instance"),
			_T("An error occurred while trying to initialize the ")
			_T("application's instance."));
		return 0x10;
	}

	// Load accelerators.
	hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_RODENT));

	// Application message loop.
	while (GetMessage(&msg, NULL, 0, 0)) {
		// Translate accelerators.
		if (!TranslateAccelerator(hwndMain, hAccel, &msg)) {
			// Translate message.
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	// Clean up.
	return TerminateInstance(hInstance, (int)msg.wParam);
}

/**
 * Initializes the application and registers the application class.
 *
 * @param hInstance Application instance.
 *
 * @return TRUE if the class was registered.
 */
ATOM RegisterApplication(HINSTANCE hInstance) {
	WNDCLASSEX wcex;

	// Setup the application's main window class.
	wcex.cbSize = sizeof(WNDCLASSEX); 
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)MainWindowProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_RODENT);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= GetSysColorBrush(COLOR_WINDOW);
	wcex.lpszMenuName	= (LPCTSTR)IDC_RODENT;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	// Register the application's main window class.
	return RegisterClassEx(&wcex);
}

/**
 * Initializes the instance and creates the window.
 *
 * @param hInstance Program instance.
 * @param lpCmdLine String with command line text.
 * @param nShowCmd  Initial state of the program's main window.
 *
 * @return Window handler.
 */
HWND InitializeInstance(HINSTANCE hInstance, LPTSTR lpCmdLine, int nCmdShow) {
	HWND hWnd;
	int iRet;

	// Set the global instance variable.
	hInst = hInstance;

	// Initialize Winsock stuff.
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(2, 2);
	if ((iRet = WSAStartup(wVersionRequested, &wsaData)) != 0) {
		tcout << _T("WSAStartup failed with error ") << iRet << _T('\r') <<
			std::endl;
		MsgBoxError(NULL, _T("Error Initializing WinSock2"),
			_T("WSAStartup failed with an error."));
		return NULL;
	}

	// Create the main window.
	hWnd = CreateWindow(szWindowClass,			// Window class.
						szAppTitle,				// Window title.
						WS_OVERLAPPEDWINDOW,	// Style flags.
						CW_USEDEFAULT,			// X position.
						CW_USEDEFAULT,			// Y position.
						600,					// Initial width,
						400,					// Initial height.
						NULL,					// Parent window.
						NULL,					// Menu class. (Always NULL)
						hInstance,				// Application instance.
						NULL);					// Pointer to create parameters.

	// Check if the window creation worked.
	if (!IsWindow(hWnd)) {
		MsgBoxError(NULL, _T("Error Initializing Instance"),
			_T("Window creation failed."));
		return NULL;
	}

#ifdef UNDERCE
	// Set the window task switching icon.
	HANDLE hIcon = LoadImage(hInst, MAKEINTRESOURCE(IDI_DESKTOP), IMAGE_ICON,
		GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0);
	SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

	// Set window taskbar icon.
	hIcon = LoadImage(hInst, MAKEINTRESOURCE(IDI_SMALL), IMAGE_ICON,
		GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
	SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
#endif

	// Show and update the window.
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return hWnd;
}

/**
 * Terminates the application instance.
 *
 * @param hInstance Application instance.
 * @param nDefRC    Return code.
 *
 * @return Previous return code.
 */
int TerminateInstance(HINSTANCE hInstance, int nDefRC) {
	// Clean up the Winsock stuff.
	WSACleanup();

	return nDefRC;
}

/**
 * Main window procedure.
 *
 * @param hWnd   Window handler.
 * @param wMsg   Message type.
 * @param wParam Message parameter.
 * @param lParam Message parameter.
 *
 * @return 0 if everything worked.
 */
LRESULT CALLBACK MainWindowProc(HWND hWnd, UINT wMsg, WPARAM wParam,
								LPARAM lParam) {
	switch (wMsg) {
		case WM_CREATE:
			return WndMainCreate(hWnd, wMsg, wParam, lParam);
		case WM_COMMAND:
			return WndMainCommand(hWnd, wMsg, wParam, lParam);
		case WM_NOTIFY:
			return WndMainNotify(hWnd, wMsg, wParam, lParam);
		case WM_SIZE:
			return WndMainSize(hWnd, wMsg, wParam, lParam);
		case WM_CLOSE:
			return WndMainClose(hWnd, wMsg, wParam, lParam);
		case WM_DESTROY:
			return WndMainDestroy(hWnd, wMsg, wParam, lParam);
	}

	return DefWindowProc(hWnd, wMsg, wParam, lParam);
}

/**
 * Process the WM_CREATE message for the window.
 *
 * @param hWnd   Window handler.
 * @param wMsg   Message type.
 * @param wParam Message parameter.
 * @param lParam Message parameter.
 *
 * @return 0 if everything worked.
 */
LRESULT WndMainCreate(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam) {
	// Ensure that the common controls DLL is loaded and initialized. 
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC  = ICC_WIN95_CLASSES | ICC_COOL_CLASSES | ICC_USEREX_CLASSES;
	InitCommonControlsEx(&icex);

	// TODO: Initialize Gopher stuff here.

	return 0;
}

/**
 * Process the WM_COMMAND message for the window.
 *
 * @param hWnd   Window handler.
 * @param wMsg   Message type.
 * @param wParam Message parameter.
 * @param lParam Message parameter.
 *
 * @return 0 if everything worked.
 */
LRESULT WndMainCommand(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam) {
	int wmId = LOWORD(wParam);
	int wmEvent = HIWORD(wParam);

	switch (wmId) {
		case IDM_ABOUT:
			AboutDialog(hInst, hWnd).ShowModal();
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
	}

	return DefWindowProc(hWnd, wMsg, wParam, lParam);
}

/**
 * Process the WM_NOTIFY message for the window.
 *
 * @param hWnd   Window handler.
 * @param wMsg   Message type.
 * @param wParam Message parameter.
 * @param lParam Message parameter.
 *
 * @return 0 if everything worked.
 */
LRESULT WndMainNotify(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam) {
	return DefWindowProc(hWnd, wMsg, wParam, lParam);
}

/**
 * Process the WM_SIZE message for the window.
 *
 * @param hWnd   Window handler.
 * @param wMsg   Message type.
 * @param wParam Specifies the type of resizing requested.
 * @param lParam New width (LOWORD) and height (HIWORD) of the client area.
 *
 * @return 0 if everything worked.
 */
LRESULT WndMainSize(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam) {
	// We have been minimized, so nothing should be done.
	if (wParam == SIZE_MINIMIZED)
		return DefWindowProc(hWnd, wMsg, wParam, lParam);

	return DefWindowProc(hWnd, wMsg, wParam, lParam);
}

/**
 * Process the WM_CLOSE message for the window.
 *
 * @param hWnd   Window handler.
 * @param wMsg   Message type.
 * @param wParam Message parameter.
 * @param lParam Message parameter.
 *
 * @return 0 if everything worked.
 */
LRESULT WndMainClose(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam) {
	// Send window destruction message.
	DestroyWindow(hWnd);

	// Call any destructors that might be needed.
	// TODO: Call destructors.

	return DefWindowProc(hWnd, wMsg, wParam, lParam);
}

/**
 * Process the WM_DESTROY message for the window.
 *
 * @param hWnd   Window handler.
 * @param wMsg   Message type.
 * @param wParam Message parameter.
 * @param lParam Message parameter.
 *
 * @return 0 if everything worked.
 */
LRESULT WndMainDestroy(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam) {
	// Post quit message and return.
	PostQuitMessage(0);
	return 0;
}

/**
 * Creates and populates the browser Toolbar with controls.
 *
 * @param hwndParent Parent window to attach the Toolbar to.
 * @param lpSize     Optional. Size of the Toolbar with all of its controls.
 *
 * @return Window handle for the Toolbar control.
 */
HWND CreateToolbar(HWND hwndParent, LPSIZE lpSize) {
	// Declare and initialize local constants.
	const DWORD buttonStyles = BTNS_AUTOSIZE;

	// Create the Toolbar.
	HWND hwndToolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL, 
		WS_CHILD | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | TBSTYLE_LIST |
		TBSTYLE_TRANSPARENT | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | CCS_NODIVIDER |
		CCS_NORESIZE | WS_VISIBLE, 0, 0, 0, 0, hwndParent, NULL, hInst, NULL);
	if (hwndToolbar == NULL) {
		MsgBoxError(NULL, _T("Error creating Toolbar"),
			_T("An error occurred while trying to run CreateWindowEx for the ")
			_T("Toolbar."));
		return NULL;
	}
	SendMessage(hwndToolbar, TB_SETEXTENDEDSTYLE, (WPARAM)NULL,
		(LPARAM)TBSTYLE_EX_MIXEDBUTTONS);

	// Create the Toolbar image list and assign it to the toolbar.
	const int imlID = 0;
	const int iImages = 6;
	const int bitmapSize = 16;
	himlToolbar = ImageList_Create(bitmapSize, bitmapSize,
		ILC_MASK | ILC_COLOR32, iImages, 0);
	SendMessage(hwndToolbar, TB_SETIMAGELIST, (WPARAM)imlID,
		(LPARAM)himlToolbar);

	// Load Toolbar button images into image list.
	HICON hIcon;
	hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_LEFT));
	ImageList_AddIcon(himlToolbar, hIcon);
	hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_RIGHT));
	ImageList_AddIcon(himlToolbar, hIcon);
	hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_UP));
	ImageList_AddIcon(himlToolbar, hIcon);
	hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_REFRESH));
	ImageList_AddIcon(himlToolbar, hIcon);
	hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_STOP));
	ImageList_AddIcon(himlToolbar, hIcon);
	hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_PLAY));
	ImageList_AddIcon(himlToolbar, hIcon);

	// Setup Toolbar buttons.
	const int numButtons = 4;
	int iButtonLabel = SendMessage(hwndToolbar, TB_ADDSTRING, (WPARAM)hInst,
		(LPARAM)IDS_TBNAVLABELS);
	TBBUTTON tbButtons[numButtons] = {
		{ 0, IDM_BACK, TBSTATE_ENABLED, buttonStyles, {0}, NULL, iButtonLabel++ },
		{ 1, IDM_NEXT, TBSTATE_ENABLED, buttonStyles, {0}, NULL, iButtonLabel++ },
		{ 2, IDM_PARENT, TBSTATE_ENABLED, buttonStyles, {0}, NULL, iButtonLabel++ },
		{ 3, IDM_REFRESH, TBSTATE_ENABLED, buttonStyles, {0}, NULL, iButtonLabel++ }
		//{ 5, IDM_GO, TBSTATE_ENABLED, buttonStyles, {0}, NULL, iButtonLabel + 2 }
	};

	// Add buttons.
	SendMessage(hwndToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
	SendMessage(hwndToolbar, TB_ADDBUTTONS, (WPARAM)numButtons,
		(LPARAM)&tbButtons);

	// Resize the Toolbar, and then show it.
	//SendMessage(hwndToolbar, TB_AUTOSIZE, 0, 0); 
	ShowWindow(hwndToolbar,  TRUE);
	
	// Get the final size of the Toolbar if requested.
	if (lpSize != NULL)
		SendMessage(hwndToolbar, TB_GETMAXSIZE, 0, (LPARAM)lpSize);

	return hwndToolbar;
}

/**
 * Creates the browser address editor window.
 *
 * @param hwndParent Parent window to attach the Rebar to.
 *
 * @return Window handle for the address window control.
 */
HWND CreateAddressWindow(HWND hwndParent) {
	// Create the ComboBoxEx control.
	HWND hwndComboBox = CreateWindowEx(0, WC_COMBOBOXEX, NULL,
		WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_CLIPCHILDREN |
		WS_CLIPSIBLINGS | CCS_NORESIZE | CBS_AUTOHSCROLL | CBS_DROPDOWN,
		0, 0, 0, 0, hwndParent, NULL, hInst, NULL);
	if (hwndComboBox == NULL) {
		MsgBoxError(NULL, _T("Error creating ComboBoxEx"),
			_T("An error occurred while trying to run CreateWindowEx for the ")
			_T("address bar ComboBoxEx."));
		return NULL;
	}

	return hwndComboBox;
}

/**
 * Creates and populates the browser Rebar with controls.
 *
 * @param hwndParent Parent window to attach the Rebar to.
 *
 * @return Window handle for the Rebar control.
 */
HWND CreateRebar(HWND hwndParent) {
	// Create Rebar control.
	HWND hwndRebar = CreateWindowEx(WS_EX_TOOLWINDOW, REBARCLASSNAME, NULL,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		RBS_VARHEIGHT | CCS_NODIVIDER | RBS_BANDBORDERS, 0, 0, 0, 0,
		hwndParent, NULL, hInst, NULL);
	if (hwndRebar == NULL) {
		MsgBoxError(NULL, _T("Error creating Rebar"),
			_T("An error occurred while trying to run CreateWindowEx for the ")
			_T("Rebar."));
		return NULL;
	}

	// Initialize common band information.
	REBARBANDINFO rbBand = { sizeof(REBARBANDINFO) };
	rbBand.fMask = RBBIM_STYLE | RBBIM_TEXT | RBBIM_CHILD | RBBIM_CHILDSIZE |
		RBBIM_SIZE;
	rbBand.fStyle = RBBS_CHILDEDGE | RBBS_GRIPPERALWAYS;

	// Create main Toolbar and get its size.
	SIZE sizeToolbar;
	HWND hwndToolbar = CreateToolbar(hwndParent, &sizeToolbar);
	if (hwndToolbar == NULL)
		return NULL;

	// Setup and add the Toolbar to the Rebar control.
	rbBand.lpText = _T("");
	rbBand.hwndChild = hwndToolbar;
	rbBand.cyChild = sizeToolbar.cy;
	rbBand.cxMinChild = sizeToolbar.cx;
	rbBand.cyMinChild = sizeToolbar.cy;
	rbBand.cx = 0;
	SendMessage(hwndRebar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);

	// Create address bar.
	HWND hwndAddress = CreateAddressWindow(hwndParent);
	if (hwndAddress == NULL)
		return NULL;

	// Setup and add the address bar to the Rebar control.
	rbBand.lpText = _T("Address ");
	rbBand.hwndChild = hwndAddress;
	rbBand.cxMinChild = 350;
	rbBand.cx = 0;
	SendMessage(hwndRebar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);

	return hwndRebar;
}
