/**
 * MainWindow.h
 * Main (browser) window of the application.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _RODENT_MAINWINDOW_H
#define _RODENT_MAINWINDOW_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stdafx.h"
#include "GopherWrapper.h"

class MainWindow {
private:
	// Gopher stuff.
	Gopher::Directory *goInitialDirectory;
	Gopher::Directory *goDirectory;
	tstring strInitialURL;
	bool bFetching;

	// Image lists.
	HIMAGELIST himlToolbar;
	HIMAGELIST himlBrowser;

	// Toolbars and toolbar controls.
	HWND hwndToolbar;
	HWND hwndRebar;
	HWND hwndAddressBar;
	HWND hwndAddressCombo;

	// Other controls.
	HWND hwndDirectory;
	HWND hwndStatusBar;

	// Window controls creation.
	HWND CreateToolbar(LPSIZE lpSize);
	HWND CreateAddressBar(LPSIZE lpSize);
	HWND CreateRebar();
	HWND CreateStatusBar();
	HWND CreateDirectoryView();

	// Sizing helpers.
	void ResizeStatusBar(LPCRECT lprcClient);

	// Control helpers.
	void LoadDirectory();
	void AddDirectoryEntry(size_t nIndex);
	void SetStatusAddress(LPCTSTR szAddress);
	void SetStatusMessage(LPCTSTR szMsg);
	int ItemTypeIconIndex(gopher_type_t type);
	LRESULT DirectoryItemPrePaint(LPNMLVCUSTOMDRAW lvcd);

	// Download helpers.
	void OpenShellLink(const Gopher::Item& goItem);
	void DownloadFile(const Gopher::Item& goItem, bool bOpen);
	void DownloadTextFile(const Gopher::Item& goItem);
	void DownloadImage(const Gopher::Item& goItem);

	// Parallel computing.
	static void FetchDirectoryThreadProc(void *lpArgs);

	// Parallel computing struct helpers.
	typedef struct {
		MainWindow *lpThis;
		gopher_addr_t *addr;
	} DirectoryFetchArgs;

public:
	// Global handles.
	HINSTANCE hInst;
	HWND hWnd;

	// Constructors and destructors.
	MainWindow(HINSTANCE hInstance, LPCTSTR szURI);
	virtual ~MainWindow();

	// Controls setup.
	BOOL SetupControls(HWND hWnd);
	BOOL ResizeWindows(HWND hwndParent);
	void UpdateControls();
	void SetFetching(bool bFetching, bool bUpdate);

	// Browser navigation.
	void BrowseTo();
	void BrowseTo(LPCTSTR szURL);
	void BrowseTo(const Gopher::Item& goItem);
	void BrowseTo(gopher_addr_t *addr);
	void GoBack();
	void GoNext();
	void GoToParent();

	// Notification handling.
	LRESULT HandleItemHover(LPNMHDR nmh);
	LRESULT HandleItemHotTrack(LPNMLISTVIEW nmlv);
	LRESULT HandleItemActivate(LPNMITEMACTIVATE nmia);
	LRESULT HandleDirectoryCustomDraw(LPNMLVCUSTOMDRAW lvcd);
	LRESULT HandleLoadingTimer(int iOper);

	// Checking for notifications.
	BOOL IsDirectoryListView(HWND hWnd) const;
	BOOL IsAddressComboBox(HWND hWnd) const;
};

#endif // _RODENT_MAINWINDOW_H
