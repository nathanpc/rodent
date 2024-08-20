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
	this->m_cType = GOPHER_TYPE_UNKNOWN;
	this->init(addr, readOnly);
}

/**
 * Creates a read-only gopherspace address from an underlying address structure.
 *
 * @param addr Internal gopherspace address structure.
 */
Address::Address(const gopher_addr_t *addr) {
	this->m_cType = GOPHER_TYPE_UNKNOWN;
	this->init(const_cast<gopher_addr_t *>(addr), true);
}

/**
 * Creates a gopherspace address from an URI conforming to RFC 4266.
 *
 * @param uri Universal Resource Identifier of the gopherspace.
 */
Address::Address(tstring uri) {
	this->init(Address::from_url(uri, &this->m_cType), false);
}

/**
 * Deallocates and cleans up internal structures of the address object.
 */
Address::~Address() {
	if (this->connected())
		this->disconnect();

	if (!this->read_only())
		this->release();
}

/**
 * Initializes the object. Used to bypass the limitation of calling constructors
 * from another constructor in older C++ editions.
 *
 * @param addr     Internal gopherspace address structure.
 * @param readOnly The object DOES NOT own its internal structure?
 */
void Address::init(gopher_addr_t *addr, bool readOnly) {
	this->m_bReadOnly = readOnly;
	this->m_bConnected = false;
	this->m_addr = addr;
}

/**
 * Creates a gopherspace address structure from an URL.
 *
 * @warning This method dinamically allocates memory.
 *
 * @param url  Gopherspace URL string.
 * @param type Optional. Stores the parsed Gopher entry item type character.
 *
 * @return Gopherspace address structure allocated based on the provided URL.
 *
 * @see gopher_addr_free
 */
gopher_addr_t *Address::from_url(const TCHAR *url, gopher_type_t *type) {
#ifdef UNICODE
	// Convert the Unicode string to multi-byte.
	char *szURL = win_wcstombs(url);
	if (szURL == NULL)
		throw std::exception("Failed to convert URL to multi-byte string");
#else
	const char *szURL = url;
#endif // UNICODE

	// Try to parse the supplied URL.
	gopher_addr_t *addr = gopher_addr_parse(szURL, type);
	if (addr == nullptr) {
		throw std::exception("Failed to parse URL into gopherspace address "
			"object");
	}

#ifdef UNICODE
	// Free the temporary buffer.
	free(szURL);
#endif // UNICODE

	return addr;
}

/**
 * Creates a gopherspace address structure from an URL.
 *
 * @warning This method dinamically allocates memory.
 *
 * @param url  Gopherspace URL string.
 * @param type Optional. Stores the parsed Gopher entry item type character.
 *
 * @return Gopherspace address structure allocated based on the provided URL.
 *
 * @see gopher_addr_free
 */
gopher_addr_t *Address::from_url(tstring url, gopher_type_t *type) {
	return from_url(url.c_str(), type);
}

/**
 * Converts an addres structure into a string URL representation of it.
 *
 * @warning This method dinamically allocates memory.
 *
 * @param addr Gopherspace address structure.
 * @param type Gopher entry item type character.
 *
 * @return URL string representing the address structure.
 */
TCHAR *Address::as_url(const gopher_addr_t *addr, gopher_type_t type) {
#ifdef UNICODE
	char *mb = gopher_addr_str(addr, type);
	TCHAR *uni = win_mbstowcs(mb);
	free(mb);
	mb = NULL;

	return uni;
#else
	return gopher_addr_str(addr, type);
#endif // UNICODE
}

/**
 * Converts an addres object into a string URL representation of it.
 *
 * @warning This method dinamically allocates memory.
 *
 * @return URL string representing the address object.
 */
