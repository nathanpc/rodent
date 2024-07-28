/**
 * GopherWrapper.cpp
 * A C++ wrapper for our Gopher protocol implementation.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "GopherWrapper.h"

using namespace Gopher;

/*
 * +===========================================================================+
 * |                                                                           |
 * |                          Gopherspace Addressing                           |
 * |                                                                           |
 * +===========================================================================+
 */

/**
 * Creates a gopherspace address from an underlying address structure.
 *
 * @param addr Internal gopherspace address structure.
 * @param readOnly The object DOES NOT own its internal structure?
 */
Address::Address(gopher_addr_t *addr, bool readOnly) {
	this->init(addr, readOnly);
}

/**
 * Creates a read-only gopherspace address from an underlying address structure.
 *
 * @param addr Internal gopherspace address structure.
 */
Address::Address(const gopher_addr_t *addr) {
	this->init(const_cast<gopher_addr_t *>(addr), true);
}

/**
 * Creates a gopherspace address from an URI conforming to RFC 4266.
 *
 * @param uri Universal Resource Identifier of the gopherspace.
 */
Address::Address(tstring uri) {
#ifdef UNICODE
	// Convert the Unicode string to multi-byte.
	char *szURI = win_wcstombs(uri.c_str());
	if (szURI == NULL)
		throw std::exception("Failed to convert URI to multi-byte string");
#else
	const char *szURI = uri.c_str();
#endif // UNICODE

	// Try to parse the supplied URI.
	this->init(gopher_addr_parse(szURI), false);
	if (this->m_addr == nullptr) {
		throw std::exception("Failed to parse URI into gopherspace address "
			"object");
	}

#ifdef UNICODE
	std::free(szURI);
#endif // UNICODE
}

/**
 * Deallocates and cleans up internal structures of the address object.
 */
Address::~Address() {
	if (this->connected())
		this->disconnect();

	if (!this->read_only())
		this->free();
}

/**
 * Initializes the object. Used to bypass the limitation of calling constructors
 * from another constructor in older C++ editions.
 *
 * @param addr Internal gopherspace address structure.
 * @param readOnly The object DOES NOT own its internal structure?
 */
void Address::init(gopher_addr_t *addr, bool readOnly) {
	this->m_bReadOnly = readOnly;
	this->m_bConnected = false;
	this->m_addr = addr;
}

/**
 * Establishes a connection with a Gopher server.
 */
void Address::connect() {
	int ret = gopher_connect(this->m_addr);
	if (ret != 0) {
		std::string msg("Failed to connect to server: ");
		msg += strerror(ret);
		throw std::exception(msg.c_str());
	}

	this->m_bConnected = true;
}

/**
 * Disconnects gracefully from a Gopher server.
 */
void Address::disconnect() {
	if (this->connected()) {
		gopher_disconnect(this->m_addr);
		this->m_bConnected = false;
		this->free();
	}
}

/**
 * Duplicates the internal gopherspace address structure and creates an owning
 * object from a read-only structure.
 *
 * @param addr Internal gopherspace address structure.
 *
 * @return Gopherspace address object that owns its internal address structure.
 */
Address Address::replicate(const gopher_addr_t *addr) {
	gopher_addr_t *addrCopy;

	// Create a copy of our structure.
	addrCopy = gopher_addr_new(addr->host, addr->port, addr->selector);
	addrCopy->ipaddr = (sockaddr_in *)malloc(addr->ipaddr_len);
	memcpy(addrCopy->ipaddr, addr->ipaddr, addr->ipaddr_len);
	addrCopy->ipaddr_len = addr->ipaddr_len;
	addrCopy->sockfd = (SOCKET)(~0);

	return Address(addrCopy, false);
}

/**
 * Invalidates the internal address structure pointer. Used when this pointer
 * was free'd elsewhere and this change must be reflected here.
 *
 * @warning This method doesn't perform any checks and will blindly set the
 *          internal structure pointer to NULL, possibly leaking memory.
 */
