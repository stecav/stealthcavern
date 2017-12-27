/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2013-2014 Henrik Andersson
Copyright © 2013 Kurt Rinnert
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
 * class MenuManager
 */

#include "UtilsParsing.h"
#include "UtilsFileSystem.h"
#include "Menu.h"
#include "MenuBook.h"
#include "MenuManager.h"
#include "MenuActionBar.h"
#include "MenuCharacter.h"
#include "MenuStatBar.h"
#include "MenuHUDLog.h"
#include "MenuInventory.h"
#include "MenuMiniMap.h"
#include "MenuNPCActions.h"
#include "MenuNumPicker.h"
#include "MenuSalvage.h"
#include "MenuKiln.h"
#include "MenuPowers.h"
#include "MenuEnemy.h"
#include "MenuVendor.h"
#include "MenuTalker.h"
#include "MenuExit.h"
#include "MenuActiveEffects.h"
#include "MenuTrove.h"
#include "MenuLog.h"
#include "MenuDevHUD.h"
#include "MenuDevConsole.h"
#include "ModManager.h"
#include "NPC.h"
#include "SharedResources.h"
#include "WidgetTooltip.h"
#include "SharedGameResources.h"

MenuManager::MenuManager(StatBlock *_stats)
	: stats(_stats)
	, tip_buf()
	, keyb_tip_buf_vendor()
	, keyb_tip_buf_trove()
	, keyb_tip_buf_salvage()
	, keyb_tip_buf_kiln()
	, keyb_tip_buf_pow()
	, keyb_tip_buf_inv()
	, keyb_tip_buf_act()
	, key_lock(false)
	, mouse_dragging(false)
	, keyboard_dragging(false)
	, sticky_dragging(false)
	, drag_stack()
	, drag_power(0)
	, drag_src(0)
	, drag_icon(NULL)
	, done(false)
	, act_drag_hover(false)
	, keydrag_pos(Point())
/*std::vector<Menu*> menus;*/
	, inv(NULL)
	, pow(NULL)
	, chr(NULL)
	, missionlog(NULL)
	, hudlog(NULL)
	, act(NULL)
	, book(NULL)
	, hull(NULL)
	, shield(NULL)
	, psi(NULL)
	, capacitor(NULL)
	, limitbreak(NULL)
	, soul(NULL)
	, tip(NULL)
	, mini(NULL)
	, npc(NULL)
	, num_picker(NULL)
	, salvage(NULL)
	, kiln(NULL)
	, enemy(NULL)
	, vendor(NULL)
	, talker(NULL)
	, exit(NULL)
	, effects(NULL)
	, trove(NULL)
	, devhud(NULL)
	, devconsole(NULL)
	, pause(false)
	, menus_open(false) {

	hull = new MenuStatBar("hull");
	shield = new MenuStatBar("shield");
	psi = new MenuStatBar("psi");
	capacitor = new MenuStatBar("capacitor");
	limitbreak = new MenuStatBar("limitbreak");
	soul = new MenuStatBar("soul");
	effects = new MenuActiveEffects(stats);
	hudlog = new MenuHUDLog();
	act = new MenuActionBar();
	enemy = new MenuEnemy();
	vendor = new MenuVendor(stats);
	npc = new MenuNPCActions();
	talker = new MenuTalker(npc);
	exit = new MenuExit();
	mini = new MenuMiniMap();
	chr = new MenuCharacter(stats);
	inv = new MenuInventory(stats);
	pow = new MenuPowers(stats, act);
	missionlog = new MenuLog();
	trove = new MenuTrove(stats);
	book = new MenuBook();
	num_picker = new MenuNumPicker();
	salvage = new MenuSalvage(stats);
	kiln = new MenuKiln(stats);

	menus.push_back(hull); // menus[0]
	menus.push_back(shield); // menus[1]
	menus.push_back(psi); // menus[2]
	menus.push_back(capacitor); // menus[3]
	menus.push_back(limitbreak); // menus[4]
	menus.push_back(soul); // menus[5]
	menus.push_back(effects); // menus[6]
	menus.push_back(hudlog); // menus[7]
	menus.push_back(act); // menus[8]
	menus.push_back(enemy); // menus[9]
	menus.push_back(vendor); // menus[10]
	menus.push_back(talker); // menus[11]
	menus.push_back(exit); // menus[12]
	menus.push_back(mini); // menus[13]
	menus.push_back(chr); // menus[14]
	menus.push_back(inv); // menus[15]
	menus.push_back(pow); // menus[16]
	menus.push_back(missionlog); // menus[17]
	menus.push_back(trove); // menus[18]
	menus.push_back(npc); // menus[19]
	menus.push_back(book); // menus[20]
	menus.push_back(num_picker); // menus[21]
	menus.push_back(salvage); // menus[22]
	menus.push_back(kiln); // menus[23]

	if (DEV_MODE) {
		devconsole = new MenuDevConsole();
		devhud = new MenuDevHUD();
	}

	tip = new WidgetTooltip();

	closeAll(); // make sure all togglable menus start closed

	SHOW_HUD = true;
}

void MenuManager::alignAll() {
	for (size_t i=0; i<menus.size(); i++) {
		menus[i]->align();
	}

	if (DEV_MODE) {
		devconsole->align();
		devhud->align();
	}
}

void MenuManager::renderIcon(int x, int y) {
	if (drag_icon) {
		drag_icon->setDest(x,y);
		render_device->render(drag_icon);
	}
}

void MenuManager::setDragIcon(int icon_id) {
	if (!icons) return;

	if (!drag_icon) {
		Image *graphics = render_device->createImage(ICON_SIZE, ICON_SIZE);

		if (!graphics) return;
		drag_icon = graphics->createSprite();
		graphics->unref();

		icons->setIcon(icon_id, Point());
		icons->renderToImage(drag_icon->getGraphics());
	}
}

void MenuManager::setDragIconItem(ItemStack stack) {
	if (!drag_icon) {
		if (stack.empty()) return;

		setDragIcon(items->items[stack.item].icon);

		if (!drag_icon) return;

		if (stack.quantity > 1 || items->items[stack.item].max_quantity > 1) {
			std::stringstream ss;
			ss << abbreviateKilo(stack.quantity);
			font->renderShadowed(ss.str(), icons->text_offset.x, icons->text_offset.y, JUSTIFY_LEFT, drag_icon->getGraphics(), 0, font->getColor("widget_normal"));
		}
	}
}

