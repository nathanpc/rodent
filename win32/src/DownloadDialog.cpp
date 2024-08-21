/**
 * DownloadDialog.cpp
 * A download progress indicator dialog.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "DownloadDialog.h"

#include <process.h>
#include <shlwapi.h>
#include <shlobj.h>

/**
 * Message to signal the end of the download thread.
 * 
 * @param wParam Should be 0.
 * @param lParam Flag indicating the state of the transfer: DL_STATE_SUCCESS,
 *               DL_STATE_FAILED, DL_STATE_CANCELLED
 */
#define DL_FINISHED (WM_APP + 10)
#define DL_STATE_FAILED    -1
#define DL_STATE_SUCCESS    0
#define DL_STATE_CANCELLED  1

/**
 * Initializes the dialog window object.
 *
 * @param hInst      Application's instance that this dialog belongs to.
 * @param hwndParent Parent window handle.
 */
DownloadDialog::DownloadDialog(HINSTANCE& hInst, HWND& hwndParent) :
	DialogWindow(hInst, hwndParent, IDD_DOWNLOAD) {
	this->hwndURLLabel = NULL;
	this->hwndPathLabel = NULL;
	this->hwndSizeLabel = NULL;

	this->hwndOpenFileButton = NULL;
	this->hwndOpenFolderButton = NULL;
	this->hwndCancelButton = NULL;

	this->bCancelIsClose = false;
	this->bAutoOpen = false;
	this->szOpenProgram = NULL;

	this->fdl = NULL;
}

/**
 * Frees up any resources allocated by this dialog.
 */
DownloadDialog::~DownloadDialog() {
	if (this->fdl) {
		delete fdl;
		fdl = nullptr;
	}

	if (this->szOpenProgram)
		free(this->szOpenProgram);
}

/**
 * Begins downloading a file from the server. This method will automatically
 * show the dialog.
 *
 * @param item      Gopher entry item to download.
 * @param bAutoOpen Automatically open the file as soon as the download finishes.
 */
void DownloadDialog::Download(const Gopher::Item& item, bool bAutoOpen) {
	this->bAutoOpen = bAutoOpen;
	this->Show();

	// Hide open buttons if the dialog is auto open.
	if (bAutoOpen) {
		ShowWindow(hwndOpenFileButton, SW_HIDE);
		ShowWindow(hwndOpenFolderButton, SW_HIDE);
	}

	// Download file on a separate thread.
	FileDownloadArgs *fda = (FileDownloadArgs *)malloc(
		sizeof(FileDownloadArgs));
	fda->lpThis = this;
	fda->addr = item.c_item()->addr;
	fda->type = item.type();
	HANDLE hDownloadThread = (HANDLE)_beginthread(
		DownloadDialog::FileTransferThreadProc, 0, (void *)fda);
}

/**
 * File transfer thread procedure.
 *
 * @param lpArgs Pointer to a FileDownloadArgs structure.
 */
void DownloadDialog::FileTransferThreadProc(void *lpArgs) {
	// Cast important bits for us to use.
	FileDownloadArgs *fda = static_cast<FileDownloadArgs *>(lpArgs);
	DownloadDialog *lpThis = fda->lpThis;
	lpThis->fdl = new Gopher::FileDownload();
	int iReturn = DL_STATE_SUCCESS;

	// Setup the transfer.
	if (lpThis->bAutoOpen) {
		// Download to a temporary folder.
		lpThis->fdl->setup_temp(fda->addr, fda->type);
	} else {
		// Let the user select where to download the file to.
		TCHAR szPath[MAX_PATH];
		_tcscpy(szPath, lpThis->fdl->basename(fda->addr));

		// Setup save file dialog.
		OPENFILENAME ofn = {0};
#if _MSC_VER > 1200
		ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
#else
		ofn.lStructSize = sizeof(OPENFILENAME);
#endif // _MSC_VER > 1200
		ofn.hwndOwner = lpThis->hDlg;
		ofn.lpstrTitle = _T("Download file");
		ofn.lpstrFilter = _T("All Files (*.*)\0*.*\0");
		ofn.lpstrFile = szPath;
		ofn.nMaxFile = MAX_PATH;
		ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY |
			OFN_OVERWRITEPROMPT;

		// Open the save file dialog.
		if (!GetSaveFileName(&ofn)) {
			iReturn = DL_STATE_CANCELLED;
			goto dlcleanup;
		}

		// Setup the transfer to the chosen directory.
		lpThis->fdl->setup(fda->addr, fda->type, szPath);
	}

	// Update UI and set the transfer reporting callback.
	lpThis->UpdateFileDetails();
	lpThis->fdl->set_transfer_cb(DownloadDialog::FileTransferReportProc,
		static_cast<void *>(lpThis));

	// Start the file transfer.
	try {
		lpThis->fdl->download();
	} catch (const std::exception& e) {
		MsgBoxException(lpThis->hDlg, e, _T("Failed to download file"));
		iReturn = DL_STATE_FAILED;
		goto dlcleanup;
	}

	// Show in the UI that the file has finished downloading.
	lpThis->SwitchCancelButtonToClose(false, true);

dlcleanup:
	// Queue the download finished message and free temporary resources.
	PostMessage(lpThis->hDlg, DL_FINISHED, (WPARAM)0, (LPARAM)iReturn);
	free(lpArgs);
	lpArgs = NULL;
	lpThis = NULL;
	fda = NULL;

	// Exit the thread.
	_endthread();
}

