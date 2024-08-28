//
//  GopherClient.m
//  MacRodentPPC
//
//  Created by Nathan Campos on 2024-08-28.
//  Copyright 2024 Innove Workshop. All rights reserved.
//

#import "GopherClient.h"

@implementation GopherClient

///------------------------
/// @name Memory Management
///------------------------

/**
 Default initializer.
 */
- (id)init
{
	self = [super init];
	if (self) {
		// Allocate and initialize private objects.
		gopher_dir = NULL;
		_items = [[NSMutableArray alloc] init];
	}
	return self;
}

/**
 Ensures that all allocated objects are properly released.
 */
- (void)dealloc
{
	// Release all our internal objects.
	//[[self obj] release];
	
	// Release C pointers.
	if (gopher_dir)
		gopher_dir_free(gopher_dir, RECURSE_FORWARD | RECURSE_BACKWARD, 1);
	
	// Deallocate ourselves.
	[super dealloc];
}

///-----------------
/// @name Operations
///-----------------

- (void)navigateToAddress:(NSURL *)url
{
	int ret;

	// Parse the address.
	gopher_addr_t *addr = gopher_addr_parse([[url absoluteString] UTF8String]);
	if (addr == NULL) {
		NSLog(@"invalid address %@", url);
		@throw [NSException exceptionWithName:@"RDTInvalidAddress"
									   reason:[NSString stringWithFormat:@"Invalid address: %@", url]
									 userInfo:nil];
	}
	
	// Connect to the server.
	NSLog(@"connecting to %@", url);
	ret = gopher_connect(addr);
	if (ret != 0) {
		NSLog(@"failed to connect (%d)", ret);
		gopher_addr_free(addr);
		addr = NULL;
		@throw [NSException exceptionWithName:@"RDTConnectionFailed"
									   reason:[NSString stringWithFormat:@"Failed to connect to server"]
									 userInfo:nil];
	}
	
	// Request directory.
	ret = gopher_dir_request(addr, &gopher_dir);
	if (ret != 0) {
		NSLog(@"failed to request directory (%d)", ret);
		gopher_addr_free(addr);
		addr = NULL;
		@throw [NSException exceptionWithName:@"RDTDirectoryRequestFailed"
									   reason:[NSString stringWithFormat:@"Failed to request directory from %@", url]
									 userInfo:nil];
	}
	
	// Gracefully disconnect from server.
	ret = gopher_disconnect(addr);
	if (ret != 0)
		NSLog(@"failed to disconnect from server");
	
	/* Print out every item from the directory. */
	if (gopher_dir->items_len > 0) {
		gopher_item_t *item = gopher_dir->items;
		do {
			//gopher_item_print(item);
			[_items addObject:[[NSString alloc] initWithUTF8String:item->label]];
			item = item->next;
		} while (item != NULL);
	} else {
		NSLog(@"empty directory");
	}
	
	// Shame non-compliant servers.
	if (gopher_dir->err_count > 0) {
		NSLog(@"%u errors encountered while parsing the server's output",
			  gopher_dir->err_count);
	}
}

///--------------------------
/// @name Getters and Setters
///--------------------------

- (gopher_dir_t *)directory
{
	return gopher_dir;
}

- (NSMutableArray *)items
{
	return _items;
}

@end
