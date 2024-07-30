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

	// Image lists.
	HIMAGELIST himlToolbar;

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
	void AddDirectoryEntry(size_t nIndex);

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
	void DirectoryListViewNotify(NMLVDISPINFO *lpVDI);

	// Browser navigation.
	void BrowseTo(LPCTSTR szURL);
};

#endif // _RODENT_MAINWINDOW_H