void Address::invalidate() {
	this->m_addr = NULL;
}

/**
 * Frees the internal structure help by this object.
 */
void Address::free() {
	if (this->read_only())
		throw std::exception("Can't free a read-only gopher item");

	if (this->m_addr != NULL) {
		gopher_addr_free(this->m_addr);
		this->m_addr = NULL;
	}
}

/**
 * Check if there's an connection with the Gopher server.
 *
 * @return Are we connected to a Gopher server?
 */
bool Address::connected() const {
	return this->m_bConnected;
}

/**
 * Checks if the internal structures are read-only. Meaning we are simply acting
 * as an acessor.
 *
 * @returns TRUE if we do not own the internal structure.
 */
bool Address::read_only() const {
	return this->m_bReadOnly;
}

/**
 * Gets the internal gopherspace address structure.
 *
 * @return Internal gopherspace address structure.
 */
const gopher_addr_t *Address::c_addr() const {
	return const_cast<const gopher_addr_t *>(this->m_addr);
}

/*
 * +===========================================================================+
 * |                                                                           |
 * |                                 Item Line                                 |
 * |                                                                           |
 * +===========================================================================+
 */

/**
 * Empty item acessor object.
 *
 * @warning DO NOT USE. Only here for vector declarations.
 */
Item::Item() {
	this->m_item = nullptr;
	this->m_label = nullptr;
}

/**
 * Creates an item acessor object.
 *
 * @param item Internal Gopher item structure.
 */
Item::Item(const gopher_item_t *item) {
	this->m_item = item;
	this->m_label = nullptr;
}

/**
 * Deallocates and cleans up internal objects.
 */
Item::~Item() {
	if (this->label_replicated()) {
		delete this->m_label;
		this->m_item = NULL;
	}
}

/**
 * Notifies this object of changes made to the internal item structure.
 *
 * @param force Forcefully update ourselves.
 */
void Item::notify(bool force) {
	if (force || this->label_replicated()) {
		TCHAR *label;

		// Check if this isn't an invalid item.
		if (this->m_item == nullptr)
			throw std::exception("Using blank Item objects is prohibted");

#ifdef UNICODE
		label = win_mbstowcs(this->m_item->label);
		this->m_label->assign(label);
		free(label);
#else
		this->m_label->assign(this->m_item->label);
#endif // UNICODE
	}
}

/**
 * Gets the Gopher type identifier of the item.
 *
 * @return Gopher type identifier.
 */
gopher_type_t Item::type() const {
	return this->m_item->type;
}

/**
 * Gets the label of the item.
 *
 * @return Label of the referenced item.
 */
const TCHAR *Item::label() {
	if (!this->label_replicated())
		this->notify(true);

	return this->m_label->c_str();
}

/**
 * Gets the internal item structure.
 *
 * @return Internal Gopher item structure.
 */
const gopher_item_t *Item::c_item() const {
	return this->m_item;
}

/**
 * Checks if we have created a copy of the label internally.
 *
 * @return TRUE if we have made a copy of the label.
 */
bool Item::label_replicated() const {
	return this->m_label != nullptr;
}

/*
 * +===========================================================================+
 * |                                                                           |
 * |                            Directory Handling                             |
 * |                                                                           |
 * +===========================================================================+
 */

/**
 * Requests a directory from the Gopher server.
 *
 * @warning This object won't free the internal directory structure when it goes
 *          out of scope.
 *
 * @see Directory::free
 */
Directory::Directory(Address *addr) {
	gopher_dir_t *dir = NULL;

	// Check if we are connected.
	if (!addr->connected()) {
		throw std::exception("Cannot retrieve directory if not previously "
			"connected");
	}

	// Request the directory.
	this->m_items = nullptr;
	int ret = gopher_dir_request(const_cast<gopher_addr_t *>(addr->c_addr()),
		&dir);
	if (ret != 0) {
		std::string msg("Failed to request directory: ");
		msg += strerror(ret);
		throw std::exception(msg.c_str());
	}

	// Finally initialize the object.
	this->init(dir, addr);
}

