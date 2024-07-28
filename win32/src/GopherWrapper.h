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
	bool m_bReadOnly;
	bool m_bConnected;

	void init(gopher_addr_t *addr, bool readOnly);

public:
	Address(gopher_addr_t *addr, bool readOnly);
	Address(const gopher_addr_t *addr);
	Address(tstring uri);
	virtual ~Address();

	void connect();
	void disconnect();

	static Address replicate(const gopher_addr_t *addr);
	void invalidate();
	void free();

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
	tstring *m_label;

	bool label_replicated() const;

public:
	Item();
	Item(const gopher_item_t *item);
	virtual ~Item();

	void notify(bool force);

	gopher_type_t type() const;
	const TCHAR *label();
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

	void init(gopher_dir_t *dir, Address *addr);
	void replicate_items();

public:
	Directory(Address *addr);
	Directory(gopher_dir_t *dir);
	virtual ~Directory();
	void free(gopher_recurse_dir_t recurse);
	void free(int recurse_flags);

	Directory prev() const;
	Directory next() const;
	bool has_prev() const;
	bool has_next() const;

	const std::vector<Item> *items();

	size_t items_count() const;
	uint16_t error_count() const;
	const gopher_dir_t *c_dir() const;
};

};

#endif // _WINRODENT_GOPHER_H