void MenuManager::handleKeyboardNavigation() {

	trove->tablist.setNextTabList(NULL);
	vendor->tablist_buy.setNextTabList(NULL);
	vendor->tablist_sell.setNextTabList(NULL);
	chr->tablist.setNextTabList(NULL);
	missionlog->setNextTabList(&missionlog->tablist);
	inv->tablist.setPrevTabList(NULL);
	pow->setNextTabList(NULL);
	salvage->tablist.setNextTabList(NULL);
	kiln->tablist.setNextTabList(NULL);

	// unlock menus if only one side is showing
	if (!inv->visible && !pow->visible) {
		trove->tablist.unlock();
		vendor->tablist.unlock();
		chr->tablist.unlock();
		kiln->tablist.unlock();
		salvage->tablist.unlock();
		if (!missionlog->getCurrentTabList())
			missionlog->tablist.unlock();

	}
	else if (!vendor->visible && !trove->visible && !kiln->visible && !salvage->visible && !chr->visible && !missionlog->visible) {
		inv->tablist.unlock();
		if (!pow->getCurrentTabList())
			pow->tablist.unlock();
	}

	if (drag_src == 0) {
		if (inv->visible) {
			trove->tablist.setNextTabList(&inv->tablist);
			vendor->tablist_buy.setNextTabList(&inv->tablist);
			vendor->tablist_sell.setNextTabList(&inv->tablist);
			chr->tablist.setNextTabList(&inv->tablist);
			missionlog->setNextTabList(&inv->tablist);
			salvage->tablist.setNextTabList(&inv->tablist);
			kiln->tablist.setNextTabList(&inv->tablist);

			if (trove->visible) {
				inv->tablist.setPrevTabList(&trove->tablist);
			}
			else if (vendor->visible) {
				inv->tablist.setPrevTabList(&vendor->tablist);
			}
			else if (chr->visible) {
				inv->tablist.setPrevTabList(&chr->tablist);
			}
			else if (missionlog->visible) {
				inv->tablist.setPrevTabList(&missionlog->tablist);
			}
			else if (kiln->visible) {
				inv->tablist.setPrevTabList(&kiln->tablist);
			}
			else if (salvage->visible) {
				inv->tablist.setPrevTabList(&salvage->tablist);
			}
		}
		else if (pow->visible) {
			trove->tablist.setNextTabList(&pow->tablist);
			vendor->tablist_buy.setNextTabList(&pow->tablist);
			vendor->tablist_sell.setNextTabList(&pow->tablist);
			chr->tablist.setNextTabList(&pow->tablist);
			missionlog->setNextTabList(&pow->tablist);
			salvage->tablist.setNextTabList(&pow->tablist);
			kiln->tablist.setNextTabList(&pow->tablist);

			// NOTE trove and vendor are only visible with inventory, so we don't need to handle them here
			if (chr->visible) {
				pow->tablist.setPrevTabList(&chr->tablist);
			}
			else if (missionlog->visible) {
				pow->tablist.setPrevTabList(&missionlog->tablist);
			}
		}
	}

	// trove and vendor always start locked
	if (!trove->visible) trove->tablist.lock();
	if (!kiln->visible) kiln->tablist.lock();
	if (!salvage->visible) salvage->tablist.lock();
	if (!vendor->visible) {
		vendor->tablist.lock();
		vendor->tablist_buy.lock();
		vendor->tablist_sell.lock();
	}

	// inventory always starts unlocked
	if (!inv->visible) inv->tablist.unlock();

	// position the drag hover icon depending on the last key press
	if (!act_drag_hover && (inpt->pressing[ACTIONBAR_BACK] || inpt->pressing[ACTIONBAR_FORWARD] || inpt->pressing[ACTIONBAR]))
		act_drag_hover = true;
	else if (act_drag_hover && (inpt->pressing[LEFT] || inpt->pressing[RIGHT] || inpt->pressing[UP] || inpt->pressing[DOWN]) && !(inpt->pressing[ACTIONBAR_BACK] || inpt->pressing[ACTIONBAR_FORWARD]))
		act_drag_hover = false;

	// don't allow dropping actionbar items in other menus
	if (keyboard_dragging && drag_src == DRAG_SRC_ACTIONBAR) {
		inpt->lock[ACCEPT] = true;
	}
}
void MenuManager::logic() {
	ItemStack stack;

	hull->update(stats->hull, stats->get(STAT_HULL_MAX), inpt->mouse, msg->get("Hull: %d", stats->hull));
	psi->update(stats->psi, stats->get(STAT_PSI_MAX), inpt->mouse, msg->get("PSI: %d", stats->psi));
	capacitor->update(stats->capacitor, stats->get(STAT_CAPACITOR_MAX), inpt->mouse, msg->get("Capacitor: %d", stats->capacitor));
	shield->update(stats->shield, stats->get(STAT_SHIELD_MAX), inpt->mouse, msg->get("Shield: %d", stats->shield));
	limitbreak->update(stats->limitbreak,100,inpt->mouse,msg->get("Limit: %d/%d", stats->limitbreak, 100));
	if (stats->generation == static_cast<int>(stats->soul_table.size()))
		soul->update((stats->soul - stats->soul_table[stats->generation-1]), (stats->soul - stats->soul_table[stats->generation-1]), inpt->mouse, msg->get("Soul: %d", stats->soul));
	else
		soul->update((stats->soul - stats->soul_table[stats->generation-1]), (stats->soul_table[stats->generation] - stats->soul_table[stats->generation-1]), inpt->mouse, msg->get("Soul: %d/%d", stats->soul, stats->soul_table[stats->generation]));

	// when selecting item quantities, don't process other menus
	if (num_picker->visible) {
		num_picker->logic();

		if (num_picker->confirm_clicked) {
			// start dragging items
			// removes the desired quantity from the source stack

			if (drag_src == DRAG_SRC_INVENTORY) {
				drag_stack.quantity = num_picker->getValue();
				inv->removeFromPrevSlot(drag_stack.quantity);
			}
			else if (drag_src == DRAG_SRC_VENDOR) {
				drag_stack.quantity = num_picker->getValue();
				vendor->removeFromPrevSlot(drag_stack.quantity);
			}
			else if (drag_src == DRAG_SRC_TROVE) {
				drag_stack.quantity = num_picker->getValue();
				trove->removeFromPrevSlot(drag_stack.quantity);
			}
			else if (drag_src == DRAG_SRC_SALVAGE) {
				drag_stack.quantity = num_picker->getValue();
				salvage->removeFromPrevSlot(drag_stack.quantity);
			}
			else if (drag_src == DRAG_SRC_KILN) {
				drag_stack.quantity = num_picker->getValue();
				kiln->removeFromPrevSlot(drag_stack.quantity);
			}

			num_picker->confirm_clicked = false;
			num_picker->visible = false;
			if (!NO_MOUSE) {
				sticky_dragging = true;
			}
		}
		else {
			pause = true;
			return;
		}
	}

	if (NO_MOUSE)
		handleKeyboardNavigation();

	// Stop attacking if the cursor is inside an interactable menu
	if (stats->attacking) {
		if (isWithinRect(act->window_area, inpt->mouse) ||
			(book->visible && isWithinRect(book->window_area, inpt->mouse)) ||
			(chr->visible && isWithinRect(chr->window_area, inpt->mouse)) ||
			(inv->visible && isWithinRect(inv->window_area, inpt->mouse)) ||
			(vendor->visible && isWithinRect(vendor->window_area, inpt->mouse)) ||
			(pow->visible && isWithinRect(pow->window_area, inpt->mouse)) ||
			(missionlog->visible && isWithinRect(missionlog->window_area, inpt->mouse)) ||
			(talker->visible && isWithinRect(talker->window_area, inpt->mouse)) ||
			(trove->visible && isWithinRect(trove->window_area, inpt->mouse)) ||
			(salvage->visible && isWithinRect(salvage->window_area, inpt->mouse)) ||
			(kiln->visible && isWithinRect(kiln->window_area, inpt->mouse)))
		{
			inpt->pressing[MAIN1] = false;
			inpt->pressing[MAIN2] = false;
		}
	}

	book->logic();
	effects->logic();
	act->logic();
	hudlog->logic();
	enemy->logic();
	chr->logic();
	inv->logic();
	vendor->logic();
	pow->logic();
	missionlog->logic();
	talker->logic();
	trove->logic();
	salvage->logic();
	kiln->logic();

	if (DEV_MODE) {
		devhud->visible = DEV_HUD;
		devhud->logic();
		devconsole->logic();
	}

	if (chr->checkUpgrade() || stats->generation_up) {
		// apply equipment and max hull/mp
		inv->applyEquipment();
		stats->hull = stats->get(STAT_HULL_MAX);
		stats->shield = stats->get(STAT_SHIELD_MAX);
		stats->psi = stats->get(STAT_PSI_MAX);
		stats->capacitor = stats->get(STAT_CAPACITOR_MAX);
		stats->generation_up = false;
	}

	// only allow the vendor window to be open if the inventory is open
	if (vendor->visible && !(inv->visible)) {
		snd->play(vendor->sfx_close);
		closeAll();
	}

	// context-sensistive help tooltip in inventory menu
	if (inv->visible && vendor->visible) {
		inv->inv_ctrl = INV_CTRL_VENDOR;
	}
	else if (inv->visible && trove->visible) {
		inv->inv_ctrl = INV_CTRL_TROVE;
	}
	else if (inv->visible && kiln->visible) {
		inv->inv_ctrl = INV_CTRL_KILN;
	}
	else if (inv->visible && salvage->visible) {
		inv->inv_ctrl = INV_CTRL_SALVAGE;
	}
	else {
		inv->inv_ctrl = INV_CTRL_NONE;
	}

	if (!inpt->pressing[INVENTORY] && !inpt->pressing[POWERS] && !inpt->pressing[CHARACTER] && !inpt->pressing[LOG] && !inpt->pressing[DEVELOPER_MENU])
		key_lock = false;

	if (DEV_MODE && devconsole->inputFocus())
		key_lock = true;

	// handle npc action menu
	if (npc->visible) {
		npc->logic();
	}

	// cancel dragging and defocus menu tablists
	if (!key_lock && inpt->pressing[CANCEL] && !inpt->lock[CANCEL] && !stats->corpse) {
		if (keyboard_dragging || mouse_dragging) {
			inpt->lock[CANCEL] = true;
			resetDrag();
		}
		for (size_t i=0; i<menus.size(); i++) {
			TabList *tablist = menus[i]->getCurrentTabList();
			if (tablist) {
					inpt->lock[CANCEL] = true;

				menus[i]->defocusTabLists();
			}
		}
	}

	// exit menu toggle
	if ((!key_lock && !mouse_dragging && !keyboard_dragging) && !(stats->corpse && stats->permadeath)) {
		if (inpt->pressing[CANCEL] && !inpt->lock[CANCEL]) {
			inpt->lock[CANCEL] = true;
			key_lock = true;
			if (act->twostep_slot != -1) {
				act->twostep_slot = -1;
			}
			else if (DEV_MODE && devconsole->visible) {
				devconsole->visible = false;
				devconsole->reset();
			}
			else if (menus_open) {
				closeAll();
			}
			else {
				exit->visible = !exit->visible;
			}
		}
	}

	if (exit->visible) {
		exit->logic();
		if (exit->isExitRequested()) {
			done = true;
		}
	}
	else {
		bool clicking_character = false;
		bool clicking_inventory = false;
		bool clicking_powers = false;
		bool clicking_log = false;

		// check if mouse-clicking a menu button
		act->checkMenu(clicking_character, clicking_inventory, clicking_powers, clicking_log);

		// inventory menu toggle
		if ((inpt->pressing[INVENTORY] && !key_lock && !mouse_dragging && !keyboard_dragging) || clicking_inventory) {
			key_lock = true;
			if (inv->visible) {
				snd->play(inv->sfx_close);
				closeRight();
			}
			else {
				closeRight();
				act->requires_attention[MENU_INVENTORY] = false;
				inv->visible = true;
				snd->play(inv->sfx_open);
			}

		}

		// powers menu toggle
		if (((inpt->pressing[POWERS] && !key_lock && !mouse_dragging && !keyboard_dragging) || clicking_powers) && !stats->transformed) {
			key_lock = true;
			if (pow->visible) {
				snd->play(pow->sfx_close);
				closeRight();
			}
			else {
				closeRight();
				act->requires_attention[MENU_POWERS] = false;
				pow->visible = true;
				snd->play(pow->sfx_open);
			}
		}

		// character menu toggleggle
		if ((inpt->pressing[CHARACTER] && !key_lock && !mouse_dragging && !keyboard_dragging) || clicking_character) {
			key_lock = true;
			if (chr->visible) {
				snd->play(chr->sfx_close);
				closeLeft();
			}
			else {
				closeLeft();
				act->requires_attention[MENU_CHARACTER] = false;
				chr->visible = true;
				snd->play(chr->sfx_open);
				// Make sure the stat list isn't scrolled when we open the character menu
				inpt->resetScroll();
			}
		}

		// log menu toggle
		if ((inpt->pressing[LOG] && !key_lock && !mouse_dragging && !keyboard_dragging) || clicking_log) {
			key_lock = true;
			if (missionlog->visible) {
				snd->play(missionlog->sfx_close);
				closeLeft();
			}
			else {
				closeLeft();
				act->requires_attention[MENU_LOG] = false;
				missionlog->visible = true;
				snd->play(missionlog->sfx_open);
				// Make sure the log isn't scrolled when we open the log menu
				inpt->resetScroll();
			}
		}

		//developer console
		if (DEV_MODE && inpt->pressing[DEVELOPER_MENU] && !key_lock && !mouse_dragging && !keyboard_dragging) {
			key_lock = true;
			if (devconsole->visible) {
				closeAll();
			}
			else {
				closeAll();
				devconsole->visible = true;
			}
		}
	}

	bool console_open = DEV_MODE && devconsole->visible;
	menus_open = (inv->visible || pow->visible || chr->visible || missionlog->visible || vendor->visible || talker->visible || npc->visible || book->visible || console_open);
	pause = (MENUS_PAUSE && menus_open) || exit->visible || console_open || book->visible;

	if (stats->alive) {

		// handle right-click
		if (!mouse_dragging && inpt->pressing[MAIN2] && !inpt->lock[MAIN2]) {
			// exit menu
			if (exit->visible && isWithinRect(exit->window_area, inpt->mouse)) {
				inpt->lock[MAIN2] = true;
			}

			// book menu
			if (book->visible && isWithinRect(book->window_area, inpt->mouse)) {
				inpt->lock[MAIN2] = true;
			}

			// activate inventory item
			else if (inv->visible && isWithinRect(inv->window_area, inpt->mouse)) {
				inpt->lock[MAIN2] = true;
				if (isWithinRect(inv->carried_area, inpt->mouse)) {
					inv->activate(inpt->mouse);
				}
			}
		}

		// handle left-click for book menu first
		if (!mouse_dragging && inpt->pressing[MAIN1] && !inpt->lock[MAIN1]) {
			if (book->visible && isWithinRect(book->window_area, inpt->mouse)) {
				inpt->lock[MAIN1] = true;
			}
		}

		// handle left-click
		if (!mouse_dragging && inpt->pressing[MAIN1] && !inpt->lock[MAIN1]) {
			resetDrag();

			for (size_t i=0; i<menus.size(); ++i) {
				if (!menus[i]->visible || !isWithinRect(menus[i]->window_area, inpt->mouse)) {
					menus[i]->defocusTabLists();
				}
			}

			// exit menu
			if (exit->visible && isWithinRect(exit->window_area, inpt->mouse)) {
				inpt->lock[MAIN1] = true;
			}


			if (chr->visible && isWithinRect(chr->window_area, inpt->mouse)) {
				inpt->lock[MAIN1] = true;
			}

			if (vendor->visible && isWithinRect(vendor->window_area,inpt->mouse)) {
				inpt->lock[MAIN1] = true;
				if (inpt->pressing[CTRL]) {
					// buy item from a vendor
					stack = vendor->click(inpt->mouse);
					if (!inv->buy(stack, vendor->getTab(), false)) {
						vendor->itemReturn(inv->drop_stack.front());
						inv->drop_stack.pop();
					}
				}
				else {
					// start dragging a vendor item
					drag_stack = vendor->click(inpt->mouse);
					if (!drag_stack.empty()) {
						mouse_dragging = true;
						drag_src = DRAG_SRC_VENDOR;
					}
					if (drag_stack.quantity > 1 && (inpt->pressing[SHIFT] || NO_MOUSE || inpt->touch_locked)) {
						int max_quantity = std::min(inv->getMaxPurchasable(drag_stack.item, vendor->getTab()), drag_stack.quantity);
						if (max_quantity >= 1) {
							num_picker->setValueBounds(1, max_quantity);
							num_picker->visible = true;
						}
						else {
							drag_stack.clear();
							resetDrag();
						}
					}
				}
			}

			if (trove->visible && isWithinRect(trove->window_area,inpt->mouse)) {
				inpt->lock[MAIN1] = true;
				if (inpt->pressing[CTRL]) {
					// take an item from the trove
					stack = trove->click(inpt->mouse);
					if (!inv->add(stack, CARRIED, -1, true, true)) {
						trove->itemReturn(inv->drop_stack.front());
						inv->drop_stack.pop();
					}
					trove->updated = true;
				}
				else {
					// start dragging a trove item
					drag_stack = trove->click(inpt->mouse);
					if (!drag_stack.empty()) {
						mouse_dragging = true;
						drag_src = DRAG_SRC_TROVE;
					}
					if (drag_stack.quantity > 1 && (inpt->pressing[SHIFT] || NO_MOUSE || inpt->touch_locked)) {
						num_picker->setValueBounds(1, drag_stack.quantity);
						num_picker->visible = true;
					}
				}
			}
			if (salvage->visible && isWithinRect(salvage->window_area,inpt->mouse)) {
				inpt->lock[MAIN1] = true;
				if (inpt->pressing[CTRL]) {
					// take an item from the salvage
					stack = salvage->click(inpt->mouse);
					if (!inv->add(stack, CARRIED, -1, true, true)) {
						salvage->itemReturn(inv->drop_stack.front());
						inv->drop_stack.pop();
					}
					salvage->updated = true;
				}
				else { 	// start dragging a salvage item
					drag_stack = salvage->click(inpt->mouse);
					if (!drag_stack.empty()) {
						mouse_dragging = true;
						drag_src = DRAG_SRC_SALVAGE;
					}
					if (drag_stack.quantity > 1 && (inpt->pressing[SHIFT] || NO_MOUSE || inpt->touch_locked)) {
						num_picker->setValueBounds(1, drag_stack.quantity);
						num_picker->visible = true;
					}
				}
			}
			if (kiln->visible && isWithinRect(kiln->window_area,inpt->mouse)) {
				inpt->lock[MAIN1] = true;
				if (inpt->pressing[CTRL]) {
					// take an item from the kiln
					stack = kiln->click(inpt->mouse);
					if (!inv->add(stack, CARRIED, -1, true, true)) {
						kiln->itemReturn(inv->drop_stack.front());
						inv->drop_stack.pop();
					}
					kiln->updated = true;
				}
				else { // start dragging a kiln item
					drag_stack = kiln->click(inpt->mouse);
					if (!drag_stack.empty()) {
						mouse_dragging = true;
						drag_src = DRAG_SRC_KILN;
					}
					if (drag_stack.quantity > 1 && (inpt->pressing[SHIFT] || NO_MOUSE || inpt->touch_locked)) {
						num_picker->setValueBounds(1, drag_stack.quantity);
						num_picker->visible = true;
					}
				}
			}

			if (missionlog->visible && isWithinRect(missionlog->window_area,inpt->mouse)) {
				inpt->lock[MAIN1] = true;
			}

			// pick up an inventory item
			if (inv->visible && isWithinRect(inv->window_area,inpt->mouse)) {
				if (inpt->pressing[CTRL]) {
					inpt->lock[MAIN1] = true;
					stack = inv->click(inpt->mouse);
					if (trove->visible) {
						if (!trove->add(stack, -1, true)) {
							inv->itemReturn(trove->drop_stack.front());
							trove->drop_stack.pop();
						}
					}
					else {
						// The vendor could have a limited amount of currency in the future. It will be tested here.
						if ((SELL_WITHOUT_VENDOR || vendor->visible) && inv->sell(stack)) {
							vendor->setTab(VENDOR_SELL);
							vendor->add(stack);
						}
						else {
							inv->itemReturn(stack);
						}
					}
				}
				else {
					inpt->lock[MAIN1] = true;
					drag_stack = inv->click(inpt->mouse);
					if (!drag_stack.empty()) {
						mouse_dragging = true;
						drag_src = DRAG_SRC_INVENTORY;
					}
					if (drag_stack.quantity > 1 && (inpt->pressing[SHIFT] || NO_MOUSE || inpt->touch_locked)) {
						num_picker->setValueBounds(1, drag_stack.quantity);
						num_picker->visible = true;
					}
				}
			}
			// pick up a power
			if (pow->visible && isWithinRect(pow->window_area,inpt->mouse)) {
				inpt->lock[MAIN1] = true;

				// check for unlock/dragging
				drag_power = pow->click(inpt->mouse);
				if (drag_power > 0) {
					mouse_dragging = true;
					keyboard_dragging = false;
					drag_src = DRAG_SRC_POWERS;
				}
			}
			// action bar
			if (!inpt->touch_locked && (act->isWithinSlots(inpt->mouse) || act->isWithinMenus(inpt->mouse))) {
				inpt->lock[MAIN1] = true;

				// ctrl-click action bar to clear that slot
				if (inpt->pressing[CTRL]) {
					act->remove(inpt->mouse);
				}
				// allow drag-to-rearrange action bar
				else if (!act->isWithinMenus(inpt->mouse)) {
					drag_power = act->checkDrag(inpt->mouse);
					if (drag_power > 0) {
						mouse_dragging = true;
						drag_src = DRAG_SRC_ACTIONBAR;
					}
				}

				// else, clicking action bar to use a power?
				// this check is done by GameEngine when calling Avatar::logic()


			}
		}

		// highlight matching inventory slots based on what we're dragging
		if (inv->visible && (mouse_dragging || keyboard_dragging)) {
			inv->inventory[EQUIPMENT].highlightMatching(items->items[drag_stack.item].type);
		}

		// handle dropping
		if (mouse_dragging && ((sticky_dragging && inpt->pressing[MAIN1] && !inpt->lock[MAIN1]) || (!sticky_dragging && !inpt->pressing[MAIN1]))) {
			if (sticky_dragging) {
				inpt->lock[MAIN1] = true;
				sticky_dragging = false;
			}

			// putting a power on the Action Bar
			if (drag_src == DRAG_SRC_POWERS) {
				if (act->isWithinSlots(inpt->mouse)) {
					act->drop(inpt->mouse, drag_power, 0);
				}
			}

			// rearranging the action bar
			else if (drag_src == DRAG_SRC_ACTIONBAR) {
				if (act->isWithinSlots(inpt->mouse)) {
					act->drop(inpt->mouse, drag_power, 1);
					// for locked slots forbid power dropping
				}
				else if (act->locked[act->drag_prev_slot]) {
					act->hotkeys[act->drag_prev_slot] = drag_power;
				}
				drag_power = 0;
			}

			// rearranging inventory or dropping items
			else if (drag_src == DRAG_SRC_INVENTORY) {

				if (inv->visible && isWithinRect(inv->window_area, inpt->mouse)) {
					inv->drop(inpt->mouse, drag_stack);
				}
				else if (act->isWithinSlots(inpt->mouse)) {
					// The action bar is not storage!
					inv->itemReturn(drag_stack);
					inv->applyEquipment();

					// put an item with a power on the action bar
					if (items->items[drag_stack.item].power != 0) {
						act->drop(inpt->mouse, items->items[drag_stack.item].power, false);
					}
				}
				else if (vendor->visible && isWithinRect(vendor->window_area, inpt->mouse)) {
					if (inv->sell( drag_stack)) {
						vendor->setTab(VENDOR_SELL);
						vendor->add( drag_stack);
					}
					else {
						inv->itemReturn(drag_stack);
					}
				}
				else if (trove->visible && isWithinRect(trove->window_area, inpt->mouse)) {
					trove->stock.drag_prev_slot = -1;
					if (!trove->drop(inpt->mouse, drag_stack)) {
						inv->itemReturn(trove->drop_stack.front());
						trove->drop_stack.pop();
					}
				}
				else if (kiln->visible && isWithinRect(kiln->window_area, inpt->mouse)) {
					kiln->oven.drag_prev_slot = -1;
					if (!kiln->drop(inpt->mouse, drag_stack)) {
						inv->itemReturn(kiln->drop_stack.front());
						kiln->drop_stack.pop();
					}
				}
				else if (salvage->visible && isWithinRect(salvage->window_area, inpt->mouse)) {
					salvage->pot.drag_prev_slot = -1;
					if (!salvage->drop(inpt->mouse, drag_stack)) {
						inv->itemReturn(salvage->drop_stack.front());
						salvage->drop_stack.pop();
					}
				}
				else {
					// if dragging and the source was inventory, drop item to the floor

					// mission items cannot be dropped
					if (!items->items[drag_stack.item].mission_item) {
						drop_stack.push(drag_stack);
					}
					else {
						pc->logMsg(msg->get("This item can not be dropped."), true);
						items->playSound(drag_stack.item);

						inv->itemReturn(drag_stack);
					}
				}
				inv->clearHighlight();
			}

			else if (drag_src == DRAG_SRC_VENDOR) {

				// dropping an item from vendor (we only allow to drop into the carried area)
				if (inv->visible && isWithinRect(inv->window_area, inpt->mouse)) {
					if (!inv->buy(drag_stack, vendor->getTab(), true)) {
						vendor->itemReturn(inv->drop_stack.front());
						inv->drop_stack.pop();
					}
				}
				else {
					vendor->itemReturn(drag_stack);
				}
			}

			else if (drag_src == DRAG_SRC_TROVE) {

				// dropping an item from trove (we only allow to drop into the carried area)
				if (inv->visible && isWithinRect(inv->window_area, inpt->mouse)) {
					if (!inv->drop(inpt->mouse, drag_stack)) {
						trove->itemReturn(inv->drop_stack.front());
						inv->drop_stack.pop();
					}
					trove->updated = true;
				}
				else if (trove->visible && isWithinRect(trove->window_area, inpt->mouse)) {
					if (!trove->drop(inpt->mouse,drag_stack)) {
						drop_stack.push(trove->drop_stack.front());
						trove->drop_stack.pop();
					}
				}
				else {
					drop_stack.push(drag_stack);
					trove->updated = true;
				}
			}
			else if (drag_src == DRAG_SRC_KILN) {

				// dropping an item from kiln (we only allow to drop into the carried area)
				if (inv->visible && isWithinRect(inv->window_area, inpt->mouse)) {
					if (!inv->drop(inpt->mouse, drag_stack)) {
						kiln->itemReturn(inv->drop_stack.front());
						inv->drop_stack.pop();
					}
					kiln->updated = true;
				}
				else if (kiln->visible && isWithinRect(kiln->window_area, inpt->mouse)) {
					if (!kiln->drop(inpt->mouse,drag_stack)) {
						drop_stack.push(kiln->drop_stack.front());
						kiln->drop_stack.pop();
					}
				}
				else {
					drop_stack.push(drag_stack);
					kiln->updated = true;
				}
			}
			else if (drag_src == DRAG_SRC_SALVAGE) {

				// dropping an item from salvage (we only allow to drop into the carried area)
				if (inv->visible && isWithinRect(inv->window_area, inpt->mouse)) {
					if (!inv->drop(inpt->mouse, drag_stack)) {
						salvage->itemReturn(inv->drop_stack.front());
						inv->drop_stack.pop();
					}
					salvage->updated = true;
				}
				else if (salvage->visible && isWithinRect(salvage->window_area, inpt->mouse)) {
					if (!salvage->drop(inpt->mouse,drag_stack)) {
						drop_stack.push(salvage->drop_stack.front());
						salvage->drop_stack.pop();
					}
				}
				else {
					drop_stack.push(drag_stack);
					salvage->updated = true;
				}
			}

			drag_stack.clear();
			drag_power = 0;
			drag_src = 0;
			mouse_dragging = false;
		}
		if (NO_MOUSE)
			dragAndDropWithKeyboard();
	}
	else {
		if (mouse_dragging || keyboard_dragging) {
			resetDrag();
		}
	}

	// return items that are currently begin dragged when returning to title screen or exiting game
	if (done || inpt->done) {
		resetDrag();
	}

	// handle equipment changes affecting hero stats
	if (inv->changed_equipment) {
		inv->applyEquipment();
		// the equipment flags get reset in GameStatePlay
	}

	if (drag_icon && !(mouse_dragging || keyboard_dragging)) {
		delete drag_icon;
		drag_icon = NULL;
	}
}