/**
 * Update UI elements to show information about the file currently being
 * downloaded.
 */
void DownloadDialog::UpdateFileDetails() {
	// Set dialog title.
	SetWindowFormatText(this->hDlg, _T("Downloading %s"),
		this->fdl->basename());

	// Set file information labels.
	SetWindowText(this->hwndPathLabel, this->fdl->path());
	TCHAR *szURL = Gopher::Address::as_url(this->fdl->c_addr());
	SetWindowText(this->hwndURLLabel, szURL);
	free(szURL);
	szURL = NULL;
}

/**
 * Event handler that's called when the Open File button gets clicked.
 *
 * @param hDlg Dialog window handle.
 *
 * @return Value to be returned by the dialog's message handling procedure.
 */
INT_PTR DownloadDialog::OpenFile(HWND hDlg) {
	// Open the file using the default program or the specified one.
	if (this->szOpenProgram) {
		// Open with the specified program.
		ShellExecute(this->hwndParent, _T("open"), this->szOpenProgram,
			this->fdl->path(), NULL, SW_NORMAL);
	} else {
		// Use the system default.
		int ret = (int)ShellExecute(this->hwndParent, _T("open"),
			this->fdl->path(), NULL, NULL, SW_NORMAL);
		if (ret <= 32) {
			// Looks like we were unable to open the file. Display the system
			// application picker dialog.
			// TODO: Use the "openas" verb or SHOpenWithDialog on Windows Vista
			//       and above: https://stackoverflow.com/questions/6364879
			tstring strParams = _T("Shell32,OpenAs_RunDLL ");
			strParams += this->fdl->path();
			ShellExecute(this->hwndParent, _T("open"), _T("RUNDLL32"),
				strParams.c_str(), NULL, SW_NORMAL);
		}
	}

	// Close the dialog.
	Close(IDOK);

	return TRUE;
}

/**
 * Event handler that's called when the Open Folder button gets clicked.
 *
 * @param hDlg Dialog window handle.
 *
 * @return Value to be returned by the dialog's message handling procedure.
 */
INT_PTR DownloadDialog::OpenFolder(HWND hDlg) {
	LPTSTR szFolder;
	LPTSTR szBuffer;

	// Get the folder from the file path.
	szFolder = _tcsdup(this->fdl->path());
	szBuffer = const_cast<LPTSTR>(PathFindFileName(szFolder)) - 1;
	*szBuffer = _T('\0');

	// Open the folder and close the dialog.
	ShellExecute(NULL, _T("explore"), szFolder, NULL, NULL, SW_SHOW);
	free(szFolder);
	szFolder = NULL;
	Close(IDOK);

	return TRUE;
}

/**
 * Event handler that's called when the Cancel button gets clicked. Won't be
 * called after the Cancel button becomes a Close button.
 *
 * @param hDlg Dialog window handle.
 * 
 * @return Value to be returned by the dialog's message handling procedure.
 */
INT_PTR DownloadDialog::Cancel(HWND hDlg) {
	// Cancel transfer and turn the cancel button into a close one.
	// TODO: Cancel the transfer.
	MsgBoxInfo(this->hDlg, _T("Not yet implemented"),
		_T("Sorry, cancelling an ongoing transfer hasn't been implemented."));
	SwitchCancelButtonToClose(true, false);

	return FALSE;
}

/**
 * Dialog window procedure.
 *
 * @param hDlg   Dialog window handle.
 * @param wMsg   Message type.
 * @param wParam Message parameter.
 * @param lParam Message parameter.
 *
 * @return TRUE if the message was handled by the function, FALSE otherwise.
 *
 * @see DefaultDlgProc
 */
