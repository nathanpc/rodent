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
	// Image lists.
	HIMAGELIST himlToolbar;

	// Toolbars.
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

public:
	// Global handles.
	HINSTANCE hInst;
	HWND hWnd;

	// Constructors and destructors.
	MainWindow(HINSTANCE hInstance);
	virtual ~MainWindow();

	// Controls setup.
	BOOL SetupControls(HWND hWnd);
	BOOL ResizeWindows(HWND hwndParent);
};

#endif // _RODENT_MAINWINDOW_H
