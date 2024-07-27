/**
 * Application.cpp
 * The Windows port of the Rodent project.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "Rodent.h"
#ifdef DEBUG
#include <crtdbg.h>
#endif // DEBUG

// Common definitions.
#define MAX_LOADSTRING 100

// Global variables.
MainWindow *wndMain = NULL;
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

#ifdef DEBUG
	// Initialize memory leak detection.
	_CrtMemState snapBegin;
	_CrtMemState snapEnd;
	_CrtMemState snapDiff;
	_CrtMemCheckpoint(&snapBegin);
#endif // DEBUG

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
	HWND hwndMain = InitializeInstance(hInstance, lpCmdLine, nCmdShow);
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

	// Terminate instance.
	rc = TerminateInstance(hInstance, (int)msg.wParam);

#ifdef DEBUG
	// Detect memory leaks.
	_CrtMemCheckpoint(&snapEnd);
	if (_CrtMemDifference(&snapDiff, &snapBegin, &snapEnd)) {
		OutputDebugString(_T("MEMORY LEAKS DETECTED\r\n"));
		OutputDebugString(L"----------- _CrtMemDumpStatistics ---------\r\n");
		_CrtMemDumpStatistics(&snapDiff);
		OutputDebugString(L"----------- _CrtMemDumpAllObjectsSince ---------\r\n");
		_CrtMemDumpAllObjectsSince(&snapBegin);
		OutputDebugString(L"----------- _CrtDumpMemoryLeaks ---------\r\n");
		_CrtDumpMemoryLeaks();
	} else {
		OutputDebugString(_T("No memory leaks detected. Congratulations!\r\n"));
	}
#endif // DEBUG

	return rc;
}

/**
 * Constructs the main window object.
 *
 * @param hInstance Application instance.
 */
MainWindow::MainWindow(HINSTANCE hInstance) {
	// Initialize important stuff.
	this->hInst = hInstance;

	// Initialize default values.
	this->hWnd = NULL;
	himlToolbar = NULL;
	hwndToolbar = NULL;
	hwndAddressCombo = NULL;
	hwndAddressBar = NULL;
	hwndRebar = NULL;
	hwndDirectory = NULL;
	hwndStatusBar = NULL;
}

/**
 * Cleans up everything that was allocated by the main window.
 */
MainWindow::~MainWindow() {
	// Destroy everything related to the Rebar.
	if (hwndRebar) {
		DestroyWindow(hwndRebar);
		hwndRebar = NULL;
	}
	if (hwndAddressCombo) {
		DestroyWindow(hwndAddressCombo);
		hwndAddressCombo = NULL;
	}
	if (hwndAddressBar) {
		DestroyWindow(hwndAddressBar);
		hwndAddressBar = NULL;
	}
	if (himlToolbar) {
		ImageList_Destroy(himlToolbar);
		himlToolbar = NULL;
	}
	if (hwndToolbar) {
		DestroyWindow(hwndToolbar);
		hwndToolbar = NULL;
	}
	if (hwndDirectory) {
		DestroyWindow(hwndDirectory);
		hwndDirectory = NULL;
	}
	if (hwndStatusBar) {
		DestroyWindow(hwndStatusBar);
		hwndStatusBar = NULL;
	}

	// Destroy the main window.
	DestroyWindow(this->hWnd);
	this->hWnd = NULL;
}

/**
 * Sets up the layout of the window's controls.
 *
 * @param hWnd Main window handle.
 *
 * @return TRUE if the operation was successful.
 */
