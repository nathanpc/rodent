/**
 * DownloadDialog.h
 * A download progress indicator dialog.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _RODENT_DOWNLOADDIALOG_H
#define _RODENT_DOWNLOADDIALOG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stdafx.h"
#include "Utilities/DialogWindow.h"
#include "GopherWrapper.h"

/**
 * A download progress indicator dialog.
 */
class DownloadDialog : public DialogWindow {
private:
	// Static labels.
	HWND hwndURLLabel;
	HWND hwndPathLabel;
	HWND hwndSizeLabel;

	// Command buttons.
	HWND hwndOpenFileButton;
	HWND hwndOpenFolderButton;
	HWND hwndCancelButton;

	// State variables.
	bool bCancelIsClose;
	bool bAutoOpen;
	LPTSTR szOpenProgram;

	// Gopher stuff.
	Gopher::FileDownload *fdl;
	static void FileTransferThreadProc(void *lpArgs);
	typedef struct {
		DownloadDialog *lpThis;
		gopher_addr_t *addr;
		gopher_type_t type;
	} FileDownloadArgs;
	static void FileTransferReportProc(const void *lpgf, void *lpvThis);

	// Event handlers.
	INT_PTR OpenFile(HWND hDlg);
	INT_PTR OpenFolder(HWND hDlg);
	INT_PTR Cancel(HWND hDlg);

	// Dialog procedure and initial setup.
	void SetupControls(HWND hDlg);
	INT_PTR CALLBACK DlgProc(HWND hDlg, UINT wMsg, WPARAM wParam,
							 LPARAM lParam);

	// Control helpers.
	void EnableOpenButtons(bool bEnable);
	void SwitchCancelButtonToClose(bool bMakeDefault, bool bEnableOpen);
	void UpdateFileDetails();
	void SetTransferredBytes(size_t bytes);

public:
	// Constructor and destructor.
	DownloadDialog(HINSTANCE& hInst, HWND& hwndParent);
	virtual ~DownloadDialog();

	// Operations
	void SetOpenProgram(LPCTSTR szProgramPath);
	void Download(const Gopher::Item& item, bool bAutoOpen);
};

#endif // _RODENT_DOWNLOADDIALOG_H