void MenuManager::dragAndDropWithKeyboard() {
	// inventory menu

	if (inv->visible && inv->getCurrentTabList() && drag_src != DRAG_SRC_ACTIONBAR) {
		int slot_index = inv->getCurrentTabList()->getCurrent();
		CLICK_TYPE slotClick;
		Point src_slot;
		WidgetSlot * inv_slot;

		if (slot_index < inv->getEquippedCount())
			inv_slot = inv->inventory[EQUIPMENT].slots[slot_index];
		else
			inv_slot = inv->inventory[CARRIED].slots[slot_index - inv->getEquippedCount()];

		src_slot.x = inv_slot->pos.x;
		src_slot.y = inv_slot->pos.y;
		slotClick = inv_slot->checkClick();

		// pick up item
		if (slotClick == CHECKED && drag_stack.empty()) {
			drag_stack = inv->click(src_slot);
			if (!drag_stack.empty()) {
				keyboard_dragging = true;
				drag_src = DRAG_SRC_INVENTORY;
			}
			if (drag_stack.quantity > 1) {
				num_picker->setValueBounds(1, drag_stack.quantity);
				num_picker->visible = true;
			}
		}
		// rearrange item
		else if (slotClick == CHECKED && !drag_stack.empty()) {
			inv->drop(src_slot, drag_stack);
			inv_slot->checked = false;
			drag_src = 0;
			drag_stack.clear();
			keyboard_dragging = false;
			sticky_dragging = false;
		}
		// sell, trove, or use item
		else if (slotClick == ACTIVATED && !drag_stack.empty()) {
			if (vendor->visible && inv->sell(drag_stack)) {
				vendor->setTab(VENDOR_SELL);
				vendor->add(drag_stack);
			}
			else if (trove->visible) {
				if (!trove->add(drag_stack, -1, true)) {
					inv->itemReturn(trove->drop_stack.front());
					trove->drop_stack.pop();
				}
			}
			else {
				inv->itemReturn(drag_stack);
				if (!vendor->visible && !trove->visible)
					inv->activate(src_slot);
			}
			inv->clearHighlight();
			drag_src = 0;
			drag_stack.clear();
			keyboard_dragging = false;
			sticky_dragging = false;
		}
	}

	// vendor menu
	if (vendor->visible && vendor->getCurrentTabList() && vendor->getCurrentTabList() != (&vendor->tablist) && drag_src != DRAG_SRC_ACTIONBAR) {
		int slot_index = vendor->getCurrentTabList()->getCurrent();
		CLICK_TYPE slotClick;
		Point src_slot;
		WidgetSlot * vendor_slot;

		if (vendor->getTab() == VENDOR_SELL)
			vendor_slot = vendor->stock[VENDOR_SELL].slots[slot_index];
		else
			vendor_slot = vendor->stock[VENDOR_BUY].slots[slot_index];

		src_slot.x = vendor_slot->pos.x;
		src_slot.y = vendor_slot->pos.y;
		slotClick = vendor_slot->checkClick();

		// buy item
		if (slotClick == CHECKED && drag_stack.empty()) {
			drag_stack = vendor->click(src_slot);
			if (!drag_stack.empty()) {
				keyboard_dragging = true;
				drag_src = DRAG_SRC_VENDOR;
				vendor->lockTabControl();
			}
			if (drag_stack.quantity > 1) {
				int max_quantity = std::min(inv->getMaxPurchasable(drag_stack.item, vendor->getTab()), drag_stack.quantity);
				if (max_quantity >= 1) {
					num_picker->setValueBounds(1, max_quantity);
					num_picker->visible = true;
				}
				else {
					drag_stack.clear();
					resetDrag();
				}
			}
		}

		// if we selected a single item buy it imediately
		// otherwise, wait until we get a result from num_picker
		if (vendor_slot->checked && !drag_stack.empty() && !num_picker->visible) {
			if (!inv->buy(drag_stack, vendor->getTab(), false)) {
				vendor->itemReturn(inv->drop_stack.front());
				inv->drop_stack.pop();
			}
			drag_src = 0;
			drag_stack.clear();
			keyboard_dragging = false;
			sticky_dragging = false;
			vendor_slot->checked = false;
			vendor->unlockTabControl();
		}
	}

	// trove menu
	if (trove->visible && trove->getCurrentTabList() && drag_src != DRAG_SRC_ACTIONBAR) {
		int slot_index = trove->getCurrentTabList()->getCurrent();
		CLICK_TYPE slotClick = trove->stock.slots[slot_index]->checkClick();
		Point src_slot(trove->stock.slots[slot_index]->pos.x, trove->stock.slots[slot_index]->pos.y);

		// pick up item
		if (slotClick == CHECKED && drag_stack.empty()) {
			drag_stack = trove->click(src_slot);
			if (!drag_stack.empty()) {
				keyboard_dragging = true;
				drag_src = DRAG_SRC_TROVE;
			}
			if (drag_stack.quantity > 1) {
				num_picker->setValueBounds(1, drag_stack.quantity);
				num_picker->visible = true;
			}
		}
		// rearrange item
		else if (slotClick == CHECKED && !drag_stack.empty()) {
			trove->stock.slots[slot_index]->checked = false;
			if (!trove->drop(src_slot, drag_stack)) {
				drop_stack.push(trove->drop_stack.front());
				trove->drop_stack.pop();
			}
			drag_src = 0;
			drag_stack.clear();
			keyboard_dragging = false;
			sticky_dragging = false;
		}
		// send to inventory
		else if (slotClick == ACTIVATED && !drag_stack.empty()) {
			if (!inv->add(drag_stack, CARRIED, -1, true, true)) {
				trove->itemReturn(inv->drop_stack.front());
				inv->drop_stack.pop();
			}
			drag_src = 0;
			drag_stack.clear();
			keyboard_dragging = false;
			sticky_dragging = false;
			trove->updated = true;
		}
	}

	// powers menu
	if (pow->visible && pow->isTabListSelected() && drag_src != DRAG_SRC_ACTIONBAR) {
		int slot_index = pow->getSelectedCellIndex();
		CLICK_TYPE slotClick = pow->slots[slot_index]->checkClick();

		if (slotClick == CHECKED) {
			Point src_slot(pow->slots[slot_index]->pos.x, pow->slots[slot_index]->pos.y);
			// check for unlock/dragging
			drag_power = pow->click(src_slot);
			if (drag_power > 0) {
				keyboard_dragging = true;
				drag_src = DRAG_SRC_POWERS;
			}
			else {
				pow->slots[slot_index]->checked = false;
			}
		}
		// clear power dragging if power slot was pressed twice
		else if (slotClick == ACTIVATED) {
			if (drag_power > 0) {
				pow->upgradeByCell(slot_index);
			}
			drag_src = 0;
			drag_power = 0;
			keyboard_dragging = false;
		}
	}

	// actionbar
	if (act->getCurrentTabList() && static_cast<unsigned>(act->getCurrentTabList()->getCurrent()) < act->slots.size()) {
		int slot_index = act->getCurrentTabList()->getCurrent();
		CLICK_TYPE slotClick = act->slots[slot_index]->checkClick();
		Point dest_slot = act->getSlotPos(slot_index);

		// pick up power
		if (slotClick == CHECKED && drag_stack.empty() && drag_power == 0) {
			drag_power = act->checkDrag(dest_slot);
			if (drag_power > 0) {
				keyboard_dragging = true;
				drag_src = DRAG_SRC_ACTIONBAR;
			}
			else {
				act->slots[slot_index]->deactivate();
			}
		}
		// drop power/item from other menu
		else if (slotClick == CHECKED && drag_src != DRAG_SRC_ACTIONBAR && (!drag_stack.empty() || drag_power > 0)) {
			if (drag_src == DRAG_SRC_POWERS) {
				act->drop(dest_slot, drag_power, 0);
				pow->slots[slot_index]->checked = false;
			}
			else if (drag_src == DRAG_SRC_INVENTORY) {
				if (slot_index< inv->getEquippedCount())
					inv->inventory[EQUIPMENT].slots[slot_index]->checked = false;
				else
					inv->inventory[CARRIED].slots[slot_index - inv->getEquippedCount()]->checked = false;

				if (items->items[drag_stack.item].power != 0) {
					act->drop(dest_slot, items->items[drag_stack.item].power, false);
				}
			}
			act->slots[slot_index]->checked = false;
			resetDrag();
			inv->applyEquipment();
		}
		// rearrange actionbar
		else if ((slotClick == CHECKED || slotClick == ACTIVATED) && drag_src == DRAG_SRC_ACTIONBAR && drag_power > 0) {
			if (slotClick == CHECKED) act->slots[slot_index]->checked = false;
			act->drop(dest_slot, drag_power, 1);
			drag_src = 0;
			drag_power = 0;
			keyboard_dragging = false;
			inpt->lock[ACCEPT] = false;
		}
	}
}