BOOL MainWindow::SetupControls(HWND hWnd) {
	// Ensure we have a copy of the window handle.
	this->hWnd = hWnd;

	// Add controls to the window.
	if (CreateRebar() == NULL)
		return FALSE;
	if (CreateStatusBar() == NULL)
		return FALSE;
	if (CreateDirectoryView() == NULL)
		return FALSE;

	return TRUE;
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

	// Initialize main window object.
	wndMain = new MainWindow(hInstance);

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

	// Setup the window.
	wndMain->SetupControls(hWnd);

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
			AboutDialog(wndMain->hInst, hWnd).ShowModal();
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

	// Resize child windows.
	wndMain->ResizeWindows(hWnd);

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
	delete wndMain;

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
 * Resizes all controls based on the parent's size.
 *
 * @param hwndParent Parent window of all controls.
 *
 * @return TRUE if the function succeeds.
 */
BOOL MainWindow::ResizeWindows(HWND hwndParent) {
	// Get the client area of the parent window.
	RECT rcParent;
	GetClientRect(hwndParent, &rcParent);

	// Reset the Rebar size.
	SetWindowPos(this->hwndRebar, NULL, 0, 0, rcParent.right - rcParent.left,
		20, SWP_NOZORDER | SWP_NOMOVE);
	SendMessage(this->hwndRebar, RB_MAXIMIZEBAND, (WPARAM)1, (LPARAM)0);

	// Resize the controls inside the address Toolbar.
	SIZE sizeToolbar;
	RECT rcAddressBar;
	GetClientRect(this->hwndAddressBar, &rcAddressBar);
	SendMessage(hwndAddressBar, TB_GETMAXSIZE, 0, (LPARAM)&sizeToolbar);
	SetWindowPos(this->hwndAddressCombo, 0, 0, 0,
		rcAddressBar.right - sizeToolbar.cx - 10, 0, SWP_NOZORDER | SWP_NOMOVE);
	SendMessage(this->hwndAddressBar, TB_SETINDENT,
		(WPARAM)(rcAddressBar.right - sizeToolbar.cx - 5), (LPARAM)0);

	// Resize Status bar and directory ListView.
	ResizeStatusBar(&rcParent);

	// Resize directory ListView.
	RECT rcRebar;
	RECT rcStatusBar;
	GetClientRect(this->hwndRebar, &rcRebar);
	GetClientRect(this->hwndStatusBar, &rcStatusBar);
	SetWindowPos(this->hwndDirectory, NULL, 0, rcRebar.bottom, rcParent.right,
		rcParent.bottom - rcStatusBar.bottom - rcRebar.bottom, SWP_NOZORDER);

	return TRUE;
}

/**
 * Creates and populates the browser Toolbar with controls.
 *
 * @param lpSize Optional. Size of the Toolbar with all of its controls.
 *
 * @return Window handle for the Toolbar control.
 */
HWND MainWindow::CreateToolbar(LPSIZE lpSize) {
	// Create the Toolbar.
	const DWORD buttonStyles = BTNS_AUTOSIZE;
	hwndToolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL, 
		WS_CHILD | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | TBSTYLE_LIST |
		TBSTYLE_TRANSPARENT | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | CCS_NODIVIDER |
		CCS_NORESIZE | WS_VISIBLE, 0, 0, 0, 0, this->hWnd, (HMENU)IDC_TBMAIN,
		hInst, NULL);
	if (hwndToolbar == NULL) {
		MsgBoxError(NULL, _T("Error creating Toolbar"),
			_T("An error occurred while trying to run CreateWindowEx for the ")
			_T("main Toolbar."));
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
	SendMessage(hwndToolbar, TB_AUTOSIZE, 0, 0); 
	ShowWindow(hwndToolbar,  TRUE);
	
	// Get the final size of the Toolbar if requested.
	if (lpSize != NULL)
		SendMessage(hwndToolbar, TB_GETMAXSIZE, 0, (LPARAM)lpSize);

	return hwndToolbar;
}

/**
 * Creates the browser address Toolbar.
 *
 * @param lpSize Optional. Size of the Toolbar with all of its controls.
 *
 * @return Window handle for the address Toolbar control.
 */
HWND MainWindow::CreateAddressBar(LPSIZE lpSize) {
	// Create address Toolbar.
	const DWORD buttonStyles = BTNS_AUTOSIZE;
	hwndAddressBar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL, 
		WS_CHILD | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | TBSTYLE_LIST |
		TBSTYLE_TRANSPARENT | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
		CCS_NODIVIDER | CCS_NORESIZE | WS_VISIBLE, 0, 0, 0, 0, this->hWnd,
		(HMENU)IDC_TBADDRESS, hInst, NULL);
	if (hwndAddressBar == NULL) {
		MsgBoxError(NULL, _T("Error creating Toolbar"),
			_T("An error occurred while trying to run CreateWindowEx for the ")
			_T("address Toolbar."));
		return NULL;
	}
	SendMessage(hwndAddressBar, TB_SETEXTENDEDSTYLE, (WPARAM)NULL,
		(LPARAM)TBSTYLE_EX_MIXEDBUTTONS);
	SendMessage(hwndAddressBar, TB_SETIMAGELIST, (WPARAM)0,
		(LPARAM)himlToolbar);

	// Create the ComboBoxEx control.
	hwndAddressCombo = CreateWindowEx(0, WC_COMBOBOXEX, NULL,
		WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_CLIPCHILDREN |
		WS_CLIPSIBLINGS | CCS_NORESIZE | CBS_AUTOHSCROLL | CBS_DROPDOWN,
		0, 0, 250, 0, this->hWnd, (HMENU)IDC_CMBADDRESS, hInst, NULL);
	if (hwndAddressCombo == NULL) {
		MsgBoxError(NULL, _T("Error creating ComboBoxEx"),
			_T("An error occurred while trying to run CreateWindowEx for the ")
			_T("address bar ComboBoxEx."));
		return NULL;
	}

	// Append the address ComboBoxEx to the Toolbar.
	SetParent(hwndAddressCombo, hwndAddressBar);

	// Setup Toolbar buttons.
	const int numButtons = 1;
	int iButtonLabel = SendMessage(hwndAddressBar, TB_ADDSTRING, (WPARAM)hInst,
		(LPARAM)IDS_TBNAVLABELS);
	TBBUTTON tbButtons[numButtons] = {
		{ 5, IDM_GO, TBSTATE_ENABLED, buttonStyles, {0}, NULL, 0 }
	};

	// Add buttons.
	SendMessage(hwndAddressBar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON),
		(LPARAM)0);
	SendMessage(hwndAddressBar, TB_ADDBUTTONS, (WPARAM)numButtons,
		(LPARAM)&tbButtons);

	// Resize the Toolbar, and then show it.
	SendMessage(hwndAddressBar, TB_AUTOSIZE, 0, 0); 
	ShowWindow(hwndAddressBar,  TRUE);
	
	// Get the final size of the Toolbar if requested.
	if (lpSize != NULL)
		SendMessage(hwndAddressBar, TB_GETMAXSIZE, 0, (LPARAM)lpSize);

	return hwndAddressBar;
}

