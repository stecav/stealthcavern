/*
Copyright © 2011-2012 Clint Bellanger
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

/**
 * class WidgetLabel
 *
 * A simple text display for menus.
 * This is preferred to directly displaying text because it helps handle positioning
 */

#ifndef WIDGET_LABEL_H
#define WIDGET_LABEL_H

#include "CommonIncludes.h"
#include "Widget.h"

const int VALIGN_CENTER = 0;
const int VALIGN_TOP = 1;
const int VALIGN_BOTTOM = 2;

class LabelInfo {
public:
	int x,y;
	int justify,valign;
	bool hidden;
	std::string font_style;

	LabelInfo();
};

LabelInfo eatLabelInfo(std::string val);

class WidgetLabel : public Widget {
private:

	void recacheTextSprite();
	void applyOffsets();

	int justify;
	int valign;
	int max_width;
	Sprite *label;

	std::string text;
	std::string font_style;
	Color color;

public:
	WidgetLabel();
	~WidgetLabel();
	void render();
	void setPos(int offset_x = 0, int offset_y = 0);
	void setMaxWidth(int width);
	void set(int _x, int _y, int _justify, int _valign, const std::string& _text, const Color& _color);
	void set(int _x, int _y, int _justify, int _valign, const std::string& _text, const Color& _color, const std::string& _font);
	void setX(int _x);
	void setY(int _y);
	int getX();
	int getY();
	void setJustify(int _justify);

	void set(const std::string& _text);
	std::string get() {
		return text;
	}

	Rect bounds;
};

#endif