void MenuManager::resetDrag() {
	if (drag_src == DRAG_SRC_VENDOR) {
		vendor->itemReturn(drag_stack);
		vendor->unlockTabControl();
		inv->clearHighlight();
	}
	else if (drag_src == DRAG_SRC_TROVE) {
		trove->itemReturn(drag_stack);
		inv->clearHighlight();
	}
	else if (drag_src == DRAG_SRC_KILN) {
		kiln->itemReturn(drag_stack);
		inv->clearHighlight();
	}
	else if (drag_src == DRAG_SRC_SALVAGE) {
		salvage->itemReturn(drag_stack);
		inv->clearHighlight();
	}
	else if (drag_src == DRAG_SRC_INVENTORY) {
		inv->itemReturn(drag_stack);
		inv->clearHighlight();
	}
	else if (drag_src == DRAG_SRC_ACTIONBAR) act->actionReturn(drag_power);
	drag_src = 0;
	drag_stack.clear();
	drag_power = 0;

	if (keyboard_dragging && DRAG_SRC_ACTIONBAR) {
		inpt->lock[ACCEPT] = false;
	}

	if (drag_icon) {
		delete drag_icon;
		drag_icon = NULL;
	}

	vendor->stock[VENDOR_BUY].drag_prev_slot = -1;
	vendor->stock[VENDOR_SELL].drag_prev_slot = -1;
	trove->stock.drag_prev_slot = -1;
	kiln->oven.drag_prev_slot = -1;
	salvage->pot.drag_prev_slot = -1;
	inv->drag_prev_src = -1;
	inv->inventory[EQUIPMENT].drag_prev_slot = -1;
	inv->inventory[CARRIED].drag_prev_slot = -1;

	keyboard_dragging = false;
	mouse_dragging = false;
	sticky_dragging = false;
}