/**
 * Creates and populates the browser Rebar with controls.
 *
 * @return Window handle for the Rebar control.
 */
HWND MainWindow::CreateRebar() {
	// Create Rebar control.
	hwndRebar = CreateWindowEx(WS_EX_TOOLWINDOW, REBARCLASSNAME, NULL,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
		RBS_VARHEIGHT | CCS_NODIVIDER | RBS_BANDBORDERS | RBS_AUTOSIZE,
		0, 0, 0, 0, this->hWnd, (HMENU)IDC_RBMAIN, hInst, NULL);
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
	rbBand.fStyle = RBBS_CHILDEDGE;

	// Create main Toolbar and get its size.
	SIZE sizeToolbar;
	if (CreateToolbar(&sizeToolbar) == NULL)
		return NULL;

	// Setup and add the Toolbar to the Rebar control.
	rbBand.lpText = _T("");
	rbBand.hwndChild = this->hwndToolbar;
	rbBand.cyChild = sizeToolbar.cy;
	rbBand.cxMinChild = sizeToolbar.cx;
	rbBand.cyMinChild = sizeToolbar.cy;
	rbBand.cx = 0;
	SendMessage(hwndRebar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);

	// Create address bar.
	if (CreateAddressBar(NULL) == NULL)
		return NULL;

	// Setup and add the address bar to the Rebar control.
	rbBand.lpText = _T("Address ");
	rbBand.hwndChild = this->hwndAddressBar;
	rbBand.cxMinChild = 250;
	rbBand.cx = 0;
	SendMessage(hwndRebar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rbBand);

	return hwndRebar;
}

/**
 * Creates the window's status bar.
 *
 * @return Status bar window handle.
 */
HWND MainWindow::CreateStatusBar() {
	// Get parent client rectangle.
	RECT rc;
	GetClientRect(this->hWnd, &rc);

	// Create the status bar window.
	hwndStatusBar = CreateWindowEx(0, STATUSCLASSNAME, NULL,
		WS_CHILD | SBARS_SIZEGRIP, 0, 0, 0, 0, this->hWnd, (HMENU)IDC_STATUSBAR,
		this->hInst, NULL);
	if (hwndStatusBar == NULL)
		return NULL;

	// Set the status bar parts and sizing and show it.
	//ResizeStatusBar(&rc);
	ShowWindow(hwndStatusBar, SW_SHOW);

	return hwndStatusBar;
}

/**
 * Resizes the status bar and ensures the aspect ratio of its parts.
 *
 * @param lprcClient Parent window's client area.
 */
void MainWindow::ResizeStatusBar(LPCRECT lprcClient) {
	// Create status bar parts.
	const int nParts = 2;
	int arrParts[nParts] = {
		(int)(lprcClient->right * 0.7), (int)(lprcClient->right * 0.3)
	};

	// Get our own size.
	RECT rc;
	GetClientRect(hwndStatusBar, &rc);

	// Resize everything.
	SetWindowPos(hwndStatusBar, NULL, 0, lprcClient->bottom - rc.bottom,
		lprcClient->right, 0, SWP_NOZORDER);
	SendMessage(hwndStatusBar, SB_SETPARTS, (WPARAM)nParts, (LPARAM)arrParts);
}

/**
 * Creates the Gopher directory ListView window.
 *
 * @return ListView window handle.
 */
HWND MainWindow::CreateDirectoryView() {
	// Get parent client rectangle.
	RECT rc;
	GetClientRect(this->hwndRebar, &rc);

	// Create ListView window.
	hwndDirectory = CreateWindow(WC_LISTVIEW, _T(""), WS_CHILD | LVS_REPORT |
		LVS_NOSORTHEADER, 0, rc.bottom, rc.right, 200, this->hWnd,
		(HMENU)IDC_LSTDIRECTORY, hInst, NULL);
	if (hwndDirectory == NULL)
		return NULL;

	// Show the directory window.
	ShowWindow(hwndDirectory, SW_SHOW);

	return hwndDirectory;
}
