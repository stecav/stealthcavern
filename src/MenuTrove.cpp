/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2013 Henrik Andersson
Copyright © 2013 Kurt Rinnert
Copyright © 2014 Henrik Andersson
Copyright © 2012-2016 Justin Jacobs
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

/**
 * class MenuTrove
 */

#include "FileParser.h"
#include "Menu.h"
#include "MenuTrove.h"
#include "Settings.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "UtilsParsing.h"
#include "WidgetButton.h"

MenuTrove::MenuTrove(StatBlock *_stats)
	: Menu()
	, stats(_stats)
	, closeButton(new WidgetButton("images/menus/buttons/button_x.png"))
	, color_normal(font->getColor("menu_normal"))
	, color_header(font->getColor("menu_header"))
	, stock()
	, updated(false)
{

	setBackground("images/menus/trove.png");

	slots_cols = 8; // default if menus/trove.txt::trove_cols not set
	slots_rows = 8; // default if menus/trove.txt::slots_rows not set

	// Load config settings
	FileParser infile;
	// @CLASS MenuTrove|Description of menus/trove.txt
	if (infile.open("menus/trove.txt")) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;

			// @ATTR close|point|Position of the close button.
			if (infile.key == "close") {
				Point pos = toPoint(infile.val);
				closeButton->setBasePos(pos.x, pos.y);
			}
			// @ATTR slots_area|point|Position of the top-left slot.
			else if (infile.key == "slots_area") {
				slots_area.x = popFirstInt(infile.val);
				slots_area.y = popFirstInt(infile.val);
			}
			// @ATTR trove_cols|int|The number of columns for the grid of slots.
			else if (infile.key == "trove_cols") {
				slots_cols = std::max(1, toInt(infile.val));
			}
			// @ATTR trove_rows|int|The number of rows for the grid of slots.
			else if (infile.key == "trove_rows") {
				slots_rows = std::max(1, toInt(infile.val));
			}
			// @ATTR label_title|label|Position of the "Trove" label.
			else if (infile.key == "label_title") {
				title =  eatLabelInfo(infile.val);
			}
			// @ATTR currency|label|Position of the label displaying the amount of currency stored in the trove.
			else if (infile.key == "currency") {
				currency =  eatLabelInfo(infile.val);
			}
			else {
				infile.error("MenuTrove: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}

	TROVE_SLOTS = slots_cols * slots_rows;
	slots_area.w = slots_cols*ICON_SIZE;
	slots_area.h = slots_rows*ICON_SIZE;

	stock.initGrid(TROVE_SLOTS, slots_area, slots_cols);
	for (int i = 0; i < TROVE_SLOTS; i++) {
		tablist.add(stock.slots[i]);
	}

	align();
}

void MenuTrove::align() {
	Menu::align();

	closeButton->setPos(window_area.x, window_area.y);
	stock.setPos(window_area.x, window_area.y);
}

void MenuTrove::logic() {
	if (!visible) return;

	tablist.logic();

	if (closeButton->checkClick()) {
		visible = false;
		snd->play(sfx_close);
	}
}

void MenuTrove::render() {
	if (!visible) return;

	// background
	Menu::render();

	// close button
	closeButton->render();

	// text overlay
	WidgetLabel label;
	if (!title.hidden) {
		label.set(window_area.x+title.x, window_area.y+title.y, title.justify, title.valign, msg->get("TROVE"), color_header, title.font_style);
		label.render();
	}
	if (!currency.hidden) {
		label.set(window_area.x+currency.x, window_area.y+currency.y, currency.justify, currency.valign, msg->get("%d %s", stock.count(CURRENCY_ID), CURRENCY), color_normal, currency.font_style);
		label.render();
	}


	// show stock
	stock.render();
}

/**
 * Dragging and dropping an item can be used to rearrange the trove
 */
bool MenuTrove::drop(const Point& position, ItemStack stack) {
	if (stack.empty()) {
		return true;
	}

	int slot;
	int drag_prev_slot;
	bool success = true;

	items->playSound(stack.item);

	slot = stock.slotOver(position);
	drag_prev_slot = stock.drag_prev_slot;

	if (slot == -1) {
		success = add(stack, slot, false);
	}
	else if (drag_prev_slot != -1) {
		if (stock[slot].item == stack.item || stock[slot].empty()) {
			// Drop the stack, merging if needed
			success = add(stack, slot, false);
		}
		else if (drag_prev_slot != -1 && stock[drag_prev_slot].empty()) {
			// Check if the previous slot is free (could still be used if SHIFT was used).
			// Swap the two stacks
			itemReturn(stock[slot]);
			stock[slot] = stack;
			updated = true;
		}
		else {
			itemReturn(stack);
			updated = true;
		}
	}
	else {
		success = add(stack, slot, false);
	}

	return success;
}

bool MenuTrove::add(ItemStack stack, int slot, bool play_sound) {
	if (stack.empty()) {
		return true;
	}

	if (play_sound) {
		items->playSound(stack.item);
	}

	if (items->items[stack.item].mission_item) {
		pc->logMsg(msg->get("Can not store mission items in the trove."), true);
		drop_stack.push(stack);
		return false;
	}

	ItemStack leftover = stock.add(stack, slot);
	if (!leftover.empty()) {
		if (leftover.quantity != stack.quantity) {
			updated = true;
		}
		pc->logMsg(msg->get("Trove is full."), true);
		drop_stack.push(leftover);
		return false;
	}
	else {
		updated = true;
	}

	return true;
}

/**
 * Start dragging a vendor item
 * Players can drag an item to their inventory.
 */
ItemStack MenuTrove::click(const Point& position) {
	ItemStack stack = stock.click(position);
	if (TOUCHSCREEN) {
		tablist.setCurrent(stock.current_slot);
	}
	return stack;
}

/**
 * Cancel the dragging initiated by the click()
 */
void MenuTrove::itemReturn(ItemStack stack) {
	stock.itemReturn(stack);
}

TooltipData MenuTrove::checkTooltip(const Point& position) {
	return stock.checkTooltip(position, stats, PLAYER_INV);
}

void MenuTrove::removeFromPrevSlot(int quantity) {
	int drag_prev_slot = stock.drag_prev_slot;
	if (drag_prev_slot > -1) {
		stock.subtract(drag_prev_slot, quantity);
	}
}

MenuTrove::~MenuTrove() {
	delete closeButton;
}

