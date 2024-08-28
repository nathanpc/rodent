//
//  RodentAppDelegate.h
//  MacRodentPPC
//
//  Created by Nathan Campos on 2024-08-28.
//  Copyright 2024 Innove Workshop. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "GopherClient.h"

@interface RodentAppDelegate : NSObject {
	NSWindow *window;
	NSTableView *directoryView;
	
	GopherClient *gopherClient;
}

// Table View delegates.
- (NSInteger)numberOfRowsInTableView:(NSTableView *)aTableView;
- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn
			row:(NSInteger)rowIndex;

@end
