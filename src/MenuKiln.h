/*
Copyright © 2015 Bryan Mcdowell

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
 * class MenuKiln
 */

#ifndef MENU_KILN_H
#define MENU_KILN_H

#include "CommonIncludes.h"
#include "ItemManager.h"
#include "Menu.h"
#include "SoundManager.h"
#include "WidgetButton.h"
#include "WidgetInput.h"

class StatBlock;
class MenuInventory;

class MenuKiln : public Menu {
protected:
	void loadGraphics();
	void updateInput();

	Point button_pos[2];
	int nb_cols;

private:
	StatBlock *stats;
	WidgetButton *okButton;
	WidgetButton *closeButton;
	int KILN_SLOTS;
	int kiln_cols;
	int kiln_rows;
	
public:
	MenuKiln(StatBlock *stats);
	~MenuKiln();
	void align();
	MenuInventory *inv;
	int slotOver(Point position);

	void logic();
	SoundManager::SoundID sfx_kiln;
	void render();
	bool drop(const Point& position, ItemStack stack);
	bool add(ItemStack stack, int slot, bool play_sound);
	TooltipData checkTooltip(Point position);
	void removeFromPrevSlot(int quantity);
	ItemStack click(Point position);
	void itemReturn(ItemStack stack);
	int getRowsCount();
	std::vector<WidgetSlot*> slots;
	Rect kiln_area;
	bool confirm_clicked;
	MenuItemStorage oven;
	bool updated;
	TabList	tablist;
	std::queue<ItemStack> drop_stack;
};

#endif
