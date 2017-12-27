/*
Copyright © 2014 Justin Jacobs
Copyright © 2014 Henrik Andersson
Copyright © 2017 Bryan Mcdowell

This file is part of Stealth Cavern.

Stealth Cavern is free software: you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.

Stealth Cavern is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
Stealth Cavern.  If not, see http://www.gnu.org/licenses/
*/

#include "CursorManager.h"
#include "FileParser.h"
#include "Settings.h"
#include "SharedResources.h"
#include "UtilsParsing.h"

CursorManager::CursorManager()
	: show_cursor(true)
	, cursor_normal(NULL)
	, cursor_interact(NULL)
	, cursor_talk(NULL)
	, cursor_target(NULL)
	, cursor_attack(NULL)
	, cursor_current(NULL)
	, offset_current(NULL) {
	Image *graphics;
	FileParser infile;
	// @CLASS CursorManager|Description of engine/mouse_cursor.txt
	if (infile.open("engine/mouse_cursor.txt", true, "")) {
		while (infile.next()) {
			if (infile.key == "normal") {
				// @ATTR normal|filename|Filename of an image for the normal cursor.
				graphics = render_device->loadImage(popFirstString(infile.val));
				if (graphics) {
					cursor_normal = graphics->createSprite();
					graphics->unref();
				}
				offset_normal = toPoint(infile.val);
			}
			else if (infile.key == "interact") {
				// @ATTR interact|filename|Filename of an image for the object interaction cursor.
				graphics = render_device->loadImage(popFirstString(infile.val));
				if (graphics) {
					cursor_interact = graphics->createSprite();
					graphics->unref();
				}
				offset_interact = toPoint(infile.val);
			}
			else if (infile.key == "talk") {
				// @ATTR talk|filename|Filename of an image for the NPC interaction cursor.
				graphics = render_device->loadImage(popFirstString(infile.val));
				if (graphics) {
					cursor_talk = graphics->createSprite();
					graphics->unref();
				}
				offset_talk = toPoint(infile.val);
			}
			else if (infile.key == "target") {
				// @ATTR target|string|Filename of an image for the NPC interaction cursor.
				graphics = render_device->loadImage(popFirstString(infile.val));
				if (graphics) {
					cursor_target = graphics->createSprite();
					graphics->unref();
				}
				offset_target = toPoint(infile.val);
			}
			else if (infile.key == "attack") {
				// @ATTR attack|filename|Filename of an image for the cursor when attacking enemies.
				graphics = render_device->loadImage(popFirstString(infile.val));
				if (graphics) {
					cursor_attack = graphics->createSprite();
					graphics->unref();
				}
				offset_attack = toPoint(infile.val);
			}
			else {
				infile.error("CursorManager: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}
}

CursorManager::~CursorManager() {
	if (cursor_normal) delete cursor_normal;
	if (cursor_interact) delete cursor_interact;
	if (cursor_talk) delete cursor_talk;
	if (cursor_target) delete cursor_target;
	if (cursor_attack) delete cursor_attack;
}

void CursorManager::logic() {
	if (!show_cursor)
		return;

	if (HARDWARE_CURSOR) {
		inpt->showCursor();
		return;
	}

	cursor_current = NULL;
	offset_current = NULL;

	if (cursor_normal) {
		inpt->hideCursor();
		cursor_current = cursor_normal;
		offset_current = &offset_normal;
	}
	else {
		// system cursor
		inpt->showCursor();
	}
}

void CursorManager::render() {
	if (HARDWARE_CURSOR || !show_cursor) return;

	if (cursor_current != NULL) {
		if (offset_current != NULL) {
			cursor_current->setDest(inpt->mouse.x+offset_current->x, inpt->mouse.y+offset_current->y);
		}
		else {
			cursor_current->setDest(inpt->mouse.x, inpt->mouse.y);
		}

		render_device->render(cursor_current);
	}
}

void CursorManager::setCursor(CURSOR_TYPE type) {
	if (HARDWARE_CURSOR) return;

	if (type == CURSOR_INTERACT && cursor_interact) {
		inpt->hideCursor();
		cursor_current = cursor_interact;
		offset_current = &offset_interact;
	}
	else if (type == CURSOR_TALK && cursor_talk) {
		inpt->hideCursor();
		cursor_current = cursor_talk;
		offset_current = &offset_talk;
	}
	else if (type == CURSOR_TARGET && cursor_target) {
		inpt->hideCursor();
		cursor_current = cursor_target;
		offset_current = &offset_target;
	}
	else if (type == CURSOR_ATTACK && cursor_attack) {
		inpt->hideCursor();
		cursor_current = cursor_attack;
		offset_current = &offset_attack;
	}
	else if (cursor_normal) {
		inpt->hideCursor();
		cursor_current = cursor_normal;
		offset_current = &offset_normal;
	}
	else {
		// system cursor
		cursor_current = NULL;
		inpt->showCursor();
	}
}
