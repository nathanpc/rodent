//
//  GopherClient.h
//  MacRodentPPC
//
//  Created by Nathan Campos on 2024-08-28.
//  Copyright 2024 Innove Workshop. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "gopher.h"

@interface GopherClient : NSObject {
	NSMutableArray *_items;
	gopher_dir_t *gopher_dir;
}

// Operations
- (void)navigateToAddress:(NSURL *)url;

// Getters and setters.
- (gopher_dir_t *)directory;
- (NSMutableArray *)items;

@end
