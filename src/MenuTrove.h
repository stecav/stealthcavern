/*
Copyright © 2011-2012 Clint Bellanger
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
 * class MenuTrove
 */

#ifndef MENU_TROVE_H
#define MENU_TROVE_H

#include "CommonIncludes.h"
#include "MenuItemStorage.h"
#include "WidgetLabel.h"

class NPC;
class StatBlock;
class WidgetButton;

class MenuTrove : public Menu {
private:
	StatBlock *stats;
	WidgetButton *closeButton;

	int TROVE_SLOTS;

	// label and widget positions
	LabelInfo title;
	LabelInfo currency;
	int slots_cols;
	int slots_rows;
	Color color_normal;
	Color color_header;

public:
	explicit MenuTrove(StatBlock *stats);
	~MenuTrove();
	void align();

	void logic();
	void render();
	ItemStack click(const Point& position);
	void itemReturn(ItemStack stack);
	bool add(ItemStack stack, int slot, bool play_sound);
	TooltipData checkTooltip(const Point& position);
	bool drop(const Point& position, ItemStack stack);

	void removeFromPrevSlot(int quantity);

	Rect slots_area;
	MenuItemStorage stock;
	bool updated;

	std::queue<ItemStack> drop_stack;
};


#endif