TCHAR *Address::to_url() const {
	return Address::as_url(this->m_addr, this->m_cType);
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
		this->release();
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
void Address::release() {
	if (this->read_only())
		throw std::exception("Can't free a read-only gopher item");

	if (this->m_addr != NULL) {
		gopher_addr_free(this->m_addr);
		this->m_addr = NULL;
	}
}

/**
 * Checks if the gopherspace address structure has a parent selector.
 *
 * @param addr Internal gopherspace address structure to check.
 *
 * @return TRUE if there's a parent to the selector. FALSE if it's top-level.
 *
 * @see parent
 */
bool Address::has_parent(const gopher_addr_t *addr) {
	int ret = gopher_addr_up(NULL, addr);
	if (ret == -1) {
		// Build up the exception message and throw exception.
		std::string msg("Failed to check for parent selector: ");
		msg += strerror(ret);
		throw std::exception(msg.c_str());
	}

	return ret > 0;
}

/**
 * Checks if the address has a parent selector.
 *
 * @return TRUE if there's a parent to the selector. FALSE if it's top-level.
 *
 * @see parent
 */
bool Address::has_parent() const {
	return Address::has_parent(this->m_addr);
}

/**
 * Gets the parent selector of a gopherspace address structure.
 *
 * @warning This function dinamically allocates memory.
 *
 * @param addr Internal gopherspace address structure.
 *
 * @return Gopherspace address structure pointing to the parent selector or NULL
 *         if one wasn't found.
 *
 * @see has_parent
 */
gopher_addr_t *Address::parent(const gopher_addr_t *addr) {
	gopher_addr_t *parent;

	// Try to get a parent selector.
	int ret = gopher_addr_up(&parent, addr);
	if (ret == -1) {
		// Build up the exception message and throw exception.
		std::string msg("Failed to get parent selector: ");
		msg += strerror(ret);
		throw std::exception(msg.c_str());
	}

	return parent;
}

/**
 * Gets the parent selector gopherspace address structure of the object.
 *
 * @warning This function dinamically allocates memory.
 *
 * @return Gopherspace address structure pointing to the parent selector or NULL
 *         if one wasn't found.
 *
 * @see has_parent
 */
gopher_addr_t *Address::parent() const {
	return Address::parent(this->m_addr);
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
		free(this->m_label);
		this->m_item = NULL;
	}
}

/**
 * Converts an item address into a string URL representation of it.
 *
 * @warning This method dinamically allocates memory.
 *
 * @return URL string representing the address of the item.
 */
TCHAR *Item::to_url() const {
	return Address::as_url(this->m_item->addr, this->m_item->type);
}

/**
 * Notifies this object of changes made to the internal item structure.
 *
 * @param force Forcefully update ourselves.
 */
