/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2013-2014 Henrik Andersson
Copyright © 2013 Kurt Rinnert
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
 * class MenuVendor
 */

#include "FileParser.h"
#include "ItemStorage.h"
#include "Menu.h"
#include "MenuVendor.h"
#include "NPC.h"
#include "Settings.h"
#include "SharedResources.h"
#include "UtilsParsing.h"
#include "WidgetButton.h"
#include "WidgetTabControl.h"
#include "SharedGameResources.h"

MenuVendor::MenuVendor(StatBlock *_stats)
	: Menu()
	, stats(_stats)
	, closeButton(new WidgetButton("images/menus/buttons/button_x.png"))
	, tabControl(new WidgetTabControl())
	, slots_cols(1)
	, slots_rows(1)
	, activetab(VENDOR_BUY)
	, color_normal(font->getColor("menu_header"))
	, npc(NULL)
	, buyback_stock() {
	setBackground("images/menus/vendor.png");

	tabControl->setTabTitle(VENDOR_BUY, msg->get("Inventory"));
	tabControl->setTabTitle(VENDOR_SELL, msg->get("Buyback"));

	// Load config settings
	FileParser infile;
	// @CLASS MenuVendor|Description of menus/vendor.txt
	if(infile.open("menus/vendor.txt")) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;

			// @ATTR close|point|Position of the close button.
			if(infile.key == "close") {
				Point pos = toPoint(infile.val);
				closeButton->setBasePos(pos.x, pos.y);
			}
			// @ATTR slots_area|point|Position of the top-left slot.
			else if(infile.key == "slots_area") {
				slots_area.x = popFirstInt(infile.val);
				slots_area.y = popFirstInt(infile.val);
			}
			// @ATTR vendor_cols|int|The number of columns in the grid of slots.
			else if (infile.key == "vendor_cols") {
				slots_cols = std::max(1, toInt(infile.val));
			}
			// @ATTR vendor_rows|int|The number of rows in the grid of slots.
			else if (infile.key == "vendor_rows") {
				slots_rows = std::max(1, toInt(infile.val));
			}
			// @ATTR label_title|label|The position of the text that displays the NPC's name.
			else if (infile.key == "label_title") {
				title =  eatLabelInfo(infile.val);
			}
			else {
				infile.error("MenuVendor: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}

	VENDOR_SLOTS = slots_cols * slots_rows;
	slots_area.w = slots_cols*ICON_SIZE;
	slots_area.h = slots_rows*ICON_SIZE;

	stock[VENDOR_BUY].initGrid(VENDOR_SLOTS, slots_area, slots_cols);
	stock[VENDOR_SELL].initGrid(VENDOR_SLOTS, slots_area, slots_cols);
	buyback_stock.init(NPC_VENDOR_MAX_STOCK);

	tablist.add(tabControl);
	tablist_buy.setPrevTabList(&tablist);
	tablist_sell.setPrevTabList(&tablist);

	tablist_buy.lock();
	tablist_sell.lock();

	for (unsigned i = 0; i < VENDOR_SLOTS; i++) {
		tablist_buy.add(stock[VENDOR_BUY].slots[i]);
	}
	for (unsigned i = 0; i < VENDOR_SLOTS; i++) {
		tablist_sell.add(stock[VENDOR_SELL].slots[i]);
	}

	align();
}

void MenuVendor::align() {
	Menu::align();

	Rect tabs_area = slots_area;
	tabs_area.x += window_area.x;
	tabs_area.y += window_area.y;

	int tabheight = tabControl->getTabHeight();
	tabControl->setMainArea(tabs_area.x, tabs_area.y-tabheight, tabs_area.w, tabs_area.h+tabheight);
	tabControl->updateHeader();

	closeButton->setPos(window_area.x, window_area.y);

	stock[VENDOR_BUY].setPos(window_area.x, window_area.y);
	stock[VENDOR_SELL].setPos(window_area.x, window_area.y);
}

void MenuVendor::logic() {
	if (!visible) return;

	tablist.logic();
	tablist_buy.logic();
	tablist_sell.logic();

	tabControl->logic();
	if (TOUCHSCREEN && activetab != tabControl->getActiveTab()) {
		tablist_buy.defocus();
		tablist_sell.defocus();
	}
	activetab = tabControl->getActiveTab();

	if (activetab == VENDOR_BUY)
		tablist.setNextTabList(&tablist_buy);
	else if (activetab == VENDOR_SELL)
		tablist.setNextTabList(&tablist_sell);

	if (TOUCHSCREEN) {
		if (activetab == VENDOR_BUY && tablist_buy.getCurrent() == -1)
			stock[VENDOR_BUY].current_slot = NULL;
		else if (activetab == VENDOR_SELL && tablist_sell.getCurrent() == -1)
			stock[VENDOR_SELL].current_slot = NULL;
	}

	if (closeButton->checkClick()) {
		setNPC(NULL);
		snd->play(sfx_close);
	}
}

void MenuVendor::setTab(int tab) {
	if (TOUCHSCREEN && activetab != tab) {
		tablist_buy.defocus();
		tablist_sell.defocus();
	}
	tabControl->setActiveTab(tab);
	activetab = tab;
}

void MenuVendor::render() {
	if (!visible) return;

	// background
	Menu::render();

	// close button
	closeButton->render();

	// text overlay
	if (!title.hidden) {
		label_vendor.set(window_area.x+title.x, window_area.y+title.y, title.justify, title.valign, msg->get("VENDOR") + " - " + npc->name, color_normal, title.font_style);
		label_vendor.render();
	}

	// render tabs
	tabControl->render();

	// show stock
	stock[activetab].render();
}

/**
 * Start dragging a vendor item
 * Players can drag an item to their inventory to purchase.
 */
ItemStack MenuVendor::click(const Point& position) {
	ItemStack stack = stock[activetab].click(position);
	saveInventory();
	if (TOUCHSCREEN) {
		if (activetab == VENDOR_BUY)
			tablist_buy.setCurrent(stock[activetab].current_slot);
		else if (activetab == VENDOR_SELL)
			tablist_sell.setCurrent(stock[activetab].current_slot);
	}
	return stack;
}

/**
 * Cancel the dragging initiated by the clic()
 */
void MenuVendor::itemReturn(ItemStack stack) {
	items->playSound(stack.item);
	stock[activetab].itemReturn(stack);
	saveInventory();
}

void MenuVendor::add(ItemStack stack) {
	// Remove the first item stack to make room
	if (stock[VENDOR_SELL].full(stack)) {
		stock[VENDOR_SELL][0].clear();
		sort(VENDOR_SELL);
	}
	items->playSound(stack.item);
	stock[VENDOR_SELL].add(stack);
	saveInventory();
}

TooltipData MenuVendor::checkTooltip(const Point& position) {
	int vendor_view = (activetab == VENDOR_BUY) ? VENDOR_BUY : VENDOR_SELL;
	return stock[activetab].checkTooltip(position, stats, vendor_view);
}

/**
 * Save changes to the inventory back to the NPC
 * For persistent stock amounts and buyback (at least until
 * the player leaves this map)
 */
void MenuVendor::saveInventory() {
	for (unsigned i=0; i<VENDOR_SLOTS; i++) {
		if (npc) npc->stock[i] = stock[VENDOR_BUY][i];
		buyback_stock[i] = stock[VENDOR_SELL][i];
	}

}

void MenuVendor::sort(int type) {
	for (unsigned i=0; i<VENDOR_SLOTS; i++) {
		if (stock[type][i].empty()) {
			for (unsigned j=i; j<VENDOR_SLOTS; j++) {
				if (!stock[type][j].empty()) {
					stock[type][i] = stock[type][j];
					stock[type][j].clear();
					break;
				}
			}
		}
	}
}

void MenuVendor::setNPC(NPC* _npc) {
	npc = _npc;

	if (_npc == NULL) {
		visible = false;
		return;
	}

	setTab(VENDOR_BUY);

	for (unsigned i=0; i<VENDOR_SLOTS; i++) {
		stock[VENDOR_BUY][i] = npc->stock[i];
		stock[VENDOR_SELL][i] = buyback_stock[i];
	}
	sort(VENDOR_BUY);
	sort(VENDOR_SELL);

	if (!visible) {
		visible = true;
		snd->play(sfx_open);
		npc->playSound(NPC_VOX_INTRO);
	}
}

void MenuVendor::removeFromPrevSlot(int quantity) {
	int drag_prev_slot = stock[activetab].drag_prev_slot;
	if (drag_prev_slot > -1) {
		stock[activetab].subtract(drag_prev_slot, quantity);
		saveInventory();
	}
}

void MenuVendor::lockTabControl() {
	tablist_buy.setPrevTabList(NULL);
	tablist_sell.setPrevTabList(NULL);
}

void MenuVendor::unlockTabControl() {
	tablist_buy.setPrevTabList(&tablist);
	tablist_sell.setPrevTabList(&tablist);
}

TabList* MenuVendor::getCurrentTabList() {
	if (tablist.getCurrent() != -1)
		return (&tablist);
	else if (tablist_buy.getCurrent() != -1) {
		return (&tablist_buy);
	}
	else if (tablist_sell.getCurrent() != -1) {
		return (&tablist_sell);
	}

	return NULL;
}

void MenuVendor::defocusTabLists() {
	tablist.defocus();
	tablist_buy.defocus();
	tablist_sell.defocus();
}

MenuVendor::~MenuVendor() {
	delete closeButton;
	delete tabControl;
}