void MenuManager::render() {
	// render the devhud under other menus
	if (DEV_MODE && SHOW_HUD) {
		devhud->render();
	}

	if (!SHOW_HUD) {
		// if the hud is disabled, only show a few necessary menus

		// exit menu
		menus[9]->render();

		// dev console
		if (DEV_MODE)
			devconsole->render();

		return;
	}

	bool hudlog_overlapped = false;
	if (chr->visible && rectsOverlap(hudlog->window_area, chr->window_area)) {
		hudlog_overlapped = true;
	}
	if (missionlog->visible && rectsOverlap(hudlog->window_area, missionlog->window_area)) {
		hudlog_overlapped = true;
	}
	if (inv->visible && rectsOverlap(hudlog->window_area, inv->window_area)) {
		hudlog_overlapped = true;
	}
	if (pow->visible && rectsOverlap(hudlog->window_area, pow->window_area)) {
		hudlog_overlapped = true;
	}
	if (vendor->visible && rectsOverlap(hudlog->window_area, vendor->window_area)) {
		hudlog_overlapped = true;
	}
	if (trove->visible && rectsOverlap(hudlog->window_area, trove->window_area)) {
		hudlog_overlapped = true;
	}
	if (kiln->visible && rectsOverlap(hudlog->window_area, kiln->window_area)) {
		hudlog_overlapped = true;
	}
	if (salvage->visible && rectsOverlap(hudlog->window_area, salvage->window_area)) {
		hudlog_overlapped = true;
	}

	for (size_t i=0; i<menus.size(); i++) {
		if (menus[i] == hudlog && hudlog_overlapped && !hudlog->hide_overlay) {
			continue;
		}
		
		menus[i]->render();
	}

	if (hudlog_overlapped && !hudlog->hide_overlay) {
		hudlog->renderOverlay();
	}

	if (!num_picker->visible && !mouse_dragging && !sticky_dragging) {
		if (NO_MOUSE || TOUCHSCREEN)
			handleKeyboardTooltips();
		else {
			TooltipData tip_new;

			// Find tooltips depending on mouse position
			if (!book->visible) {
				if (chr->visible && isWithinRect(chr->window_area,inpt->mouse)) {
					tip_new = chr->checkTooltip();
				}
				if (vendor->visible && isWithinRect(vendor->window_area,inpt->mouse)) {
					tip_new = vendor->checkTooltip(inpt->mouse);
				}
				if (trove->visible && isWithinRect(trove->window_area,inpt->mouse)) {
					tip_new = trove->checkTooltip(inpt->mouse);
				}
				if (kiln->visible && isWithinRect(kiln->window_area,inpt->mouse)) {
					tip_new = kiln->checkTooltip(inpt->mouse);
				}
				if (salvage->visible && isWithinRect(salvage->window_area,inpt->mouse)) {
					tip_new = salvage->checkTooltip(inpt->mouse);
				}
				if (pow->visible && isWithinRect(pow->window_area,inpt->mouse)) {
					tip_new = pow->checkTooltip(inpt->mouse);
				}
				if (inv->visible && !mouse_dragging && isWithinRect(inv->window_area,inpt->mouse)) {
					tip_new = inv->checkTooltip(inpt->mouse);
				}
			}
			if (isWithinRect(effects->window_area, inpt->mouse)) {
				tip_new = effects->checkTooltip(inpt->mouse);
			}
			if (isWithinRect(act->window_area,inpt->mouse)) {
				tip_new = act->checkTooltip(inpt->mouse);
			}

			if (!tip_new.isEmpty()) {

				// when we render a tooltip it buffers the rasterized text for performance.
				// If this new tooltip is the same as the existing one, reuse.

				if (!tip_new.compare(&tip_buf)) {
					tip_buf.clear();
					tip_buf = tip_new;
				}
				tip->render(tip_buf, inpt->mouse, STYLE_FLOAT);
				TOOLTIP_CONTEXT = TOOLTIP_MENU;
			}
			else if (TOOLTIP_CONTEXT != TOOLTIP_MAP) {
				TOOLTIP_CONTEXT = TOOLTIP_NONE;
			}
		}
	}

	// draw icon under cursor if dragging
	if (mouse_dragging && !num_picker->visible) {
		if (drag_src == DRAG_SRC_INVENTORY || drag_src == DRAG_SRC_VENDOR || drag_src == DRAG_SRC_TROVE || drag_src == DRAG_SRC_KILN || drag_src == DRAG_SRC_SALVAGE)
			setDragIconItem(drag_stack);
		else if (drag_src == DRAG_SRC_POWERS || drag_src == DRAG_SRC_ACTIONBAR)
			setDragIcon(powers->powers[drag_power].icon);

		if (TOUCHSCREEN && sticky_dragging)
			renderIcon(keydrag_pos.x - ICON_SIZE/2, keydrag_pos.y - ICON_SIZE/2);
		else
			renderIcon(inpt->mouse.x - ICON_SIZE/2, inpt->mouse.y - ICON_SIZE/2);
	}
	else if (keyboard_dragging && !num_picker->visible) {
		if (drag_src == DRAG_SRC_INVENTORY || drag_src == DRAG_SRC_VENDOR || drag_src == DRAG_SRC_TROVE || drag_src == DRAG_SRC_KILN || drag_src == DRAG_SRC_SALVAGE)
			setDragIconItem(drag_stack);
		else if (drag_src == DRAG_SRC_POWERS || drag_src == DRAG_SRC_ACTIONBAR)
			setDragIcon(powers->powers[drag_power].icon);

		renderIcon(keydrag_pos.x - ICON_SIZE/2, keydrag_pos.y - ICON_SIZE/2);
	}

	// render the dev console above everything else
	if (DEV_MODE) {
		devconsole->render();
	}
}

