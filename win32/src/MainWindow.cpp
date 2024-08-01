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
	himlBrowser = NULL;
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
	if (himlBrowser) {
		ImageList_Destroy(himlBrowser);
		himlBrowser = NULL;
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
		// Free the current directory.
		if (goDirectory != goInitialDirectory) {
			delete goDirectory;
			goDirectory = nullptr;
		}

		// Free the initial directory.
		goInitialDirectory->free(RECURSE_BACKWARD | RECURSE_FORWARD);
		delete goInitialDirectory;
		goInitialDirectory = nullptr;
	}
}

/**
 * Navigates to the address specified in the address bar.
 */
void MainWindow::BrowseTo() {
	// Get URL from address bar.
	LPTSTR szAddress = GetWindowTextAlloc(hwndAddressCombo);
	BrowseTo(szAddress);
	free(szAddress);
}

/**
 * Navigates the browser to a specific gopherspace URL.
 *
 * @param szURL Gopherspace URL to navigate to. Use NULL to get the value from
 *              the address combo control.
 */
void MainWindow::BrowseTo(LPCTSTR szURL) {
	gopher_type_t type = GOPHER_TYPE_UNKNOWN;
	gopher_addr_t *addr = NULL;

	try {
		// Get gopherspace address structure from URL.
		addr = Gopher::Address::from_url(szURL, &type);
	} catch (const std::exception& e) {
		MsgBoxException(this->hWnd, e, _T("Failed to parse URL"));
		UpdateControls();
		return;
	}

	// Browse to the address.
	BrowseTo(addr, type);
}

/**
 * Navigates the browser to a Gopher entry item.
 *
 * @param goItem Gopher entry item to navigate to.
 */
void MainWindow::BrowseTo(const Gopher::Item& goItem) {
	LPTSTR szAddress = goItem.to_url();
	BrowseTo(szAddress);
	free(szAddress);
	szAddress = NULL;
}

/**
 * Navigates the browser using a gopherspace address structure.
 *
 * @param addr Internal gopherspace address structure to navigate to. This will
 *             be owned by the internals of the API, ensure it won't conflict.
 */
void MainWindow::BrowseTo(gopher_addr_t *addr, gopher_type_t type) {
	// If the type is not a directory or unknown (assuming directory) try to open it.
	if ((type != GOPHER_TYPE_DIR) && (type != GOPHER_TYPE_UNKNOWN)) {
		MsgBoxError(this->hWnd, _T("Still not implemented"),
			_T("Navigating directly to files is not yet implemented."));
		return;
	}

	// Ensure we don't run into weird race conditions with the directory.
	ListView_DeleteAllItems(hwndDirectory);

	try {
		if (goInitialDirectory != nullptr) {
			// Get directory from requested address.
			Gopher::Directory *dirOld = goDirectory;
			goDirectory = dirOld->push(addr);
			if (dirOld != goInitialDirectory)
				delete dirOld;
		} else {
			// Ensure we have an initial directory to start off.
			goInitialDirectory = new Gopher::Directory(addr);
			goDirectory = goInitialDirectory;
		}
	} catch (const std::exception& e) {
		MsgBoxException(this->hWnd, e, _T("Failed to browse to address"));
		UpdateControls();
		return;
	}

	// Update the interface to show our fetched directory.
	LoadDirectory();
}

/**
 * Goes back to the previous directory in the history stack.
 */
void MainWindow::GoBack() {
	// Check if we have something to go to.
	if (!goDirectory->has_prev()) {
		MsgBoxInfo(this->hWnd, _T("No previous gopherhole"),
			_T("No previous gopherhole is available to go back to."));
		return;
	}

	// Go to the previous directory.
	Gopher::Directory *dirOld = goDirectory;
	goDirectory = dirOld->prev();
	delete dirOld;

	// Update everything.
	LoadDirectory();
}

/**
 * Goes to the next directory in the history stack.
 */
void MainWindow::GoNext() {
	// Check if we have something to go to.
	if (!goDirectory->has_next()) {
		MsgBoxInfo(this->hWnd, _T("No next gopherhole"),
			_T("No next gopherhole is available to go forward to."));
		return;
	}

	// Go to the previous directory.
	Gopher::Directory *dirOld = goDirectory;
	goDirectory = dirOld->next();
	delete dirOld;

	// Update everything.
	LoadDirectory();
}

/**
 * Navigates to the parent of the current directory.
 */
