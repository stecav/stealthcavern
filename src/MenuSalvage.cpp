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
 * class MenuSalvage
 */

#include "FileParser.h"
#include "Menu.h"
#include "MenuInventory.h"
#include "MenuSalvage.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "Settings.h"
#include "UtilsMath.h"
#include "UtilsParsing.h"
#include <limits.h>

MenuSalvage::MenuSalvage(StatBlock *_stats)
	: Menu()
	, nb_cols(0)
	, stats(_stats)
	, okButton(new WidgetButton)
	, closeButton(new WidgetButton("images/menus/buttons/button_x.png"))
	, slots()
	, confirm_clicked(false)
	, pot()
	, updated(false)
{
	setBackground("images/menus/salvage_bg.png");
	salvage_cols = 5; // default to 5 if menus/salvage.txt::salvage_cols not set
	salvage_rows = 2; // default to 2 if menus/salvage.txt::salvage_rows not set

	// Load config settings
	FileParser infile;
	// @CLASS MenuSalvage|Description of menus/salvage.txt
	if(infile.open("menus/salvage.txt")) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;
			if (infile.key == "close") {
				Point pos = toPoint(infile.val);
				closeButton->setBasePos(pos.x, pos.y);
			}
			// @ATTR salvage_area|x (integer), y (integer)|Position of the top-left slot.
			else if (infile.key == "salvage_area") {
				salvage_area.x = popFirstInt(infile.val);
				salvage_area.y = popFirstInt(infile.val);
			}
			else if (infile.key == "soundfx_salvage") {
				// @ATTR soundfx_salvage|string|Filename of a sound to play when salvaging items.
				sfx_salvage = snd->load(infile.val, "Salvage items");
			}
			// @ATTR salvage_cols|integer|The number of columns for the normal salvage.
			else if (infile.key == "salvage_cols") salvage_cols = std::max(1, toInt(infile.val));
			// @ATTR salvage_rows|integer|The number of rows for the normal salvage.
			else if (infile.key == "salvage_rows") salvage_rows = std::max(1, toInt(infile.val));
			else if (infile.key == "confirm") { // @ATTR confirm|x (integer), y (integer)|Position of the "Salvage items" button.
				button_pos[0] = toPoint(infile.val);//Salvage items
			}
			else
				infile.error("MenuSalvage: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();
	}
	SALVAGE_SLOTS = salvage_cols * salvage_rows;
	salvage_area.w = salvage_cols*ICON_SIZE;
	salvage_area.h = salvage_rows*ICON_SIZE;

	pot.initGrid(SALVAGE_SLOTS, salvage_area, salvage_cols);
	for (int i = 0; i < SALVAGE_SLOTS; i++) {
		tablist.add(pot.slots[i]);
	}

	align();
}

