//
//  RodentAppDelegate.m
//  MacRodentPPC
//
//  Created by Nathan Campos on 2024-08-28.
//  Copyright 2024 Innove Workshop. All rights reserved.
//

#import "RodentAppDelegate.h"

@implementation RodentAppDelegate

- (id)init
{
	self = [super init];
	if (self) {
		// Alloc and init private objects.
	}
	return self;
}

- (void)dealloc
{
	NSLog(@"AppDelegate: dealloc");
	[super dealloc];
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    // Insert code here to initialize your application
	NSLog(@"application did finish launching: %@", aNotification);
}

- (void)awakeFromNib
{
	NSLog(@"awaken from nib");
}

@end
