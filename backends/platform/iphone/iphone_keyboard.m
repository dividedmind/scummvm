/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */

#import "iphone_keyboard.h"

@implementation TextInputHandler

- (id)initWithKeyboard:(SoftKeyboard*)keyboard; {
	self = [super initWithFrame:CGRectMake(0.0f, 0.0f, 0.0f, 0.0f)];
	softKeyboard = keyboard;
	return self;
}

- (BOOL)webView:(id)fp8 shouldDeleteDOMRange:(id)fp12 {
	[softKeyboard handleKeyPress:0x08];
}

- (BOOL)webView:(id)fp8 shouldInsertText:(id)character
                       replacingDOMRange:(id)fp16
                             givenAction:(int)fp20 {

	if ([character length] != 1) {
		[NSException raise:@"Unsupported" format:@"Unhandled multi-char insert!"];
		return false;
	}
	[softKeyboard handleKeyPress:[character characterAtIndex:0]];
}

@end


@implementation SoftKeyboard

- (id)initWithFrame:(CGRect)frame {
	self = [super initWithFrame:frame];
	inputDelegate = nil;
	inputView = [[TextInputHandler alloc] initWithKeyboard:self];
	return self;
}

- (UITextView*)inputView {
	return inputView;
}

- (void)setInputDelegate:(id)delegate {
	inputDelegate = delegate;
}

- (void)handleKeyPress:(unichar)c {
	[inputDelegate handleKeyPress:c];
}

@end