void Item::notify(bool force) {
	if (force || this->label_replicated()) {
		// Check if this isn't an invalid item.
		if (this->m_item == nullptr)
			throw std::exception("Using blank Item objects is prohibted");

#ifdef UNICODE
		this->m_label = win_mbstowcs(this->m_item->label);
#else
		this->m_label = this->m_item->label;
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
TCHAR *Item::label() {
	if (!this->label_replicated())
		this->notify(true);

	return this->m_label;
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
 * Creates a Gopher directory object and requests the directory from the Gopher
 * server. This object will manage all the dynamically allocated memory, when
 * out of scope we'll clean everything up.
 *
 * @param goaddr Gopherspace address structure. (Fully managed by us, including
 *               freeing when needed.)
 */
Directory::Directory(gopher_addr_t *goaddr) {
	gopher_dir_t *dir = NULL;
	int ret;

	// Connect to the server.
	ret = gopher_connect(goaddr);
	if (ret != 0) {
		// Build up the exception message.
		std::string msg("Failed to connect to server: ");
		msg += strerror(ret);

		// Free the address structure and throw the exception.
		gopher_addr_free(goaddr);
		throw std::exception(msg.c_str());
	}

	// Get directory from address.
	ret = gopher_dir_request(goaddr, &dir);
	if (ret != 0) {
		// Build up the exception message.
		std::string msg("Failed to request directory: ");
		msg += strerror(ret);

		// Free the address structure and throw the exception.
		gopher_addr_free(goaddr);
		throw std::exception(msg.c_str());
	}

	// Gracefully disconnect from the server.
	ret = gopher_disconnect(goaddr);
	if (ret != 0)
		perror("Failed to disconnect");

	// Finally initialize the object.
	this->init(dir, nullptr, true);
}

/**
 * Requests a directory from the Gopher server.
 *
 * @warning This object won't free the internal directory structure when it goes
 *          out of scope. Call Directory::free for that.
 *
 * @param addr Gopherspace address object. (Won't be free'd by us)
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
	this->init(dir, addr, false);
}

/**
 * Creates a Gopher directory acessor object.
 *
 * @warning This object won't free the internal directory structure when it goes
 *          out of scope. Call Directory::free for that.
 *
 * @param dir Internal Gopher directory structure.
 *
 * @see Directory::free
 */
Directory::Directory(gopher_dir_t *dir) {
	this->init(dir, nullptr, false);
}

/**
 * Deallocates and cleans up internal objects. If we are the owner of the
 * internal structures it'll also free recursively forward other directories.
 *
 * @see Directory::free
 */
Directory::~Directory() {
	if (this->m_bOwner) {
		this->release(RECURSE_FORWARD);
	} else if (this->m_items != nullptr) {
		delete this->m_items;
		this->m_items = nullptr;
	}
}

/**
 * Initializes the values of the object.
 *
 * @param dir   Gopher directory structure.
 * @param addr  Gopherspace address structure.
 * @param owner Are we the sole owners of all structures? Are we the ones that
 *              should free them completely?
 */
void Directory::init(gopher_dir_t *dir, Address *addr, bool owner) {
	this->m_dir = dir;
	this->m_items = nullptr;
	this->m_addr = addr;
	this->m_bOwner = owner;
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
 * Retrieves the directory of another gopherhole and pushes it into the history
 * stack.
 *
 * @param goaddr Gopherspace address structure. Will be managed by the library,
 *               do not free after passing it into this method.
 *
 * @return Directory object retrieved from the requested server.
 */
Directory *Directory::push(gopher_addr_t *goaddr) {
	// Retrieve new directory.
	Directory *dir = new Directory(goaddr);
	dir->set_ownership(false);

	// Free everything that should no longer be in the history.
	this->release(RECURSE_FORWARD, false);
	
	// Ensure the history stack link is preserved.
	dir->set_prev(this);
	this->set_next(dir);

	return dir;
}

/**
 * Sets the previous directory pointer in the history stack.
 *
 * @param dir New previous directory.
 */
void Directory::set_prev(Directory *dir) {
	this->m_dir->prev = const_cast<gopher_dir_t *>(dir->c_dir());
}

/**
 * Sets the next directory pointer in the history stack.
 *
 * @param dir New next directory.
 */
void Directory::set_next(Directory *dir) {
	this->m_dir->next = const_cast<gopher_dir_t *>(dir->c_dir());
}

/**
 * Gets the previous directory in the browser stack.
 *
 * @return Previous directory object.
 *
 * @see Directory::has_prev
 */
Directory *Directory::prev() const {
	return new Directory(this->m_dir->prev);
}

/**
 * Gets the next directory in the browser stack.
 *
 * @return Next directory object.
 *
 * @see Directory::has_next
 */
Directory *Directory::next() const {
	return new Directory(this->m_dir->next);
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
 * Checks if the directory has a parent.
 *
 * @return TRUE if the directory has a parent, FALSE otherwise.
 *
 * @see Address::has_parent
 */
bool Directory::has_parent() const {
	return Address::has_parent(this->m_dir->addr);
}

/**
 * Gets parent of the directory.
 *
 * @warning This function dinamically allocates memory.
 *
 * @return Gopherspace address structure pointing to the parent selector or NULL
 *         if one wasn't found.
 *
 * @see Address::has_parent
 */
gopher_addr_t *Directory::parent() const {
	return Address::parent(this->m_dir->addr);
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
 * Sets the ownership of the internal structures.
 *
 * @param owner Are we supposed to own the internal structures and free them
 *              when the destructor gets called?
 */
void Directory::set_ownership(bool owner) {
	this->m_bOwner = owner;
}

/**
 * Frees the internal structures held by this object.
 *
 * @param recurse   Bitwise field to recursively free its history stack.
 * @param inclusive Should we also free our own internal directory pointer?
 */
void Directory::release(gopher_recurse_dir_t recurse, bool inclusive) {
	// Free underlying structure.
	if (this->m_dir != nullptr) {
		gopher_dir_free(this->m_dir, recurse, inclusive);
		if (inclusive)
			this->m_dir = nullptr;
	}

	// Stop if we shouldn't free ourselves.
	if (!inclusive)
		return;

	// Free cached items.
	if (this->m_items != nullptr) {
		delete this->m_items;
		this->m_items = nullptr;
	}

	// Check if we need to propagate this to relying address object.
	if (this->m_addr != nullptr) {
		// Needed since gopher_dir_free() frees the socket.
		m_addr->invalidate();
		m_addr->disconnect();

		// Should we also free the address?
		if (this->m_bOwner) {
			delete m_addr;
			m_addr = nullptr;
		}
	}
}

/**
 * Frees the internal structures held by this object.
 *
 * @param recurse Bitwise field to recursively free its history stack.
 */
void Directory::release(gopher_recurse_dir_t recurse) {
	this->release(recurse, true);
}

/**
 * Frees the internal structures held by this object.
 *
 * @param recurse_flags Bitwise field to recursively free its history stack.
 */
void Directory::release(int recurse_flags) {
	this->release(static_cast<gopher_recurse_dir_t>(recurse_flags));
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

/*
 * +===========================================================================+
 * |                                                                           |
 * |                              File Downloads                               |
 * |                                                                           |
 * +===========================================================================+
 */

/**
 * Creates a blank downloaded file object.
 */
FileDownload::FileDownload() {
	this->m_gfile = NULL;
	this->m_fpath = NULL;
	this->m_bname = NULL;
}

/**
 * Creates a downloaded file object.
 *
 * @param gfile Internal Gopher file download structure. Will be managed by
 *              this object.
 */
FileDownload::FileDownload(gopher_file_t *gfile) {
	this->m_gfile = gfile;
	this->m_fpath = NULL;
	this->m_bname = NULL;
}

/**
 * Deallocates and cleans up internal objects.
 */
FileDownload::~FileDownload() {
	if (this->m_gfile)
		gopher_file_free(this->m_gfile);
	if (this->m_fpath)
		free(this->m_fpath);
	if (this->m_bname)
		free(this->m_bname);
}

/**
 * Sets up a download to a specific file path.
 *
 * @param goaddr Gopherspace address structure. Will NOT be managed by the
 *               object, you are responsible for freeing this.
 * @param hint   Hint at the type of file we may be dealing with.
 * @param fpath  Path to where the downloaded file should be written.
 *
 * @see setup_default
 * @see setup_temp
 */
void FileDownload::setup(gopher_addr_t *goaddr, gopher_type_t hint,
						 const TCHAR *fpath) {
	// Basic copying of constant data.
	this->m_fpath = _tcsdup(fpath);
	this->basename();

	// Create internal file download structure.
#ifdef UNICODE
	char *mbfpath = win_wcstombs(fpath);
	this->m_gfile = gopher_file_new(goaddr, mbfpath, hint);
	free(mbfpath);
	mbfpath = NULL;
#else
	this->m_gfile = gopher_file_new(goaddr, fpath, hint);
#endif // UNICODE
}

/**
 * Sets up a temporary file transfer.
 *
 * @param goaddr Gopherspace address structure. Will NOT be managed by the
 *               object, you are responsible for freeing this.
 * @param hint   Hint at the type of file we may be dealing with.
 *
 * @see setup
 * @see setup_default
 */
void FileDownload::setup_temp(gopher_addr_t *goaddr, gopher_type_t hint) {
	tstring strFilePath;

	// Get temporary folder path.
	DWORD dwLen = GetTempPath(0, NULL);
	TCHAR *szTempPath = (TCHAR *)malloc((dwLen + 1) * sizeof(TCHAR));
	if (szTempPath == NULL)
		throw std::exception("Failed to allocate temporary folder string");
	GetTempPath(dwLen + 1, szTempPath);
	strFilePath = szTempPath;
	free(szTempPath);
	szTempPath = NULL;

	// Get file basename.
	char *szmbBasename = gopher_file_basename(goaddr);

	// Build up a path to the actual temporary file path.
#ifdef UNICODE
	this->m_bname = win_mbstowcs(szmbBasename);
	strFilePath += this->m_bname;
	free(szmbBasename);
	szmbBasename = NULL;
#else
	this->m_bname = szmbBasename;
	strFilePath += szmbBasename;
#endif // UNICODE

	// Perform the actual setup.
	this->setup(goaddr, hint, strFilePath.c_str());
}

/**
 * Download the file from the specified gopherspace address.
 *
 * @see setup
 */
void FileDownload::download() {
	int ret;

	// Check if we have set things up.
	if (this->m_gfile == NULL)
		throw std::exception("Can't download a file without setting up first");

	// Connect to the server.
	ret = gopher_connect(this->m_gfile->addr);
	if (ret != 0) {
		// Build up the exception message.
		std::string msg("Failed to connect to server: ");
		msg += strerror(ret);
		throw std::exception(msg.c_str());
	}

	// Download file from address.
	ret = gopher_file_download(this->m_gfile);
	if (ret != 0) {
		// Build up the exception message.
		std::string msg("Failed to request directory: ");
		msg += strerror(ret);
		throw std::exception(msg.c_str());
	}

	// Gracefully disconnect from the server.
	ret = gopher_disconnect(this->m_gfile->addr);
	if (ret != 0)
		perror("Failed to disconnect");
}

/**
 * Sets up a callback to listen for reports of transferred bytes while
 * downloading.
 *
 * @param func Callback function.
 * @param arg  Optional. Parameter to be passed to the callback function.
 */
void FileDownload::set_transfer_cb(gopher_file_transfer_func func, void *arg) {
	gopher_file_set_transfer_cb(this->m_gfile, func, arg);
}

/**
 * Gets the basename from an address structure and caches the basename.
 *
 * @param addr Gopherspace address structure to extract basename from selector.
 *
 * @return Basename of the requested file in the address structure.
 */
const TCHAR *FileDownload::basename(const gopher_addr_t *addr) {
	// Dispose of any previous basename.
	if (this->m_bname)
		free(this->m_bname);

	// Get the basename from address selector.
	char *bname = gopher_file_basename(addr);
#ifdef UNICODE
	this->m_bname = win_mbstowcs(bname);
	free(bname);
	bname = NULL;
#else
	this->m_bname = bname;
#endif // UNICODE

	return this->m_bname;
}

/**
 * Gets the basename from the internal requested address structure.
 *
 * @return Basename of the requested file in the address structure.
 */
const TCHAR *FileDownload::basename() {
	// Get the basename if needed.
	if (this->m_bname == NULL) {
		// Check if we have an address structure to get the basename from.
		if ((this->m_gfile == NULL) || (this->m_gfile->addr == NULL))
			throw std::exception("Can't get basename without setting up first");

		// Get the basename.
		return this->basename(this->m_gfile->addr);
	}

	return this->m_bname;
}

/**
 * Gets the path where the file was downloaded to.
 *
 * @return Path to where the downloaded file is.
 */
const TCHAR *FileDownload::path() {
	// Convert the path to Unicode if needed.
	if (this->m_fpath == NULL)
		this->m_fpath = win_mbstowcs(this->m_gfile->fpath);

	return const_cast<const TCHAR *>(this->m_fpath);
}

/**
 * Gets the size of the downloaded file.
 *
 * @return Size of the downloaded file in bytes.
 */
size_t FileDownload::size() const {
	return this->m_gfile->fsize;
}

/**
 * Gets the internal gopherspace address structure.
 *
 * @return Internal gopherspace address structure.
 */
const gopher_addr_t *FileDownload::c_addr() const {
	return const_cast<const gopher_addr_t *>(this->m_gfile->addr);
}

/**
 * Gets the internal file download structure.
 *
 * @return Internal file download structure.
 */
const gopher_file_t *FileDownload::c_file() const {
	return const_cast<const gopher_file_t *>(this->m_gfile);
}
