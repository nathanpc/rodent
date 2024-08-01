/**
 * MsgBoxes.cpp
 * Some utility functions to work with Message Boxes more easily and write less
 * boilerplate for something that is so simple.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "MsgBoxes.h"

#include <tchar.h>

/**
 * Generic message box.
 * 
 * @param hwndParent Parent window's handle or NULL if it doesn't have one.
 * @param uType      Flags that control the buttons and the icon of the message box.
 * @param szTitle    Title of the message box dialog window.
 * @param szText     Descriptive text of the dialog box.
 * 
 * @return ID of the button that was clicked by the user.
 */
int MsgBox(HWND hwndParent, UINT uType, LPCTSTR szTitle, LPCTSTR szText) {
	return MessageBox(hwndParent, szText, szTitle, uType);
}

/**
 * Information message box.
 *
 * @param hwndParent Parent window's handle or NULL if it doesn't have one.
 * @param szTitle    Title of the message box dialog window.
 * @param szText     Descriptive text of the dialog box.
 *
 * @return ID of the button that was clicked by the user.
 */
int MsgBoxInfo(HWND hwndParent, LPCTSTR szTitle, LPCTSTR szText) {
	return MessageBox(hwndParent, szText, szTitle, MB_OK | MB_ICONINFORMATION);
}

/**
 * Warning message box.
 *
 * @param hwndParent Parent window's handle or NULL if it doesn't have one.
 * @param szTitle    Title of the message box dialog window.
 * @param szText     Descriptive text of the dialog box.
 *
 * @return ID of the button that was clicked by the user.
 */
int MsgBoxWarning(HWND hwndParent, LPCTSTR szTitle, LPCTSTR szText) {
	return MessageBox(hwndParent, szText, szTitle, MB_OK | MB_ICONWARNING);
}

/**
 * Error message box.
 * 
 * @param hwndParent Parent window's handle or NULL if it doesn't have one.
 * @param szTitle    Title of the message box dialog window.
 * @param szText     Descriptive text of the dialog box.
 *
 * @return ID of the button that was clicked by the user.
 */
int MsgBoxError(HWND hwndParent, LPCTSTR szTitle, LPCTSTR szText) {
	return MessageBox(hwndParent, szText, szTitle, MB_OK | MB_ICONERROR);
}

/**
 * Win32 last error message box.
 *
 * @param hwndParent Parent window's handle or NULL if it doesn't have one.
 *
 * @return ID of the button that was clicked by the user or 0 if no error was
 *         reported.
 */
int MsgBoxLastError(HWND hwndParent) {
	DWORD dwError;
	LPTSTR szError;
	int nRet;

	// Get the last error code.
	if ((dwError = GetLastError()) == 0)
		return 0;

	// Get the detailed description of the error.
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dwError,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&szError, 0, NULL);

	// Show the message box and clean up afterwards.
	nRet = MsgBoxError(hwndParent, _T("Win32 API Error"), szError);
	LocalFree(szError);

	return nRet;
}

/**
 * C++ exception message box.
 *
 * @param hwndParent Parent window's handle or NULL if it doesn't have one.
 * @param exc        C++ exception to be displayed to the user.
 * @param szTitle    Title of the message box dialog window.
 *
 * @return ID of the button that was clicked by the user.
 */
int MsgBoxException(HWND hwndParent, const std::exception& exc,
					LPCTSTR szTitle) {
#ifdef UNICODE
	// Get required buffer size and allocate some memory for it.
	int nLen = MultiByteToWideChar(CP_OEMCP, 0, exc.what(), -1, NULL, 0);
	if (nLen == 0)
		goto failure;
	TCHAR *szMessage = (TCHAR *)malloc(nLen * sizeof(TCHAR));
	if (szMessage == NULL) {
		throw std::exception("Failed to allocate memory for dialog exception "
			"message string");
	}

	// Perform the conversion.
	nLen = MultiByteToWideChar(CP_OEMCP, 0, exc.what(), -1, szMessage, nLen);
	if (nLen == 0) {
failure:
		MsgBoxError(hwndParent, _T("String Conversion Failure"),
			_T("Failed to convert UTF-8 string to UTF-16."));
		if (szMessage)
			free(szMessage);

		throw std::exception("Failed to convert message exception to wide "
			"string");
	}
#else
	const char *szMessage = e.what();
#endif // UNICODE

	// Finally display the error message box!
	MsgBoxError(hwndParent, szTitle, szMessage);

#ifdef UNICODE
	// Free the temporary buffer.
	free(szMessage);
#endif // UNICODE
}
