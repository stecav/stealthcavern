/*
Copyright © 2011-2012 kitano
Copyright © 2012 Stefan Beller
Copyright © 2013 Kurt Rinnert
Copyright © 2014 Henrik Andersson
Copyright © 2012-2016 Justin Jacobs

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

#include "WidgetInput.h"
#include "SharedResources.h"
#include "Settings.h"

WidgetInput::WidgetInput(const std::string& filename)
	: background(NULL)
	, enabled(true)
	, pressed(false)
	, hover(false)
	, cursor_frame(0)
	, del_frame(0)
	, max_del_frame(0)
	, inFocus(false)
	, max_length(0)
	, only_numbers(false)
{

	loadGraphics(filename);

	render_to_alpha = false;
	color_normal = font->getColor("widget_normal");
}

void WidgetInput::setPos(int offset_x, int offset_y) {
	setPosition(pos_base.x+offset_x, pos_base.y+offset_y);
}

void WidgetInput::loadGraphics(const std::string& filename) {
	// load input background image
	Image *graphics;
	graphics = render_device->loadImage(filename, "Couldn't load image", true);
	if (graphics) {
		background = graphics->createSprite();
		pos.w = background->getGraphicsWidth();
		pos.h = background->getGraphicsHeight()/2;
		graphics->unref();
	}
}

void WidgetInput::trimText() {
	int padding =font->getFontHeight();
	trimmed_text = font->trimTextToWidth(text, pos.w-padding, false, text.length());
}

void WidgetInput::logic() {
	if (logic(inpt->mouse.x,inpt->mouse.y))
		return;
}

bool WidgetInput::logic(int x, int y) {
	Point mouse(x, y);

	// Change the hover state
	hover = isWithinRect(pos, mouse);

	if (checkClick()) {
		inFocus = true;
	}

	// if clicking elsewhere unfocus the text box
	if (inpt->pressing[MAIN1]) {
		if (!isWithinRect(pos, inpt->mouse)) {
			inFocus = false;
		}
	}

	if (inFocus) {

		if (inpt->inkeys != "") {
			// handle text input
			// only_numbers will restrict our input to 0-9 characters
			if (!only_numbers || (inpt->inkeys[0] >= 48 && inpt->inkeys[0] <= 57)) {
				text += inpt->inkeys;
				trimText();
			}

			// HACK: this prevents normal keys from triggering common menu shortcuts
			if (inpt->pressing[ACCEPT]) {
				inpt->lock[ACCEPT] = true;
			}
			else if (inpt->pressing[CANCEL]) {
				inpt->lock[CANCEL] = true;
			}
		}

		if (!inpt->pressing[DEL]) {
			max_del_frame = MAX_FRAMES_PER_SEC;
		}

		// handle backspaces
		if (!inpt->lock[DEL] && inpt->pressing[DEL]) {
			inpt->lock[DEL] = true;
			del_frame = 0;
			max_del_frame = std::max(MAX_FRAMES_PER_SEC/8, max_del_frame - (MAX_FRAMES_PER_SEC/4));
			// remove utf-8 character
			int n = static_cast<int>(text.length()-1);
			while (n > 0 && ((text[n] & 0xc0) == 0x80) ) n--;
			text = text.substr(0, n);
			trimText();
		} else if (inpt->pressing[DEL]) {
			// delay unlocking of DEL lock
			del_frame++;
		}
		if (inpt->pressing[DEL] && inpt->lock[DEL] && del_frame >= max_del_frame) {
			// after X frames allow DEL again
			inpt->lock[DEL]	= false;
		}

		// animate cursor
		// cursor visible one second, invisible the next
		cursor_frame++;
		if (cursor_frame == MAX_FRAMES_PER_SEC+MAX_FRAMES_PER_SEC) cursor_frame = 0;

	}
	return true;
}

void WidgetInput::render() {
	Rect src;
	src.x = 0;
	src.y = (inFocus ? pos.h : 0);
	src.w = pos.w;
	src.h = pos.h;

	if (background) {
		background->local_frame = local_frame;
		background->setOffset(local_offset);
		background->setClip(src);
		background->setDest(pos);
		render_device->render(background);
	}

	font->setFont("font_regular");

	if (!inFocus) {
		font->render(trimmed_text, font_pos.x, font_pos.y, JUSTIFY_LEFT, NULL, 0, color_normal);
	}
	else {
		if (cursor_frame < MAX_FRAMES_PER_SEC) {
			font->renderShadowed(trimmed_text + "|", font_pos.x, font_pos.y, JUSTIFY_LEFT, NULL, 0, color_normal);
		}
		else {
			font->renderShadowed(trimmed_text, font_pos.x, font_pos.y, JUSTIFY_LEFT, NULL, 0, color_normal);
		}
	}
}

void WidgetInput::setPosition(int x, int y) {
	pos.x = x + local_frame.x - local_offset.x;
	pos.y = y + local_frame.y - local_offset.y;

	font->setFont("font_regular");
	font_pos.x = pos.x  + (font->getFontHeight()/2);
	font_pos.y = pos.y + (pos.h/2) - (font->getFontHeight()/2);
}

bool WidgetInput::checkClick() {

	// disabled buttons can't be clicked;
	if (!enabled) return false;

	// main button already in use, new click not allowed
	if (inpt->lock[MAIN1]) return false;

	// main click released, so the button state goes back to unpressed
	if (pressed && !inpt->lock[MAIN1]) {
		pressed = false;

		if (isWithinRect(pos, inpt->mouse)) {

			// activate upon release
			return true;
		}
	}

	pressed = false;

	// detect new click
	if (inpt->pressing[MAIN1]) {
		if (isWithinRect(pos, inpt->mouse)) {

			inpt->lock[MAIN1] = true;
			pressed = true;

		}
	}
	return false;
}

WidgetInput::~WidgetInput() {
	if (background) delete background;
}