INT_PTR CALLBACK DownloadDialog::DlgProc(HWND hDlg, UINT wMsg,
										 WPARAM wParam, LPARAM lParam) {
	// Handle messages.
	switch (wMsg) {
		case WM_INITDIALOG:
			// Setup the controls for the operation.
			SetupControls(hDlg);
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
					// Open File button was clicked.
					return OpenFile(hDlg);
				case IDC_BTOPENFOLDER:
					// Open Folder button was clicked.
					return OpenFolder(hDlg);
				case IDCANCEL: {
					// Call the cancel button event handler if needed.
					if (!bCancelIsClose) {
						INT_PTR iRet = Cancel(hDlg);
						if (iRet)
							return iRet;
					}
					break;
				}
			}
			break;
		case DL_FINISHED:
			// Looks like a download has finished.
			if (lParam != DL_STATE_SUCCESS) {
				Close(IDCANCEL);
				break;
			}
			if (bAutoOpen)
				return OpenFile(hDlg);
			break;
	}

	// Pass the message to the default message handler.
	return DefaultDlgProc(hDlg, wMsg, wParam, lParam);
}

/**
 * Sets up the controls for operation.
 * 
 * @param hDlg Dialog window handle.
 */
void DownloadDialog::SetupControls(HWND hDlg) {
	// Get the handle of every useful control in the window.
	this->hwndURLLabel = GetDlgItem(hDlg, IDC_LBLURL);
	this->hwndPathLabel = GetDlgItem(hDlg, IDC_LBLPATH);
	this->hwndSizeLabel = GetDlgItem(hDlg, IDC_LBLSIZE);
	this->hwndOpenFileButton = GetDlgItem(hDlg, IDOK);
	this->hwndOpenFolderButton = GetDlgItem(hDlg, IDC_BTOPENFOLDER);
	this->hwndCancelButton = GetDlgItem(hDlg, IDCANCEL);

	// Set some default texts for the labels.
	SetWindowText(this->hwndURLLabel, _T(""));
	SetWindowText(this->hwndPathLabel, _T(""));
	SetWindowText(this->hwndSizeLabel, _T(""));

	// Disable the open buttons.
	EnableOpenButtons(false);
}

/**
 * Enables or disables the Open File and Open Folder buttons.
 *
 * @param bEnable Should the buttons be enabled?
 */
void DownloadDialog::EnableOpenButtons(bool bEnable) {
	EnableWindow(this->hwndOpenFileButton, bEnable);
	EnableWindow(this->hwndOpenFolderButton, bEnable);
}

/**
 * Switches the cancel button into a close button.
 * 
 * @param bMakeDefault Make the close button the default in the dialog.
 */
void DownloadDialog::SwitchCancelButtonToClose(bool bMakeDefault,
											   bool bEnableOpen) {
	// Check if Cancel should be the default button of the dialog.
	if (bMakeDefault)
		SetDlgDefaultButton(this->hDlg, IDCANCEL);

	// Enable or disable the open buttons.
	EnableOpenButtons(bEnableOpen);

	// Change the text of the button.
	SetWindowText(this->hwndCancelButton, _T("Close"));
	this->bCancelIsClose = true;
}

/**
 * Sets the number of transferred bytes on the dialog.
 *
 * @param bytes Number of bytes transferred so far.
 */
void DownloadDialog::SetTransferredBytes(size_t bytes) {
	SetWindowFormatText(this->hwndSizeLabel, _T("%d bytes"), bytes);
}

/**
 * Sets the program to be used to open the file when the transfer has finished.
 *
 * @param szProgramPath Path of the program to be used.
 */
void DownloadDialog::SetOpenProgram(LPCTSTR szProgramPath) {
	// Check if we already have one and dispose of it.
	if (this->szOpenProgram)
		free(this->szOpenProgram);

	// Set the program.
	this->szOpenProgram = _tcsdup(szProgramPath);
}

/**
 * Handles the file transfer report events.
 *
 * @param lpgf    Gopher downloaded file object.
 * @param lpvThis Pointer to our own "this" object.
 */
void DownloadDialog::FileTransferReportProc(const void *lpgf, void *lpvThis) {
	const gopher_file_t *gf = static_cast<const gopher_file_t *>(lpgf);
	DownloadDialog *lpThis = static_cast<DownloadDialog *>(lpvThis);

	lpThis->SetTransferredBytes(gf->fsize);
}