void MainWindow::GoToParent() {
	// Get the directory and check if it's even possible to move to a parent.
	gopher_addr_t *addr = goDirectory->parent();
	if (addr == NULL) {
		MsgBoxInfo(this->hWnd, _T("No parent available"),
			_T("You are already at the top-level of the server."));
		return;
	}

	// Navigate to the parent.
	BrowseTo(addr, GOPHER_TYPE_DIR);
}

/**
 * Loads the current directory in the UI.
 */
void MainWindow::LoadDirectory() {
	// Start with a clean slate.
	ListView_DeleteAllItems(hwndDirectory);

	// Reset the address bar contents with parsed address.
	TCHAR *szNewURL = Gopher::Address::as_url(goDirectory->c_dir()->addr,
		GOPHER_TYPE_DIR);
	SetWindowText(hwndAddressCombo, szNewURL);
	free(szNewURL);

	// Shame non-compliant servers.
	if (goDirectory->error_count() > 0) {
		tstring strMsg;
		strMsg += goDirectory->error_count();
		strMsg += _T(" warnings");
		SetStatusMessage(strMsg.c_str());
	} else {
		SetStatusMessage(_T("OK"));
	}

	// Populate the directory list view.
	if (goDirectory->items_count() > 0) {
		for (size_t i = 0; i < goDirectory->items_count(); i++)
			AddDirectoryEntry(i);
	} else {
		MsgBoxInfo(this->hWnd, _T("Empty directory"),
			_T("This page was intentionally left blank."));
	}
	ListView_SetColumnWidth(hwndDirectory, 0, LVSCW_AUTOSIZE);

	// Ensure the state of everything is up-to-date.
	UpdateControls();
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
	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
	lvi.state = 0; 
	lvi.stateMask = 0; 
   	lvi.iItem = nIndex;
	lvi.iImage = ItemTypeIconIndex(item.type());
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
 * Sets the text of the address part of the status bar.
 *
 * @param szAddress Address to be displayed in the status bar.
 */
void MainWindow::SetStatusAddress(LPCTSTR szAddress) {
	SendMessage(hwndStatusBar, SB_SETTEXT, (WPARAM)0, (LPARAM)szAddress);
}

/**
 * Sets the text of the message part of the status bar.
 *
 * @param szMsg Message to be displayed in the status bar.
 */
void MainWindow::SetStatusMessage(LPCTSTR szMsg) {
	SendMessage(hwndStatusBar, SB_SETTEXT, (WPARAM)1, (LPARAM)szMsg);
}

/**
 * Handles the actions to take place when the user hovers over an item in the
 * directory ListView.
 *
 * @param nmh Information about the NM_HOVER notification.
 *
 * @return 0 to allow the message to be propagated further.
 */
LRESULT MainWindow::HandleItemHover(LPNMHDR nmh) {
	// Get hovered item.
	int nIndex = ListView_GetHotItem(hwndDirectory);
	if (nIndex < 0)
		return 1;
	Gopher::Item item = goDirectory->items()->at(nIndex);

	// Put address of hovered item in status bar.
	LPTSTR szAddress = item.to_url();
	SetStatusAddress(szAddress);
	free(szAddress);
	szAddress = NULL;

	// Don't select the item automatically.
	return 1;
}

/**
 * Handles the actions to take place when the user hovers over an item in the
 * directory ListView.
 *
 * @param nmlv Information about the LVN_HOTTRACK notification.
 *
 * @return 0 to allow the message to be propagated further.
 */
LRESULT MainWindow::HandleItemHotTrack(LPNMLISTVIEW nmlv) {
	// Ensure the index is valid.
	if (nmlv->iItem < 0)
		return 1;

	// Get hovered item.
	Gopher::Item item = goDirectory->items()->at(nmlv->iItem);

	// Ignore information items.
	if (item.type() == GOPHER_TYPE_INFO) {
		SetStatusAddress(_T(""));
		nmlv->iItem = -1;
	}

	return 0;
}

/**
 * Handles the actions to take place when the user clicks on an item in the
 * directory ListView.
 *
 * @param nmia Information about the LVN_ITEMACTIVATE notification.
 *
 * @return Must always return 0.
 */
LRESULT MainWindow::HandleItemActivate(LPNMITEMACTIVATE nmia) {
	// Get hovered item.
	Gopher::Item item = goDirectory->items()->at(nmia->iItem);

	// Ignore information items.
	switch (item.type()) {
	case GOPHER_TYPE_INFO:
	case GOPHER_TYPE_ERROR:
		// Do nothing on info or error entries.
		break;
	case GOPHER_TYPE_DIR:
		// Handle directory links.
		BrowseTo(item);
		break;
	case GOPHER_TYPE_TEXT:
	case GOPHER_TYPE_XML:
		// Handle text links.
		DownloadTextFile(item);
		break;
	case GOPHER_TYPE_BINHEX:
	case GOPHER_TYPE_UNIX:
	case GOPHER_TYPE_DOS:
	case GOPHER_TYPE_BINARY:
		// Handle binary and "binary" links.
		// TODO: Download to downloads folder.
		MsgBoxError(this->hWnd, _T("Not yet implemented"),
			_T("Downloading binaries hasn't been implemented yet."));
		break;
	case GOPHER_TYPE_SEARCH:
		// Handle search links.
		MsgBoxError(this->hWnd, _T("Search not supported"),
			_T("The search feature still hasn't been implemented."));
		break;
	case GOPHER_TYPE_TELNET:
	case GOPHER_TYPE_TN3270:
		// Handle telnet links.
		OpenShellLink(item);
		break;
	case GOPHER_TYPE_GIF:
	case GOPHER_TYPE_IMAGE:
	case GOPHER_TYPE_BITMAP:
	case GOPHER_TYPE_PNG:
		// Handle image files.
		DownloadImage(item);
		break;
	case GOPHER_TYPE_MOVIE:
	case GOPHER_TYPE_AUDIO:
	case GOPHER_TYPE_WAV:
	case GOPHER_TYPE_DOC:
	case GOPHER_TYPE_PDF:
		// Handle binary files with auto open.
		DownloadOpenDefault(item);
		break;
	case GOPHER_TYPE_HTML:
		// Handle HTTP requests using a proper browser.
		OpenShellLink(item);
		break;
	default:
		// Unknown file types.
		MsgBoxError(this->hWnd, _T("Unknown entry type"),
			_T("Unable to open an entry which the type is unknown to the ")
			_T("application. Please contact the developer."));
		break;
	}

	return 0;
}

/**
 * Opens a link referenced in a Gopher entry item.
 *
 * @param goItem Gopher entry item to open its referenced link.
 */
void MainWindow::OpenShellLink(const Gopher::Item& goItem) {
	std::string strURL;

	// Check if it's a telnet item.
	switch (goItem.type()) {
	case GOPHER_TYPE_TELNET:
	case GOPHER_TYPE_TN3270:
		// Create telnet URL.
		MsgBoxError(this->hWnd, _T("Telnet not yet supported"),
			_T("The telnet feature still hasn't been implemented."));
		return;
	case GOPHER_TYPE_HTML:
		// Convert to HTTP URL.
		strURL = goItem.c_item()->addr->selector;
		if (strURL.find("URL:") != 0) {
			MsgBoxError(this->hWnd, _T("Unsupported hyperlink"),
				_T("Unsupported hyperlink format in selector string."));
			return;
		}
		strURL = strURL.substr(4);
		break;
	}

	// Open the requested link.
#ifdef UNICODE
	LPTSTR szURL = win_mbstowcs(strURL.c_str());
	ShellExecute(this->hWnd, _T("open"), szURL, NULL, NULL, SW_NORMAL);
	free(szURL);
	szURL = NULL;
#else
	ShellExecute(this->hWnd, _T("open"), strURL.c_str(), NULL, NULL, SW_NORMAL);
#endif // UNICODE
}

/**
 * Downloads a file related to an item.
 *
 * @param goItem Gopher entry item to download.
 *
 * @return Downloaded file object.
 */
Gopher::FileDownload *MainWindow::DownloadFile(const Gopher::Item& goItem) {
	Gopher::FileDownload *fdl = new Gopher::FileDownload();

	try {
		fdl->download(goItem.c_item()->addr, goItem.type(), nullptr);
	} catch (const std::exception& e) {
		MsgBoxException(this->hWnd, e, _T("Failed to download file"));
		delete fdl;
		return nullptr;
	}

	return fdl;
}

/**
 * Downloads and displays a text file to the user.
 *
 * @param goItem Gopher entry item to download.
 */
void MainWindow::DownloadTextFile(const Gopher::Item& goItem) {
	// Download the file.
	Gopher::FileDownload *fdl = DownloadFile(goItem);
	if (fdl == nullptr)
		return;

	// Open the file.
	ShellExecute(this->hWnd, _T("open"), _T("notepad.exe"), fdl->path(), NULL,
		SW_NORMAL);
	delete fdl;
}

/**
 * Downloads and displays an image to the user.
 *
 * @param goItem Gopher entry item to download.
 */
void MainWindow::DownloadImage(const Gopher::Item& goItem) {
	DownloadOpenDefault(goItem);
}

/**
 * Downloads a file and automatically open it with the default application.
 *
 * @param goItem Gopher entry item to download.
 */
void MainWindow::DownloadOpenDefault(const Gopher::Item& goItem) {
	// Download the file.
	Gopher::FileDownload *fdl = DownloadFile(goItem);
	if (fdl == nullptr)
		return;

	// Open the file.
	int ret = (int)ShellExecute(this->hWnd, _T("open"), fdl->path(), NULL,
		NULL, SW_NORMAL);
	if (ret <= 32) {
		// Looks like we were unable to open the file. Display a picker dialog.
		// TODO: Use the "openas" verb or SHOpenWithDialog on Windows Vista and above.
		//       https://stackoverflow.com/questions/6364879
		tstring strParams = _T("Shell32,OpenAs_RunDLL ");
		strParams += fdl->path();
		ShellExecute(this->hWnd, _T("open"), _T("RUNDLL32"), strParams.c_str(),
			NULL, SW_NORMAL);
	}

	// Delete the file download object.
	delete fdl;
}

/**
 * Updates the state of controls related to the browser to reflect changes in
 * internal objects.
 */
void MainWindow::UpdateControls() {
	SendMessage(hwndToolbar, TB_ENABLEBUTTON, (WPARAM)IDM_BACK,
		(LPARAM)(goDirectory && goDirectory->has_prev()));
	SendMessage(hwndToolbar, TB_ENABLEBUTTON, (WPARAM)IDM_NEXT,
		(LPARAM)(goDirectory && goDirectory->has_next()));
	SendMessage(hwndToolbar, TB_ENABLEBUTTON, (WPARAM)IDM_PARENT,
		(LPARAM)(goDirectory && goDirectory->has_parent()));
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

	// Get everything on a known state.
	UpdateControls();

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
	const int iImages = 6;
	const int bitmapSize = 16;
	himlToolbar = ImageList_Create(bitmapSize, bitmapSize,
		ILC_MASK | ILC_COLOR32, iImages, 0);
	SendMessage(hwndToolbar, TB_SETIMAGELIST, (WPARAM)0, (LPARAM)himlToolbar);

	// Load Toolbar button images into image list.
	HICON hIcon;
	hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_LEFT));
	ImageList_AddIcon(himlToolbar, hIcon);
	DestroyIcon(hIcon);
	hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_RIGHT));
	ImageList_AddIcon(himlToolbar, hIcon);
	DestroyIcon(hIcon);
	hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_UP));
	ImageList_AddIcon(himlToolbar, hIcon);
	DestroyIcon(hIcon);
	hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_REFRESH));
	ImageList_AddIcon(himlToolbar, hIcon);
	DestroyIcon(hIcon);
	hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_STOP));
	ImageList_AddIcon(himlToolbar, hIcon);
	DestroyIcon(hIcon);
	hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_PLAY));
	ImageList_AddIcon(himlToolbar, hIcon);
	DestroyIcon(hIcon);

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
		(int)(lprcClient->right * 0.7),
		-1
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
		LVS_NOSORTHEADER | LVS_NOCOLUMNHEADER | LVS_SHAREIMAGELISTS, 0,
		rc.bottom, rc.right, 200, this->hWnd, (HMENU)IDC_LSTDIRECTORY, hInst,
		NULL);
	if (hwndDirectory == NULL)
		return NULL;
	ListView_SetExtendedListViewStyle(hwndDirectory, LVS_EX_FULLROWSELECT |
		LVS_EX_TRACKSELECT | LVS_EX_ONECLICKACTIVATE | LVS_EX_UNDERLINEHOT);
	ListView_SetHoverTime(hwndDirectory, 10);

	// Ensure we use a monospace font for that nice ASCII art.
	HFONT hFont = (HFONT)GetStockObject(ANSI_FIXED_FONT);
	SendMessage(hwndDirectory, WM_SETFONT, (WPARAM)hFont, (LPARAM)NULL);

	// Create the directory image list and assign it to the ListView.
	const int iImages = 13;
	const int bitmapSize = 16;
	himlBrowser = ImageList_Create(bitmapSize, bitmapSize,
		ILC_MASK | ILC_COLOR32, iImages, 0);
	ListView_SetImageList(hwndDirectory, himlBrowser, LVSIL_SMALL);

	// Load Toolbar button images into image list.
	HICON hIcon;
	hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_BLANK));
	ImageList_AddIcon(himlBrowser, hIcon);
	DestroyIcon(hIcon);
	hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_TYPE_UNKNOWN));
	ImageList_AddIcon(himlBrowser, hIcon);
	DestroyIcon(hIcon);
	hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_TYPE_ERROR));
	ImageList_AddIcon(himlBrowser, hIcon);
	DestroyIcon(hIcon);
	hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_TYPE_FOLDER));
	ImageList_AddIcon(himlBrowser, hIcon);
	DestroyIcon(hIcon);
	hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_TYPE_TEXT));
	ImageList_AddIcon(himlBrowser, hIcon);
	DestroyIcon(hIcon);
	hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_TYPE_BIN));
	ImageList_AddIcon(himlBrowser, hIcon);
	DestroyIcon(hIcon);
	hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_TYPE_SEARCH));
	ImageList_AddIcon(himlBrowser, hIcon);
	DestroyIcon(hIcon);
	hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_TYPE_IMAGE));
	ImageList_AddIcon(himlBrowser, hIcon);
	DestroyIcon(hIcon);
	hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_TYPE_WEB));
	ImageList_AddIcon(himlBrowser, hIcon);
	DestroyIcon(hIcon);
	hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_TYPE_TELNET));
	ImageList_AddIcon(himlBrowser, hIcon);
	DestroyIcon(hIcon);
	hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_TYPE_AUDIO));
	ImageList_AddIcon(himlBrowser, hIcon);
	DestroyIcon(hIcon);
	hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_TYPE_VIDEO));
	ImageList_AddIcon(himlBrowser, hIcon);
	DestroyIcon(hIcon);
	hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_TYPE_DOC));
	ImageList_AddIcon(himlBrowser, hIcon);
	DestroyIcon(hIcon);

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

