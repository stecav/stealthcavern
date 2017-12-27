/*
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
 * class MenuKiln
 */

#include "FileParser.h"
#include "Menu.h"
#include "MenuInventory.h"
#include "MenuKiln.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "Settings.h"
#include "UtilsMath.h"
#include "UtilsParsing.h"

#include <limits.h>

MenuKiln::MenuKiln(StatBlock *_stats)
	: Menu()
	, nb_cols(0)
	, stats(_stats)
	, okButton(new WidgetButton)
	, closeButton(new WidgetButton("images/menus/buttons/button_x.png"))
	, slots()
	, confirm_clicked(false)
	, oven()
	, updated(false)
{
	setBackground("images/menus/kiln_bg.png");
	kiln_cols = 5; // default to 5 if menus/kiln.txt::kiln_cols not set
	kiln_rows = 2; // default to 2 if menus/kiln.txt::kiln_rows not set

	//int itemCount = 0;
	// Load config settings
	FileParser infile;
	// @CLASS MenuKiln|Description of menus/kiln.txt
	if(infile.open("menus/kiln.txt")) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;
			// @ATTR kiln_area|x (integer), y (integer)|Position of the top-left slot.
			else if (infile.key == "kiln_area") {
				kiln_area.x = popFirstInt(infile.val);
				kiln_area.y = popFirstInt(infile.val);
			}
			else if (infile.key == "soundfx_kiln") {
				// @ATTR soundfx_kiln|string|Filename of a sound to play when kiln items.
				sfx_kiln = snd->load(infile.val, "Kiln items");
			}
			// @ATTR kiln_cols|integer|The number of columns for the normal kiln.
			else if (infile.key == "kiln_cols") kiln_cols = std::max(1, toInt(infile.val));
			// @ATTR kiln_rows|integer|The number of rows for the normal kiln.
			else if (infile.key == "kiln_rows") kiln_rows = std::max(1, toInt(infile.val));
			else if (infile.key == "confirm") { // @ATTR confirm|x (integer), y (integer)|Position of the "Kiln items" button.
				button_pos[0] = toPoint(infile.val);//Kiln items
			}
			else
				infile.error("MenuKiln: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();
	}
	KILN_SLOTS = kiln_cols * kiln_rows;
	kiln_area.w = kiln_cols*ICON_SIZE;
	kiln_area.h = kiln_rows*ICON_SIZE;

	oven.initGrid(KILN_SLOTS, kiln_area, kiln_cols);
	for (int i = 0; i < KILN_SLOTS; i++) {
		tablist.add(oven.slots[i]);
	}

	align();
}

void MenuKiln::align() {
	Menu::align();

	okButton->setPos(window_area.x+button_pos[0].x, window_area.y+ button_pos[0].y);
	okButton->label = msg->get("Kiln items");
	closeButton->setPos(window_area.x+ window_area.w, window_area.y);
	oven.setPos(window_area.x, window_area.y);
}
void MenuKiln::logic() {
	if (!visible) return;
	tablist.logic();	
	int itemCount = 0;
	if (okButton->checkClick()) {
		for(int x=0; x < KILN_SLOTS; x++) { //find out if there is anything in slots
			if(!oven[x].empty()) {
				if((items->items[oven[x].item].type == "gem")||(items->items[oven[x].item].type == "other")||(items->items[oven[x].item].type == "junk")){
					pc->logMsg(msg->get("Can't kiln junk, gems or credits!"),true);
					return;
				}
				itemCount++;
			}				
		}
		for(int x=0; x < KILN_SLOTS; x++) { //kiln all items 
			oven[x].clear();
		}
		ItemStack leftover;
		int g = 1;//stats->get_luck();
		while(itemCount > 0){
			leftover.item = randBetween(89,115);
			if(g < 3) {		leftover.quantity = 1;	}
			else if(g < 6) {		leftover.quantity = randBetween(1,2);	}
			else if(g < 8) {		leftover.quantity = randBetween(1,3);	}
			else if(g < 12) {	leftover.quantity = randBetween(2,4);	}
			else{	leftover.quantity = randBetween(3,6);	}
			add(leftover,itemCount-1,true); //obtain gems from kiln items
			itemCount--;
		}
		confirm_clicked = true;
		snd->play(sfx_kiln);
	}
	if (closeButton->checkClick()) {
		visible = false;
	}
}
int MenuKiln::slotOver(Point position) {
	if (isWithinRect(kiln_area, position)) {	
		return (position.x - kiln_area.x) / slots[0]->pos.w + (position.y - kiln_area.y) / slots[0]->pos.w * nb_cols;	
	}
	else if (nb_cols == 0) {
		for (unsigned int i=0; i<slots.size(); i++) {
			if (isWithinRect(slots[i]->pos, position)) return i;
		}
	}
	return -1;
}
int MenuKiln::getRowsCount() {
	return kiln_rows;
}
bool MenuKiln::drop(const Point& position, ItemStack stack) {
	if (stack.empty()) {
		return true;
	}

	int slot;
	int drag_prev_slot;
	bool success = true;

	items->playSound(stack.item);

	slot = oven.slotOver(position);
	drag_prev_slot = oven.drag_prev_slot;

	if (slot == -1) {
		success = add(stack, slot, false);
	}
	else if (drag_prev_slot != -1) {
		if (oven[slot].item == stack.item || oven[slot].empty()) {
			// Drop the stack, merging if needed
			success = add(stack, slot, false);
		}
		else if (drag_prev_slot != -1 && oven[drag_prev_slot].empty()) {
			// Check if the previous slot is free (could still be used if SHIFT was used).
			// Swap the two stacks
			itemReturn(oven[slot]);
			oven[slot] = stack;
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

bool MenuKiln::add(ItemStack stack, int slot, bool play_sound) {
	if (stack.empty()) {
		return true;
	}

	if (play_sound) {
		items->playSound(stack.item);
	}

	if (items->items[stack.item].mission_item) {
		pc->logMsg(msg->get("Can not kiln mission items."), true);
		drop_stack.push(stack);
		return false;
	}

	ItemStack leftover = oven.add(stack, slot);
	if (!leftover.empty()) {
		if (leftover.quantity != stack.quantity) {
			updated = true;
		}
		pc->logMsg(msg->get("Kiln is full."), true);
		drop_stack.push(leftover);
		return false;
	}
	else {
		updated = true;
	}

	return true;
}
TooltipData MenuKiln::checkTooltip(Point position) {
	return oven.checkTooltip(position, stats, PLAYER_INV);
}
void MenuKiln::removeFromPrevSlot(int quantity) {
	int drag_prev_slot = oven.drag_prev_slot;
	if (drag_prev_slot > -1) {
		oven.subtract(drag_prev_slot, quantity);
	}
}
ItemStack MenuKiln::click(Point position) {
	ItemStack stack = oven.click(position);
	if (TOUCHSCREEN) {
		tablist.setCurrent(oven.current_slot);
	}
	return stack;
}
void MenuKiln::itemReturn(ItemStack stack) {
	oven.itemReturn(stack);
}
void MenuKiln::render() {
	if (!visible) return;

	// background
	Menu::render();

	// close button
	closeButton->render();
	for(int x=0; x < KILN_SLOTS; x++) { //only show button if there's something to kiln
		if(!oven[x].empty()) {
			okButton->render();
			break;break;
		}				
	}

	// text overlay
	WidgetLabel label;

	// show stock
	oven.render();
}

MenuKiln::~MenuKiln() {
	delete okButton;
	delete closeButton;
}

