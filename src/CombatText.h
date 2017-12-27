/*
Copyright © 2011-2012 Thane Brimhall
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
 * class CombatText
 *
 * The CombatText class displays floating damage numbers and miss messages
 * above the targets.
 *
 */

#ifndef COMBAT_TEXT_H
#define COMBAT_TEXT_H

#include "CommonIncludes.h"
#include "Utils.h"
#include "WidgetLabel.h"

#define COMBAT_MESSAGE_GIVEDMG 0
#define COMBAT_MESSAGE_TAKEDMG 1
#define COMBAT_MESSAGE_CRIT 2
#define COMBAT_MESSAGE_MISS 3
#define COMBAT_MESSAGE_BUFF 4

class WidgetLabel;

class Combat_Text_Item {
public:
	Combat_Text_Item();
	~Combat_Text_Item();

	WidgetLabel *label;
	int lifespan;
	FPoint pos;
	float floating_offset;
	std::string text;
	int displaytype;
};

class CombatText {
public:
	CombatText();
	~CombatText();

	void logic(const FPoint& _cam);
	void render();
	void addString(const std::string& message, const FPoint& location, int displaytype);
	void addInt(int num, const FPoint& location, int displaytype);
	void clear();

private:
	FPoint cam;
	std::vector<Combat_Text_Item> combat_text;

	Color msg_color[5];
	int duration;
	float speed;
	int offset;
};

#endif
