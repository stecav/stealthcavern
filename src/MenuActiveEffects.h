/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012-2014 Justin Jacobs
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
 * MenuActiveEffects
 *
 * Handles the display of active effects (buffs/debuffs)
 */

#ifndef MENU_ACTIVE_EFFECTS_H
#define MENU_ACTIVE_EFFECTS_H

#include "CommonIncludes.h"
#include "Menu.h"
#include "TooltipData.h"
#include "Utils.h"

class StatBlock;

class EffectIcon {
public:
	int icon;
	int type;
	int current;
	int max;
	Rect pos;
	Rect overlay;
	std::string name;

	EffectIcon()
		: icon(-1)
		, type(0)
		, current(0)
		, max(0)
	{}
};

class MenuActiveEffects : public Menu {
private:
	Sprite *timer;
	StatBlock *stats;
	bool is_vertical;
	std::vector<EffectIcon> effect_icons;

public:
	explicit MenuActiveEffects(StatBlock *_stats);
	~MenuActiveEffects();
	void loadGraphics();
	void logic();
	void render();
	TooltipData checkTooltip(const Point& mouse);
};

#endif