void MenuManager::handleKeyboardTooltips() {

	TooltipData keyb_tip_new_vendor;
	TooltipData keyb_tip_new_trove;
	TooltipData keyb_tip_new_kiln;
	TooltipData keyb_tip_new_salvage;
	TooltipData keyb_tip_new_pow;
	TooltipData keyb_tip_new_inv;
	TooltipData keyb_tip_new_act;

	if (vendor->visible && vendor->getCurrentTabList() && vendor->getCurrentTabList() != (&vendor->tablist)) {
		int slot_index = vendor->getCurrentTabList()->getCurrent();

		if (vendor->getTab() == VENDOR_BUY) {
			keydrag_pos.x = vendor->stock[VENDOR_BUY].slots[slot_index]->pos.x;
			keydrag_pos.y = vendor->stock[VENDOR_BUY].slots[slot_index]->pos.y;
		}
		else if (vendor->getTab() == VENDOR_SELL) {
			keydrag_pos.x = vendor->stock[VENDOR_SELL].slots[slot_index]->pos.x;
			keydrag_pos.y = vendor->stock[VENDOR_SELL].slots[slot_index]->pos.y;
		}

		keyb_tip_new_vendor = vendor->checkTooltip(keydrag_pos);
		if (!keyb_tip_new_vendor.isEmpty()) {
			if (!keyb_tip_new_vendor.compare(&keyb_tip_buf_vendor)) {
				keyb_tip_buf_vendor.clear();
				keyb_tip_buf_vendor = keyb_tip_new_vendor;
			}
			tip->render(keyb_tip_buf_vendor, keydrag_pos, STYLE_FLOAT);
		}
	}

	if (trove->visible && trove->getCurrentTabList()) {
		int slot_index = trove->getCurrentTabList()->getCurrent();

		keydrag_pos.x = trove->stock.slots[slot_index]->pos.x;
		keydrag_pos.y = trove->stock.slots[slot_index]->pos.y;

		keyb_tip_new_trove = trove->checkTooltip(keydrag_pos);
		if (!keyb_tip_new_trove.isEmpty()) {
			if (!keyb_tip_new_trove.compare(&keyb_tip_buf_trove)) {
				keyb_tip_buf_trove.clear();
				keyb_tip_buf_trove = keyb_tip_new_trove;
			}
			tip->render(keyb_tip_buf_trove, keydrag_pos, STYLE_FLOAT);
		}
	}
	
	if (kiln->visible && kiln->getCurrentTabList()) {
		int slot_index = kiln->getCurrentTabList()->getCurrent();

		keydrag_pos.x = kiln->oven.slots[slot_index]->pos.x;
		keydrag_pos.y = kiln->oven.slots[slot_index]->pos.y;

		keyb_tip_new_kiln = kiln->checkTooltip(keydrag_pos);
		if (!keyb_tip_new_kiln.isEmpty()) {
			if (!keyb_tip_new_kiln.compare(&keyb_tip_buf_kiln)) {
				keyb_tip_buf_kiln.clear();
				keyb_tip_buf_kiln = keyb_tip_new_kiln;
			}
			tip->render(keyb_tip_buf_kiln, keydrag_pos, STYLE_FLOAT);
		}
	}
	
	if (salvage->visible && salvage->getCurrentTabList()) {
		int slot_index = salvage->getCurrentTabList()->getCurrent();

		keydrag_pos.x = salvage->pot.slots[slot_index]->pos.x;
		keydrag_pos.y = salvage->pot.slots[slot_index]->pos.y;

		keyb_tip_new_salvage = salvage->checkTooltip(keydrag_pos);
		if (!keyb_tip_new_salvage.isEmpty()) {
			if (!keyb_tip_new_salvage.compare(&keyb_tip_buf_salvage)) {
				keyb_tip_buf_salvage.clear();
				keyb_tip_buf_salvage = keyb_tip_new_salvage;
			}
			tip->render(keyb_tip_buf_salvage, keydrag_pos, STYLE_FLOAT);
		}
	}

	if (pow->visible && pow->isTabListSelected()) {
		int slot_index = pow->getSelectedCellIndex();

		keydrag_pos.x = pow->slots[slot_index]->pos.x;
		keydrag_pos.y = pow->slots[slot_index]->pos.y;

		keyb_tip_new_pow = pow->checkTooltip(keydrag_pos);
		if (!keyb_tip_new_pow.isEmpty()) {
			if (!keyb_tip_new_pow.compare(&keyb_tip_buf_pow)) {
				keyb_tip_buf_pow.clear();
				keyb_tip_buf_pow = keyb_tip_new_pow;
			}
			tip->render(keyb_tip_buf_pow, keydrag_pos, STYLE_FLOAT);
		}
	}

	if (inv->visible && inv->getCurrentTabList()) {
		int slot_index = inv->getCurrentTabList()->getCurrent();

		if (slot_index < inv->getEquippedCount()) {
			keydrag_pos.x = inv->inventory[EQUIPMENT].slots[slot_index]->pos.x;
			keydrag_pos.y = inv->inventory[EQUIPMENT].slots[slot_index]->pos.y;
		}
		else {
			keydrag_pos.x = inv->inventory[CARRIED].slots[slot_index - inv->getEquippedCount()]->pos.x;
			keydrag_pos.y = inv->inventory[CARRIED].slots[slot_index - inv->getEquippedCount()]->pos.y;
		}

		keyb_tip_new_inv = inv->checkTooltip(keydrag_pos);
		if (!keyb_tip_new_inv.isEmpty()) {
			if (!keyb_tip_new_inv.compare(&keyb_tip_buf_inv)) {
				keyb_tip_buf_inv.clear();
				keyb_tip_buf_inv = keyb_tip_new_inv;
			}
			tip->render(keyb_tip_buf_inv, keydrag_pos, STYLE_FLOAT);
		}
	}

	if (act_drag_hover && act->getCurrentTabList()) {
		int slot_index = act->getCurrentTabList()->getCurrent();

		keydrag_pos = act->getSlotPos(slot_index);

		keyb_tip_new_act = act->checkTooltip(keydrag_pos);
		if (!keyb_tip_new_act.isEmpty()) {
			if (!keyb_tip_new_act.compare(&keyb_tip_buf_act)) {
				keyb_tip_buf_act.clear();
				keyb_tip_buf_act = keyb_tip_new_act;
			}
			tip->render(keyb_tip_buf_act, keydrag_pos, STYLE_FLOAT);
		}
	}
}

