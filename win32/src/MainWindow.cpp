/**
 * MainWindow.cpp
 * Main (browser) window of the application.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "MainWindow.h"

/**
 * Constructs the main window object.
 *
 * @param hInstance Application instance.
 * @param szURI     Optional. Initial URI to load on open.
 */
MainWindow::MainWindow(HINSTANCE hInstance, LPCTSTR szURI) {
	// Initialize important stuff.
	this->hInst = hInstance;
	strInitialURL = (szURI) ? szURI : _T("gopher://gopher.floodgap.com/1/overbite");

	// Initialize default values.
	goInitialDirectory = nullptr;
	goDirectory = nullptr;
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

	// Destroy everything related to the directory ListView.
	if (hwndDirectory) {
		DestroyWindow(hwndDirectory);
		hwndDirectory = NULL;
	}

	// Destroy status bar.
	if (hwndStatusBar) {
		DestroyWindow(hwndStatusBar);
		hwndStatusBar = NULL;
	}

	// Destroy the main window.
	DestroyWindow(this->hWnd);
	this->hWnd = NULL;

	// Free up any resources allocated by the Gopher client implementation.
	if (goInitialDirectory) {
		goInitialDirectory->free(RECURSE_BACKWARD | RECURSE_FORWARD);
		delete goInitialDirectory;
		goInitialDirectory = nullptr;
		goDirectory = nullptr;
	}
}

/**
 * Navigates the browser to a specific gopherspace URL.
 *
 * @param szURL Gopherspace URL to navigate to.
 */
void MainWindow::BrowseTo(LPCTSTR szURL) {
	try {
		// Get gopherspace address structure from URL.
		gopher_addr_t *addr = Gopher::Address::from_url(szURL);

		// TODO: Reset the address bar contents with parsed address.

		if (goInitialDirectory != nullptr) {
			// TODO: Get directory from requested address.
		} else {
			// Ensure we have an initial directory to start off.
			goInitialDirectory = new Gopher::Directory(addr);
			goDirectory = goInitialDirectory;
		}
	} catch (const std::exception& e) {
#ifdef UNICODE
		// Convert the Unicode string to multi-byte.
		TCHAR *szMessage = win_mbstowcs(e.what());
		if (szMessage == NULL)
			throw std::exception("Failed to convert exception to wide string");
#else
		const char *szMessage = e.what();
#endif // UNICODE

		MsgBoxError(this->hWnd, _T("Failed to browse to URL"), szMessage);

#ifdef UNICODE
		// Free the temporary buffer.
		std::free(szMessage);
#endif // UNICODE

		return;
	}

	// Populate the directory list view.
	ListView_DeleteAllItems(hwndDirectory);
	if (goDirectory->items_count() > 0) {
		for (size_t i = 0; i < goDirectory->items_count(); i++)
			AddDirectoryEntry(i);
	} else {
		MsgBoxInfo(this->hWnd, _T("Empty directory"),
			_T("This page was intentionally left blank."));
	}
}

/**
 * Appends a directory entry item to the ListView.
 *
 * @param nIndex Index of the item in the directory object.
 */
void MainWindow::AddDirectoryEntry(size_t nIndex) {
	Gopher::Item item = goDirectory->items()->at(nIndex);
	LVITEM lvi;

	// Populate ListView item structure.
	lvi.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
	lvi.state = 0; 
	lvi.stateMask = 0; 
   	lvi.iItem = nIndex;
	//lvi.iImage = index;
	lvi.iSubItem = 0;
	lvi.lParam = (LPARAM)&goDirectory->items()->at(nIndex);
	lvi.pszText = item.label();

	// Insert the item into the ListView.
	if (ListView_InsertItem(hwndDirectory, &lvi) == -1) {
		MsgBoxError(this->hWnd, _T("List view error"),
			_T("An error occurred while trying to add an entry to the ")
			_T("browser's ListView."));
	}
}

/**
 * Sets up the layout of the window's controls.
 *
 * @param hWnd Main window handle.
 *
 * @return TRUE if the operation was successful.
 */
BOOL MainWindow::SetupControls(HWND hWnd) {
	// Ensure that the common controls DLL is loaded and initialized. 
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC  = ICC_WIN95_CLASSES | ICC_COOL_CLASSES | ICC_USEREX_CLASSES;
	InitCommonControlsEx(&icex);

	// Ensure we have a copy of the window handle.
	this->hWnd = hWnd;

	// Add controls to the window.
	if (CreateRebar() == NULL)
		return FALSE;
	if (CreateStatusBar() == NULL)
		return FALSE;
	if (CreateDirectoryView() == NULL)
		return FALSE;

	// Go to the initial gopherhole.
	BrowseTo(strInitialURL.c_str());

	return TRUE;
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
	LVCOLUMN lvc;

	// Get parent client rectangle.
	RECT rc;
	GetClientRect(this->hwndRebar, &rc);

	// Create ListView window.
	hwndDirectory = CreateWindow(WC_LISTVIEW, _T(""), WS_CHILD | LVS_REPORT |
		LVS_NOSORTHEADER, 0, rc.bottom, rc.right, 200, this->hWnd,
		(HMENU)IDC_LSTDIRECTORY, hInst, NULL);
	if (hwndDirectory == NULL)
		return NULL;

	// Setup common column properties.
	int iCol = 0;
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvc.fmt = LVCFMT_LEFT;

	// Create entry label column.
	lvc.iSubItem = iCol;
	lvc.pszText = _T("Label");
	lvc.cx = 400;
	if (ListView_InsertColumn(hwndDirectory, iCol++, &lvc) == -1)
		return NULL;

	// Show the directory window.
	ShowWindow(hwndDirectory, SW_SHOW);

	return hwndDirectory;
}