/**
 * Gets an icon index from the entry type image list.
 *
 * @param type Gopher entry item type.
 *
 * @return Icon index for the associated entry type in the directory image list.
 */
int MainWindow::ItemTypeIconIndex(gopher_type_t type) {
	switch (type) {
	case GOPHER_TYPE_INFO:
		// Info (should be blank).
		return 0;
	case GOPHER_TYPE_ERROR:
		// Server errors.
		return 2;
	case GOPHER_TYPE_DIR:
		// Directory links.
		return 3;
	case GOPHER_TYPE_TEXT:
	case GOPHER_TYPE_XML:
		// Text and XML.
		return 4;
	case GOPHER_TYPE_BINHEX:
	case GOPHER_TYPE_UNIX:
	case GOPHER_TYPE_DOS:
	case GOPHER_TYPE_BINARY:
		// Binary and executables.
		return 5;
	case GOPHER_TYPE_SEARCH:
		// Search links.
		return 6;
	case GOPHER_TYPE_GIF:
	case GOPHER_TYPE_IMAGE:
	case GOPHER_TYPE_BITMAP:
	case GOPHER_TYPE_PNG:
		// Images and pictures.
		return 7;
	case GOPHER_TYPE_HTML:
		// Web link.
		return 8;
	case GOPHER_TYPE_TELNET:
	case GOPHER_TYPE_TN3270:
		// Telnet links.
		return 9;
	case GOPHER_TYPE_AUDIO:
	case GOPHER_TYPE_WAV:
		// Audio and music.
		return 10;
	case GOPHER_TYPE_MOVIE:
		// Video file.
		return 11;
	case GOPHER_TYPE_DOC:
	case GOPHER_TYPE_PDF:
		// Document files.
		return 12;
	default:
		// Unknown file types.
		return 1;
	}
}

/**
 * Checks if a window handle is for the directory list view.
 *
 * @param hWnd Window handle to be tested.
 *
 * @return TRUE if the window handle is in fact of the directory list view.
 */
BOOL MainWindow::IsDirectoryListView(HWND hWnd) const {
	return this->hwndDirectory == hWnd;
}

/**
 * Checks if a window handle is for the address bar ComboBoxEx.
 *
 * @param hWnd Window handle to be tested.
 *
 * @return TRUE if the window handle is in fact of the address bar ComboBoxEx.
 */
BOOL MainWindow::IsAddressComboBox(HWND hWnd) const {
	return this->hwndAddressCombo == hWnd;
}
