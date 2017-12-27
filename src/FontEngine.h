/*
Copyright © 2011-2012 Clint Bellanger and Thane Brimhall
Copyright © 2014 Henrik Andersson
Copyright © 2015 Igor Paliychuk
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

#ifndef FONT_ENGINE_H
#define FONT_ENGINE_H

#include "CommonIncludes.h"
#include "Utils.h"
#include <map>

const int JUSTIFY_LEFT = 0;
const int JUSTIFY_RIGHT = 1;
const int JUSTIFY_CENTER = 2;

const Color FONT_WHITE = Color(255,255,255);
const Color FONT_BLACK = Color(0,0,0);

class FontStyle {
public:
	std::string name;
	std::string path;
	int ptsize;
	bool blend;
	int line_height;
	int font_height;

	FontStyle();
	virtual ~FontStyle() {};
};

/**
 *
 * class FontEngine
 * Provide abstract interface for Stealth Cavern engine text rendering.
 *
 */
class FontEngine {

public:
	FontEngine();
	virtual ~FontEngine() {};

	Color getColor(const std::string& _color);

	Point calc_size(const std::string& text_with_newlines, int width);

	void render(const std::string& text, int x, int y, int justify, Image *target, int width, const Color& color);
	void renderShadowed(const std::string& text, int x, int y, int justify, Image *target, int width, const Color& color);

	virtual int getLineHeight() = 0;
	virtual int getFontHeight() = 0;

	virtual void setFont(const std::string& _font) = 0;
	virtual int calc_width(const std::string& text) = 0;
	virtual std::string trimTextToWidth(const std::string& text, const int width, const bool use_ellipsis, size_t left_pos) = 0;

	int cursor_y;

protected:
	Rect position(const std::string& text, int x, int y, int justify);
	virtual void renderInternal(const std::string& text, int x, int y, int justify, Image *target, const Color& color) = 0;

	std::map<std::string,Color> color_map;

};

#endif
