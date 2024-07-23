/**
 * Tester.cpp
 * A simple application to test the port of the librodent library.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "stdafx.h"
#ifdef DEBUG
#include <crtdbg.h>
#endif // DEBUG

#include "GopherWrapper.h"

/**
 * Application's main entry point.
 *
 * @param argc Number of command line arguments.
 * @param argv Command line arguments.
 *
 * @return Return code.
 */
int _tmain(int argc, TCHAR* argv[]) {
#ifdef DEBUG
	// Initialize memory leak detection.
	_CrtMemState snapBegin;
	_CrtMemState snapEnd;
	_CrtMemState snapDiff;
	_CrtMemCheckpoint(&snapBegin);
#endif // DEBUG

	Gopher::Address *addr = nullptr;
	WSADATA wsaData;
	int ret = 0;

	// Initialize Winsock stuff.
	WORD wVersionRequested = MAKEWORD(2, 2);
	if ((ret = WSAStartup(wVersionRequested, &wsaData)) != 0) {
		tcout << _T("WSAStartup failed with error ") << ret << _T('\r') <<
			std::endl;
		return 1;
	}

	tcout << _T("libgopher v") << LIBGOPHER_VER_STR << _T(" tester") <<
		_T('\r') << std::endl;
	try {
		// Use default address if none were supplied.
		if (argc < 2) {
			tcout << _T("No address was supplied, using floodgap's for ")
				_T("testing.") << _T('\r') << std::endl;
			addr = new Gopher::Address(_T("gopher://gopher.floodgap.com/1/overbite"));
		} else {
			// Parse Gopher URI from argument.
			addr = new Gopher::Address(argv[1]);
		}
	
		// Print information about the requested address.
		tcout << _T("Requesting ");
		gopher_addr_print(addr->c_addr());
	
		// Connect to the server and request directory.
		addr->connect();
		Gopher::Directory dir(addr);
	
		// Print out every item from the directory.
		if (dir.items_count() > 0) {
			for (size_t i = 0; i < dir.items_count(); i++)
				gopher_item_print(dir.items()->at(i).c_item());
		} else {
			tcout << _T("Empty directory.") << _T('\r') << std::endl;
		}
	
		// Shame non-compliant servers.
		if (dir.error_count() > 0) {
			tcout << dir.error_count() << _T(" errors encountered while ")
				_T("parsing the server's output") << _T('\r') << std::endl;
		}

		// Gracefully disconnect from the server and free resources.
		dir.free((gopher_recurse_dir_t)(RECURSE_BACKWARD | RECURSE_FORWARD));
	} catch (const std::exception& e) {
		std::cout << e.what() << std::endl;
		ret = 1;
	}

	// Free up any resources.
	delete addr;
	WSACleanup();

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
	system("pause");

	return ret;
}
