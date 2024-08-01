/**
 * GopherWrapper.h
 * A C++ wrapper for our Gopher protocol implementation.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#ifndef _WINRODENT_GOPHER_H
#define _WINRODENT_GOPHER_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stdafx.h"

#include <gopher.h>
#include <vector>

namespace Gopher {

/**
 * Gopherspace address.
 */
class Address {
private:
	gopher_addr_t *m_addr;
	gopher_type_t m_cType;
	bool m_bReadOnly;
	bool m_bConnected;

	void init(gopher_addr_t *addr, bool readOnly);

public:
	Address(gopher_addr_t *addr, bool readOnly);
	Address(const gopher_addr_t *addr);
	Address(tstring uri);
	virtual ~Address();
	
	static gopher_addr_t *from_url(const TCHAR *url, gopher_type_t *type);
	static gopher_addr_t *from_url(tstring url, gopher_type_t *type);
	static TCHAR *as_url(const gopher_addr_t *addr, gopher_type_t type);
	TCHAR *to_url() const;

	void connect();
	void disconnect();

	static Address replicate(const gopher_addr_t *addr);
	void invalidate();
	void free();

	static bool has_parent(const gopher_addr_t *addr);
	bool has_parent() const;
	static gopher_addr_t *parent(const gopher_addr_t *addr);
	gopher_addr_t *parent() const;

	bool connected() const;
	bool read_only() const;
	const gopher_addr_t *c_addr() const;
};

/**
 * Gopher directory item acessor.
 */
class Item {
private:
	const gopher_item_t *m_item;
	TCHAR *m_label;

	bool label_replicated() const;

public:
	Item();
	Item(const gopher_item_t *item);
	virtual ~Item();

	TCHAR *to_url() const;

	void notify(bool force);

	gopher_type_t type() const;
	TCHAR *label();
	const gopher_item_t *c_item() const;
};

/**
 * Gopher directory.
 */
class Directory {
private:
	gopher_dir_t *m_dir;
	std::vector<Item> *m_items;
	Address *m_addr;
	bool m_bOwner;

	void init(gopher_dir_t *dir, Address *addr, bool owner);
	void replicate_items();

public:
	Directory(gopher_addr_t *goaddr);
	Directory(Address *addr);
	Directory(gopher_dir_t *dir);
	virtual ~Directory();

	void set_ownership(bool owner);
	void free(gopher_recurse_dir_t recurse, bool inclusive);
	void free(gopher_recurse_dir_t recurse);
	void free(int recurse_flags);

	Directory *push(gopher_addr_t *goaddr);
	void set_prev(Directory *dir);
	void set_next(Directory *dir);
	Directory *prev() const;
	Directory *next() const;
	bool has_prev() const;
	bool has_next() const;
	
	bool has_parent() const;
	gopher_addr_t *parent() const;

	const std::vector<Item> *items();

	size_t items_count() const;
	uint16_t error_count() const;
	const gopher_dir_t *c_dir() const;
};

/**
 * Gopher file download.
 */
class FileDownload {
private:
	gopher_file_t *m_gfile;
	TCHAR *m_fpath;

public:
	FileDownload();
	FileDownload(gopher_file_t *gfile);
	virtual ~FileDownload();

	void download(gopher_addr_t *goaddr, gopher_type_t hint,
				  const TCHAR *fpath);

	const TCHAR *path();
	size_t size() const;
	const gopher_file_t *c_file() const;
};

};

#endif // _WINRODENT_GOPHER_H