void MenuManager::closeAll() {
	closeLeft();
	closeRight();
}

void MenuManager::closeLeft() {
	resetDrag();
	chr->visible = false;
	missionlog->visible = false;
	exit->visible = false;
	trove->visible = false;
	kiln->visible = false;
	salvage->visible = false;
	book->visible = false;
	book->book_name = "";
	num_picker->visible = false;

	npc->setNPC(NULL);
	talker->setNPC(NULL);
	vendor->setNPC(NULL);

	if (DEV_MODE && devconsole->visible) {
		devconsole->visible = false;
		devconsole->reset();
	}
}

void MenuManager::closeRight() {
	resetDrag();
	inv->visible = false;
	pow->visible = false;
	exit->visible = false;
	book->visible = false;
	book->book_name = "";
	num_picker->visible = false;

	npc->setNPC(NULL);
	talker->setNPC(NULL);

	if (DEV_MODE && devconsole->visible) {
		devconsole->visible = false;
		devconsole->reset();
	}
}

bool MenuManager::isDragging() {
	return drag_src != 0;
}

bool MenuManager::isNPCMenuVisible() {
	return npc->visible || talker->visible || vendor->visible;
}
MenuManager::~MenuManager() {

	tip_buf.clear();

	delete hull;
	delete shield;
	delete psi;
	delete capacitor;
	delete limitbreak;
	delete soul;
	delete mini;
	delete inv;
	delete pow;
	delete chr;
	delete hudlog;
	delete missionlog;
	delete act;
	delete tip;
	delete vendor;
	delete talker;
	delete exit;
	delete enemy;
	delete effects;
	delete trove;
	delete npc;
	delete book;
	delete num_picker;
	delete salvage;
	delete kiln;

	if (DEV_MODE) {
		delete devhud;
		delete devconsole;
	}

	if (drag_icon) delete drag_icon;
}
