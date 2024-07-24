/**
 * Application.h
 * The Windows port of the Rodent project.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _RODENT_APP_H
#define _RODENT_APP_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stdafx.h"
#include "AboutDialog.h"
#include "GopherWrapper.h"

// Instance operators.
ATOM RegisterApplication(HINSTANCE hInstance);
HWND InitializeInstance(HINSTANCE hInstance, LPTSTR lpCmdLine, int nCmdShow);
int TerminateInstance(HINSTANCE hInstance, int nDefRC);

// Window procedure.
LRESULT CALLBACK MainWindowProc(HWND hWnd, UINT wMsg, WPARAM wParam,
								LPARAM lParam);

// Window message handlers.
LRESULT WndMainCreate(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
LRESULT WndMainCommand(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
LRESULT WndMainNotify(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
LRESULT WndMainClose(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
LRESULT WndMainDestroy(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);

#endif // _RODENT_APP_H
