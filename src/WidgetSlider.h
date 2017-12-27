/*
Copyright © 2012-2015 Justin Jacobs
Copyright © 2013 Kurt Rinnert

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
 * class WidgetSlider
 */

#ifndef WIDGET_SLIDER_H
#define WIDGET_SLIDER_H

class Widget;

class WidgetSlider : public Widget {
public:
	explicit WidgetSlider (const std::string & fname = "images/menus/buttons/slider_default.png");
	~WidgetSlider ();
	void setPos(int offset_x = 0, int offset_y = 0);

	bool checkClick ();
	bool checkClick (int x, int y);
	void set (int min, int max, int val);
	int getValue () const;
	void render ();
	bool enabled;

	Rect pos_knob; // This is the position of the slider's knob within the screen

	bool getNext();
	bool getPrev();

private:
	Sprite *sl;
	bool pressed;
	bool changed_without_mouse;
	int minimum;
	int maximum;
	int value;
};

#endif