void MenuSalvage::align() {
	Menu::align();

	okButton->setPos(window_area.x+button_pos[0].x, window_area.y+ button_pos[0].y);
	okButton->label = msg->get("Salvage items");
	closeButton->setPos(window_area.x + window_area.w, window_area.y);
	pot.setPos(window_area.x, window_area.y);
}
void MenuSalvage::logic() {
	if (visible) {
		tablist.logic();
		if (okButton->checkClick()) {
			int itemCount = 0;
			for(int x=0; x < SALVAGE_SLOTS; x++){//find out if there is anything in slots
				if(!pot[x].empty()){
					if((items->items[pot[x].item].type == "scrap")||(items->items[pot[x].item].type == "other")){
						pc->logMsg(msg->get("Can't salvage scrap or credits!"),true);
						return;
					} 
					itemCount++; 
				}
			}
			if(itemCount == 0){return;}
			for(int x=0; x < SALVAGE_SLOTS; x++){//salvage all items 
				pot[x].clear();
			}
			ItemStack leftover;
			int g = 1;//stats->get_luck();
			while(itemCount > 0){
				leftover.item = randBetween(57,80);
				if(g < 3){		leftover.quantity = randBetween(1,2);	}
				else if(g < 6){		leftover.quantity = randBetween(1,4);	}
				else if(g < 8){		leftover.quantity = randBetween(1,6);	}
				else if(g < 12){	leftover.quantity = randBetween(2,8);	}
				else{	leftover.quantity = randBetween(3,10);	}
				add(leftover,itemCount-1,true);//obtain scrap from salvage items
				itemCount--;
			}
			confirm_clicked = true;
			snd->play(sfx_salvage);
		}
		if (closeButton->checkClick()) {
		visible = false;
		snd->play(sfx_close);
		}
	}
}
int MenuSalvage::slotOver(Point position) {
	if (isWithinRect(salvage_area, position)) {	
		return (position.x - salvage_area.x) / slots[0]->pos.w + (position.y - salvage_area.y) / slots[0]->pos.w * nb_cols;	
	}
	else if (nb_cols == 0) {
		for (unsigned int i=0; i<slots.size(); i++) {
			if (isWithinRect(slots[i]->pos, position)) return i;
		}
	}
	return -1;
}
int MenuSalvage::getRowsCount() {
	return salvage_rows;
}
bool MenuSalvage::drop(const Point& position, ItemStack stack) {
	if (stack.empty()) {
		return true;
	}

	int slot;
	int drag_prev_slot;
	bool success = true;

	items->playSound(stack.item);

	slot = pot.slotOver(position);
	drag_prev_slot = pot.drag_prev_slot;

	if (slot == -1) {
		success = add(stack, slot, false);
	}
	else if (drag_prev_slot != -1) {
		if (pot[slot].item == stack.item || pot[slot].empty()) {
			// Drop the stack, merging if needed
			success = add(stack, slot, false);
		}
		else if (drag_prev_slot != -1 && pot[drag_prev_slot].empty()) {
			// Check if the previous slot is free (could still be used if SHIFT was used).
			// Swap the two stacks
			itemReturn(pot[slot]);
			pot[slot] = stack;
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

bool MenuSalvage::add(ItemStack stack, int slot, bool play_sound) {
	if (stack.empty()) {
		return true;
	}

	if (play_sound) {
		items->playSound(stack.item);
	}

	if (items->items[stack.item].mission_item) {
		pc->logMsg(msg->get("Can not salvage mission items."), true);
		drop_stack.push(stack);
		return false;
	}

	ItemStack leftover = pot.add(stack, slot);
	if (!leftover.empty()) {
		if (leftover.quantity != stack.quantity) {
			updated = true;
		}
		pc->logMsg(msg->get("Salvage is full."), true);
		drop_stack.push(leftover);
		return false;
	}
	else {
		updated = true;
	}

	return true;
}
TooltipData MenuSalvage::checkTooltip(Point position) {
	return pot.checkTooltip(position, stats, PLAYER_INV);
}
void MenuSalvage::removeFromPrevSlot(int quantity) {
	int drag_prev_slot = pot.drag_prev_slot;
	if (drag_prev_slot > -1) {
		pot.subtract(drag_prev_slot, quantity);
	}
}
ItemStack MenuSalvage::click(Point position) {
	ItemStack stack = pot.click(position);
	if (TOUCHSCREEN) {
		tablist.setCurrent(pot.current_slot);
	}
	return stack;
}
void MenuSalvage::itemReturn(ItemStack stack) {
	pot.itemReturn(stack);
}
void MenuSalvage::render() {
	if (!visible) return;

	// background
	Menu::render();

	// close button
	closeButton->render();
	for(int x=0; x < SALVAGE_SLOTS; x++) { //only show button if there's something to salvage
		if(!pot[x].empty()) {
			okButton->render();
			break;break;
		}				
	}

	// text overlay
	WidgetLabel label;

	// show stock
	pot.render();
}

MenuSalvage::~MenuSalvage() {
	delete okButton;
	delete closeButton;
}

