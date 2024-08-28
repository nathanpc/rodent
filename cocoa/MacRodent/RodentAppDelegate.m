//
//  RodentAppDelegate.m
//  MacRodentPPC
//
//  Created by Nathan Campos on 2024-08-28.
//  Copyright 2024 Innove Workshop. All rights reserved.
//

#import "RodentAppDelegate.h"

@implementation RodentAppDelegate

///------------------------
/// @name Memory Management
///------------------------

- (id)init
{
	self = [super init];
	if (self) {
		// Allocate and initialize private objects.
		gopherClient = [[GopherClient alloc] init];
	}
	return self;
}

- (void)dealloc
{
	NSLog(@"AppDelegate: dealloc");
	[super dealloc];
}

///-------------------------
/// @name Application Events
///-------------------------

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    // Insert code here to initialize your application
	NSLog(@"application did finish launching: %@", aNotification);
}

- (void)awakeFromNib
{
	[gopherClient navigateToAddress:[NSURL URLWithString:
									 @"gopher://gopher.floodgap.com/1/"]];
	NSLog(@"awaken from nib");
}

///---------------------------
/// @name Table View Delegates
///---------------------------

- (NSInteger)numberOfRowsInTableView:(NSTableView *)aTableView
{
	if ([gopherClient directory] == NULL)
		return (NSInteger)0;

	return (NSInteger)[gopherClient directory]->items_len;
}

- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn
				row:(NSInteger)rowIndex
{
	if ([[aTableColumn identifier] isEqualToString:@"label"]) {
		return [[gopherClient items] objectAtIndex:rowIndex];
	} else {
		return nil;
	}
}

@end