/**
 * Creates a Gopher directory acessor object.
 *
 * @warning This object won't free the internal directory structure when it goes
 *          out of scope.
 *
 * @param dir Internal Gopher directory structure.
 *
 * @see Directory::free
 */
Directory::Directory(gopher_dir_t *dir) {
	this->init(dir, nullptr);
}

/**
 * Deallocates and cleans up internal objects.
 */
Directory::~Directory() {
	if (this->m_items != nullptr) {
		delete this->m_items;
		this->m_items = nullptr;
	}
}

void Directory::init(gopher_dir_t *dir, Address *addr) {
	this->m_dir = dir;
	this->m_items = nullptr;
	this->m_addr = addr;
}

/**
 * Replicates the internal items linked list in a C++ fashion.
 */
void Directory::replicate_items() {
	if (this->m_items != nullptr)
		delete this->m_items;
	this->m_items = new std::vector<Item>;
	this->m_items->reserve(this->items_count());

	// Populate the vector.
	gopher_item_t *item = this->m_dir->items;
	do {
		this->m_items->push_back(Item(item));
		item = item->next;
	} while (item != NULL);
}

/**
 * Gets the previous directory in the browser stack.
 *
 * @return Previous directory object.
 *
 * @see Directory::has_prev
 */
Directory Directory::prev() const {
	return Directory(this->m_dir->prev);
}

/**
 * Gets the next directory in the browser stack.
 *
 * @return Next directory object.
 *
 * @see Directory::has_next
 */
Directory Directory::next() const {
	return Directory(this->m_dir->next);
}

/**
 * Checks if there's a previous directory object in the browser stack.
 *
 * @return TRUE if there's a previous directory in the stack.
 */
bool Directory::has_prev() const {
	return this->m_dir->prev != NULL;
}

/**
 * Checks if there's a next directory object in the browser stack.
 *
 * @return TRUE if there's a next directory in the stack.
 */
bool Directory::has_next() const {
	return this->m_dir->next != NULL;
}

/**
 * Gets the list of items inside this directory object.
 *
 * @return List of items inside the directory.
 */
const std::vector<Item> *Directory::items() {
	// Build up the items list replica.
	if (this->m_items == nullptr)
		this->replicate_items();

	return const_cast<const std::vector<Item> *>(this->m_items);
}

/**
 * Frees the internal structure help by this object.
 *
 * @param recurse Bitwise field to recursively free its history stack.
 */
void Directory::free(gopher_recurse_dir_t recurse) {
	// Free underlying structure.
	gopher_dir_free(this->m_dir, recurse, 1);
	this->m_dir = NULL;

	// Free cached items.
	delete this->m_items;
	this->m_items = nullptr;

	// Check if we need to propagate this to relying address object.
	if (this->m_addr != nullptr) {
		// Needed since gopher_dir_free() frees the socket.
		m_addr->invalidate();
		m_addr->disconnect();
	}
}

/**
 * Frees the internal structure help by this object.
 *
 * @param recurse_flags Bitwise field to recursively free its history stack.
 */
void Directory::free(int recurse_flags) {
	this->free(static_cast<gopher_recurse_dir_t>(recurse_flags));
}

/**
 * Gets the number of items inside this directory.
 *
 * @return Number of items inside this directory.
 */
size_t Directory::items_count() const {
	return this->m_dir->items_len;
}

/**
 * Gets the Number of parsing errors or non-conformities according to the RFC
 * encountered while parsing this directory.
 *
 * @return Number of parsing errors or non-conformities found while parsing.
 */
uint16_t Directory::error_count() const {
	return this->m_dir->err_count;
}

/**
 * Gets the underlying internal directory structure of the object.
 *
 * @return Directory structure.
 */
const gopher_dir_t *Directory::c_dir() const {
	return const_cast<const gopher_dir_t *>(this->m_dir);
}
