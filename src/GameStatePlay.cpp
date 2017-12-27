/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
Copyright © 2012-2014 Henrik Andersson
Copyright © 2012 Stefan Beller
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
 * class GameStatePlay
 *
 * Handles logic and rendering of the main action game play
 * Also handles message passing between child objects, often to avoid circular dependencies.
 */

#include "Avatar.h"
#include "CampaignManager.h"
#include "EnemyManager.h"
#include "GameStatePlay.h"
#include "GameState.h"
#include "GameStateTitle.h"
#include "GameStateCutscene.h"
#include "Hazard.h"
#include "HazardManager.h"
#include "Menu.h"
#include "MenuActionBar.h"
#include "MenuCharacter.h"
#include "MenuBook.h"
#include "MenuEnemy.h"
#include "MenuExit.h"
#include "MenuHUDLog.h"
#include "MenuInventory.h"
#include "MenuLog.h"
#include "MenuManager.h"
#include "MenuMiniMap.h"
#include "MenuNPCActions.h"
#include "MenuTrove.h"
#include "MenuKiln.h"
#include "MenuTalker.h"
#include "MenuVendor.h"
#include "NPC.h"
#include "NPCManager.h"
#include "MissionLog.h"
#include "WidgetLabel.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "UtilsFileSystem.h"
#include "FileParser.h"
#include "UtilsParsing.h"
#include "MenuPowers.h"
#include "SaveLoad.h"

GameStatePlay::GameStatePlay()
	: GameState()
	, enemy(NULL)
	, loading(new WidgetLabel())
	, loading_bg(NULL)
	// Load the loading screen image (we currently use the confirm dialog background):
	, npc_id(-1)
	, npc_from_map(true)
	, color_normal(font->getColor("menu_normal"))
	, nearest_npc(-1)
	, menu_enemy_timeout(MAX_FRAMES_PER_SEC*10)
{
	Image *graphics;
	hasMusic = true;
	has_background = false;
	// GameEngine scope variables

	graphics = render_device->loadImage("images/menus/confirm_bg.png");
	if (graphics) {
		loading_bg = graphics->createSprite();
		graphics->unref();
	}

	if (items == NULL)
		items = new ItemManager();

	loot = new LootManager();
	powers = new PowerManager();
	camp = new CampaignManager();
	mapr = new MapRenderer();
	pc = new Avatar();
	enemies = new EnemyManager();
	enemyg = new EnemyGroupManager();
	hazards = new HazardManager();
	menu = new MenuManager(&pc->stats);
	npcs = new NPCManager(&pc->stats);
	missions = new MissionLog(menu->missionlog);

	// LootManager needs hero StatBlock
	loot->hero = &pc->stats;

	loading->set(0, 0, JUSTIFY_CENTER, VALIGN_CENTER, msg->get("Loading..."), color_normal);

	// load the config file for character titles
	loadTitles();

	refreshWidgets();
}

void GameStatePlay::refreshWidgets() {
	menu->alignAll();
	loading->setPos(VIEW_W_HALF, VIEW_H_HALF);
}

/**
 * Reset all game states to a new game.
 */
void GameStatePlay::resetGame() {
	mapr->load("maps/spawn.txt");
	setLoadingFrame();
	camp->status.clear();
	pc->init();
	pc->stats.currency = 0;
	menu->act->clear();
	menu->inv->inventory[0].clear();
	menu->inv->inventory[1].clear();
	menu->inv->changed_equipment = true;
	menu->inv->currency = 0;
	menu->missionlog->clear();
	missions->createMissionList();
	menu->hudlog->clear();
	save_load->loadTrove();

	// Finalize new character settings
	menu->talker->setHero(pc->stats);
	pc->loadSounds();
	mapr->executeOnLoadEvents();
}

/**
 * Check mouseover for enemies.
 * class variable "enemy" contains a live enemy on mouseover.
 * This function also sets enemy mouseover for Menu Enemy.
 */
void GameStatePlay::checkEnemyFocus() {
	// check the last hit enemy first
	// if there's none, then either get the nearest enemy or one under the mouse (depending on mouse mode)
	if (NO_MOUSE) {
		if (hazards->last_enemy) {
			if (enemy == hazards->last_enemy) {
				if (menu->enemy->timeout > 0 && hazards->last_enemy->stats.hull > 0) return;
				else hazards->last_enemy = NULL;
			}
			enemy = hazards->last_enemy;
		}
		else {
			enemy = enemies->getNearestEnemy(pc->stats.pos);
		}
	}
	else {
		if (hazards->last_enemy) {
			enemy = hazards->last_enemy;
			hazards->last_enemy = NULL;
		}
		else {
			enemy = enemies->enemyFocus(inpt->mouse, mapr->cam, true);
			if (enemy) curs->setCursor(CURSOR_ATTACK);
		}
	}

	if (enemy) {
		// set the actual menu with the enemy selected above
		if (!enemy->stats.suppress_hull) {
			menu->enemy->enemy = enemy;
			menu->enemy->timeout = menu_enemy_timeout;
		}
	}
	else if (!NO_MOUSE) {
		// if we're using a mouse and we didn't select an enemy, try selecting a dead one instead
		Enemy *temp_enemy = enemies->enemyFocus(inpt->mouse, mapr->cam, false);
		if (temp_enemy) {
			menu->enemy->enemy = temp_enemy;
			menu->enemy->timeout = menu_enemy_timeout;
		}
	}

	// save the highlighted enemy position for auto-targeting purposes
	if (enemy) {
		pc->enemy_pos = enemy->stats.pos;
	}
	else {
		pc->enemy_pos.x = -1;
		pc->enemy_pos.y = -1;
	}
}

/**
 * If mouse_move is enabled, and the mouse is over a live enemy,
 * Do not allow power use with button MAIN1
 */
bool GameStatePlay::restrictPowerUse() {
	if(MOUSE_MOVE) {
		if(inpt->pressing[MAIN1] && !inpt->pressing[SHIFT] && !menu->act->isWithinSlots(inpt->mouse) && !menu->act->isWithinMenus(inpt->mouse)) {
			if(enemy == NULL) {
				return true;
			}
			else {
				if(static_cast<unsigned>(ACTIONBAR_MAIN) < menu->act->slots_count && menu->act->slot_enabled[ACTIONBAR_MAIN] && (powers->powers[menu->act->hotkeys[ACTIONBAR_MAIN]].target_party != enemy->stats.hero_ally))
					return true;
			}
		}
	}

	return false;
}

/**
 * Check to see if the player is picking up loot on the ground
 */
void GameStatePlay::checkLoot() {

	if (!pc->stats.alive)
		return;

	if (menu->isDragging())
		return;

	ItemStack pickup;

	// Autopickup
	if (AUTOPICKUP_CURRENCY) {
		pickup = loot->checkAutoPickup(pc->stats.pos);
		if (!pickup.empty()) {
			menu->inv->add(pickup, CARRIED, -1, true, true);
			pickup.clear();
		}
	}

	// Normal pickups
	if (!pc->stats.attacking) {
		pickup = loot->checkPickup(inpt->mouse, mapr->cam, pc->stats.pos);
	}

	if (!pickup.empty()) {
		menu->inv->add(pickup, CARRIED, -1, true, true);
		camp->setStatus(items->items[pickup.item].pickup_status);
		pickup.clear();
	}

}

void GameStatePlay::checkTeleport() {
	bool on_load_teleport = false;

	// both map events and player powers can cause teleportation
	if (mapr->teleportation || pc->stats.teleportation) {

		mapr->collider.unblock(pc->stats.pos.x, pc->stats.pos.y);

		if (mapr->teleportation) {
			mapr->cam.x = pc->stats.pos.x = mapr->teleport_destination.x;
			mapr->cam.y = pc->stats.pos.y = mapr->teleport_destination.y;
		}
		else {
			mapr->cam.x = pc->stats.pos.x = pc->stats.teleport_destination.x;
			mapr->cam.y = pc->stats.pos.y = pc->stats.teleport_destination.y;
		}

		// if we're not changing map, move allies to a the player's new position
		// when changing maps, enemies->handleNewMap() does something similar to this
		if (mapr->teleport_mapname.empty()) {
			FPoint spawn_pos = mapr->collider.get_random_neighbor(FPointToPoint(pc->stats.pos), 1, false);
			for (unsigned int i=0; i < enemies->enemies.size(); i++) {
				if(enemies->enemies[i]->stats.hero_ally && enemies->enemies[i]->stats.alive) {
					mapr->collider.unblock(enemies->enemies[i]->stats.pos.x, enemies->enemies[i]->stats.pos.y);
					enemies->enemies[i]->stats.pos = spawn_pos;
					mapr->collider.block(enemies->enemies[i]->stats.pos.x, enemies->enemies[i]->stats.pos.y, true);
				}
			}
		}

		// process intermap teleport
		if (mapr->teleportation && !mapr->teleport_mapname.empty()) {
			std::string teleport_mapname = mapr->teleport_mapname;
			mapr->teleport_mapname = "";
			inpt->lock_all = (teleport_mapname == "maps/spawn.txt");
			mapr->executeOnMapExitEvents();
			showLoading();
			mapr->load(teleport_mapname);
			setLoadingFrame();
			enemies->handleNewMap();
			hazards->handleNewMap();
			loot->handleNewMap();
			powers->handleNewMap(&mapr->collider);
			menu->enemy->handleNewMap();
			npcs->handleNewMap();
			resetNPC();
			menu->trove->visible = false;
			menu->mini->prerender(&mapr->collider, mapr->w, mapr->h);
			npc_id = nearest_npc = -1;

			// use the default hero spawn position for this map
			if (mapr->teleport_destination.x == -1 && mapr->teleport_destination.y == -1) {
				mapr->cam.x = pc->stats.pos.x = mapr->hero_pos.x;
				mapr->cam.y = pc->stats.pos.y = mapr->hero_pos.y;
			}

			// store this as the new respawn point (provided the tile is open)
			if (mapr->collider.is_valid_position(pc->stats.pos.x, pc->stats.pos.y, MOVEMENT_NORMAL, true)) {
				mapr->respawn_map = teleport_mapname;
				mapr->respawn_point = pc->stats.pos;
			}
			else {
				logError("GameStatePlay: Spawn position (%d, %d) is blocked.", static_cast<int>(pc->stats.pos.x), static_cast<int>(pc->stats.pos.y));
			}

			// return to title (permadeath) OR auto-save
			if (pc->stats.permadeath && pc->stats.corpse) {
				removeSaveDir(save_load->getGameSlot());

				snd->stopMusic();
				delete requestedGameState;
				requestedGameState = new GameStateTitle();
			}
			else if (SAVE_ONLOAD) {
				save_load->saveGame();
			}
			// switch off teleport flag so we can check if an on_load event has teleportation
			mapr->teleportation = false;
			mapr->executeOnLoadEvents();
			if (mapr->teleportation)
				on_load_teleport = true;
		}

		mapr->collider.block(pc->stats.pos.x, pc->stats.pos.y, false);

		pc->stats.teleportation = false; // teleport spell

	}

	if (!on_load_teleport && mapr->teleport_mapname.empty())
		mapr->teleportation = false;
}

/**
 * Check for cancel key to exit menus or exit the game.
 * Also check closing the game window entirely.
 */
void GameStatePlay::checkCancel() {

	// if user has clicked exit game from exit menu
	if (menu->requestingExit()) {
		if (SAVE_ONEXIT)
			save_load->saveGame();

		// audio levels can be changed in the pause menu, so update our settings file
		saveSettings();

		snd->stopMusic();
		delete requestedGameState;
		requestedGameState = new GameStateTitle();

		save_load->setGameSlot(0);
	}

	// if user closes the window
	if (inpt->done) {
		if (SAVE_ONEXIT)
			save_load->saveGame();

		snd->stopMusic();
		exitRequested = true;
	}
}

/**
 * Check for log messages from various child objects
 */
void GameStatePlay::checkLog() {

	// If the player has just respawned, we want to clear the HUD log
	if (pc->respawn) {
		menu->hudlog->clear();
	}

	while (!pc->log_msg.empty()) {
		const std::string& str = pc->log_msg.front().first;
		const bool prevent_spam = pc->log_msg.front().second;

		menu->missionlog->add(str, LOG_TYPE_MESSAGES, prevent_spam);
		menu->hudlog->add(str, prevent_spam);

		pc->log_msg.pop();
	}
}

/**
 * Check if we need to open book
 */
void GameStatePlay::checkBook() {
	// Map events can open books
	if (mapr->show_book != "") {
		menu->book->book_name = mapr->show_book;
		mapr->show_book = "";
	}

	// items can be readable books
	if (menu->inv->show_book != "") {
		menu->book->book_name = menu->inv->show_book;
		menu->inv->show_book = "";
	}
}

void GameStatePlay::loadTitles() {
	FileParser infile;
	// @CLASS GameStatePlay: Titles|Description of engine/titles.txt
	if (infile.open("engine/titles.txt")) {
		while (infile.next()) {
			if (infile.new_section && infile.section == "title") {
				Title t;
				titles.push_back(t);
			}

			if (titles.empty()) continue;

			// @ATTR title.title|string|The displayed title.
			if (infile.key == "title") titles.back().title = infile.val;
			// @ATTR title.generation|int|Requires generation.
			else if (infile.key == "generation") titles.back().generation = toInt(infile.val);
			// @ATTR title.power|power_id|Requires power.
			else if (infile.key == "power") titles.back().power = toInt(infile.val);
			// @ATTR title.requires_status|string|Requires status.
			else if (infile.key == "requires_status") titles.back().requires_status = infile.val;
			// @ATTR title.requires_not_status|string|Requires not status.
			else if (infile.key == "requires_not_status") titles.back().requires_not = infile.val;
			// @ATTR title.primary_stat|predefined_string, predefined_string : Primary stat, Lesser primary stat|Required primary stat(s). The lesser stat is optional.
			else if (infile.key == "primary_stat") {
				titles.back().primary_stat_1 = popFirstString(infile.val);
				titles.back().primary_stat_2 = popFirstString(infile.val);
			}
			else infile.error("GameStatePlay: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();
	}
}

void GameStatePlay::checkTitle() {
	if (!pc->stats.check_title || titles.empty()) return;

	int title_id = -1;

	for (unsigned i=0; i<titles.size(); i++) {
		if (titles[i].title.empty())
			continue;

		if (titles[i].generation > 0 && pc->stats.generation < titles[i].generation)
			continue;
		if (titles[i].power > 0 && std::find(pc->stats.powers_list.begin(), pc->stats.powers_list.end(), titles[i].power) == pc->stats.powers_list.end())
			continue;
		if (!titles[i].requires_status.empty() && !camp->checkStatus(titles[i].requires_status))
			continue;
		if (!titles[i].requires_not.empty() && camp->checkStatus(titles[i].requires_not))
			continue;
		if (!titles[i].primary_stat_1.empty() && !checkPrimaryStat(titles[i].primary_stat_1, titles[i].primary_stat_2))
			continue;

		// Title meets the requirements
		title_id = i;
		break;
	}

	if (title_id != -1) pc->stats.character_subclass = titles[title_id].title;
	pc->stats.check_title = false;
	pc->stats.refresh_stats = true;
}

void GameStatePlay::checkEquipmentChange() {
	// force the actionbar to update when we change gear
	if (menu->inv->changed_equipment) {
		menu->act->updated = true;
	}

	if (menu->inv->changed_equipment) {

		int feet_index = -1;
		std::vector<Layer_gfx> img_gfx;
		// load only displayable layers
		for (unsigned int j=0; j<pc->layer_reference_order.size(); j++) {
			Layer_gfx gfx;
			gfx.gfx = "";
			gfx.type = "";
			for (int i=0; i<menu->inv->inventory[EQUIPMENT].getSlotNumber(); i++) {
				if (pc->layer_reference_order[j] == menu->inv->inventory[EQUIPMENT].slot_type[i]) {
					gfx.gfx = items->items[menu->inv->inventory[EQUIPMENT][i].item].gfx;
					gfx.type = menu->inv->inventory[EQUIPMENT].slot_type[i];
				}
				if (menu->inv->inventory[EQUIPMENT].slot_type[i] == "feet") {
					feet_index = i;
				}
			}
			// special case: if we don't have a head, use the portrait's head
			if (gfx.gfx == "" && pc->layer_reference_order[j] == "head") {
				gfx.gfx = pc->stats.gfx_head;
				gfx.type = "head";
			}
			// fall back to default if it exists
			if (gfx.gfx == "") {
				bool exists = fileExists(mods->locate("animations/avatar/" + pc->stats.gfx_base + "/default_" + gfx.type + ".txt"));
				if (exists) gfx.gfx = "default_" + gfx.type;
			}
			img_gfx.push_back(gfx);
		}
		assert(pc->layer_reference_order.size()==img_gfx.size());
		pc->loadGraphics(img_gfx);

		if (feet_index != -1)
			pc->loadStepFX(items->items[menu->inv->inventory[EQUIPMENT][feet_index].item].stepfx);
	}

	menu->inv->changed_equipment = false;
}

void GameStatePlay::checkLootDrop() {

	// if the player has dropped an item from the inventory
	while (!menu->drop_stack.empty()) {
		if (!menu->drop_stack.front().empty()) {
			loot->addLoot(menu->drop_stack.front(), pc->stats.pos, true);
		}
		menu->drop_stack.pop();
	}

	// if the player has dropped a mission reward because inventory full
	while (!camp->drop_stack.empty()) {
		if (!camp->drop_stack.front().empty()) {
			loot->addLoot(camp->drop_stack.front(), pc->stats.pos, true);
		}
		camp->drop_stack.pop();
	}

	// if the player been directly given items, but their inventory is full
	// this happens when adding currency from older save files
	while (!menu->inv->drop_stack.empty()) {
		if (!menu->inv->drop_stack.front().empty()) {
			loot->addLoot(menu->inv->drop_stack.front(), pc->stats.pos, true);
		}
		menu->inv->drop_stack.pop();
	}
}

/**
 * Removes items as required by certain powers
 */
void GameStatePlay::checkUsedItems() {
	for (unsigned i=0; i<powers->used_items.size(); i++) {
		menu->inv->remove(powers->used_items[i]);
	}
	for (unsigned i=0; i<powers->used_equipped_items.size(); i++) {
		menu->inv->inventory[EQUIPMENT].remove(powers->used_equipped_items[i]);
		menu->inv->applyEquipment();
	}
	powers->used_items.clear();
	powers->used_equipped_items.clear();
}

/**
 * Marks the menu if it needs attention.
 */
void GameStatePlay::checkNotifications() {
	if (pc->newGenerationNotification || menu->chr->getUnspent() > 0) {
		pc->newGenerationNotification = false;
		menu->act->requires_attention[MENU_CHARACTER] = !menu->chr->visible;
	}
	if (menu->pow->newPowerNotification) {
		menu->pow->newPowerNotification = false;
		menu->act->requires_attention[MENU_POWERS] = !menu->pow->visible;
	}
	if (missions->newMissionNotification) {
		missions->newMissionNotification = false;
		menu->act->requires_attention[MENU_LOG] = !menu->missionlog->visible;
	}

	// if the player is transformed into a creature, don't notifications for the powers menu
	if (pc->stats.transformed) {
		menu->act->requires_attention[MENU_POWERS] = false;
	}
}

/**
 * If the player has clicked on an NPC, the game mode might be changed.
 * If a player walks away from an NPC, end the interaction with that NPC
 * If an NPC is giving a reward, process it
 */
void GameStatePlay::checkNPCInteraction() {
	if (pc->stats.attacking || !pc->stats.humanoid)
		return;

	// reset movement restrictions when we're not in dialog
	if (!menu->talker->visible) {
		pc->allow_movement = true;
	}
	if (npc_id != -1 && !menu->isNPCMenuVisible()) {
		// if we have an NPC, but no NPC windows are open, clear the NPC
		resetNPC();
	}
	// get NPC by ID
	// event NPCs take precedence over map NPCs
	if (mapr->event_npc != "") {
		// if the player is already interacting with an NPC when triggering an event NPC, clear the current NPC
		if (npc_id != -1) {
			resetNPC();
		}
		npc_id = mapr->npc_id = npcs->getID(mapr->event_npc);
		npc_from_map = false;
	}
	else if (mapr->npc_id != -1) {
		npc_id = mapr->npc_id;
		npc_from_map = true;
	}
	mapr->event_npc = "";
	mapr->npc_id = -1;
	if (npc_id == -1)
		return;

	if (npc_id != -1) {
		bool interact_with_npc = false;
		if (npc_from_map) {
			float interact_distance = calcDist(pc->stats.pos, npcs->npcs[npc_id]->pos);
			if (interact_distance < INTERACT_RANGE) {
				interact_with_npc = true;
			}
			else {
				resetNPC();
			}
		}
		else {
			// npc is from event
			interact_with_npc = true;
			// since its impossible for the player to walk away from event NPCs, we disable their movement here
			pc->allow_movement = false;
		}

		if (interact_with_npc) {
			if (!menu->isNPCMenuVisible()) {
				if (inpt->pressing[MAIN1] && !NO_MOUSE) inpt->lock[MAIN1] = true;
				if (inpt->pressing[ACCEPT]) inpt->lock[ACCEPT] = true;

				menu->npc->setNPC(npcs->npcs[npc_id]);

				// only show npc action menu if multiple actions are available
				if (!menu->npc->empty() && !menu->npc->selection())
					menu->npc->visible = true;
			}
		}
	}

	// check if a NPC action selection is made
	if (npc_id != -1 && menu->npc->selection()) {
		if (menu->npc->vendor_selected && !menu->vendor->visible) {
			// begin trading
			menu->closeAll();
			menu->vendor->setNPC(npcs->npcs[npc_id]);
			menu->inv->visible = true;
		}
		else if (menu->npc->dialog_selected && !menu->talker->visible) {
			// begin talking
			menu->closeAll();
			menu->talker->setNPC(npcs->npcs[npc_id]);
			menu->talker->chooseDialogNode(menu->npc->selected_dialog_node);
			if (npc_from_map) {
				pc->allow_movement = npcs->npcs[npc_id]->checkMovement(menu->npc->selected_dialog_node);
			}
		}
		menu->npc->setNPC(NULL);
	}
}

void GameStatePlay::checkTrove() {
	if (mapr->trove) {
		// If triggered, open the trove and inventory menus
		menu->closeAll();
		menu->inv->visible = true;
		menu->trove->visible = true;
		mapr->trove = false;
	}
	else if (menu->trove->visible) {
		// Close trove if inventory is closed
		if (!menu->inv->visible) {
			menu->resetDrag();
			menu->trove->visible = false;
		}
		// If the player walks away from the trove, close its menu
		float interact_distance = calcDist(pc->stats.pos, mapr->trove_pos);
		if (interact_distance > INTERACT_RANGE || !pc->stats.alive) {
			menu->resetDrag();
			menu->trove->visible = false;
		}
	}
}

void GameStatePlay::checkKiln() {
	if (mapr->kiln) {
		// If triggered, open the kiln and inventory menus
		menu->closeAll();
		menu->inv->visible = true;
		menu->kiln->visible = true;
		mapr->kiln = false;
	}
	else if (menu->kiln->visible) {
		// Close kiln if inventory is closed
		if (!menu->inv->visible) {
			menu->resetDrag();
			menu->kiln->visible = false;
		}
		// If the player walks away from the kiln, close its menu
		float interact_distance = calcDist(pc->stats.pos, mapr->kiln_pos);
		if (interact_distance > INTERACT_RANGE || !pc->stats.alive) {
			menu->resetDrag();
			menu->kiln->visible = false;
		}
	}
}

void GameStatePlay::checkCutscene() {
	if (!mapr->cutscene)
		return;

	GameStateCutscene *cutscene = new GameStateCutscene(NULL);

	if (!cutscene->load(mapr->cutscene_file)) {
		delete cutscene;
		mapr->cutscene = false;
		return;
	}

	// handle respawn point and set game play game_slot
	cutscene->game_slot = save_load->getGameSlot();

	if (mapr->teleportation) {

		if (mapr->teleport_mapname != "")
			mapr->respawn_map = mapr->teleport_mapname;

		mapr->respawn_point = mapr->teleport_destination;

	}
	else {
		mapr->respawn_point = FPointToPoint(pc->stats.pos);
	}

	if (SAVE_ONLOAD)
		save_load->saveGame();

	delete requestedGameState;
	requestedGameState = cutscene;
}

void GameStatePlay::checkSaveEvent() {
	if (mapr->save_game) {
		mapr->respawn_point = FPointToPoint(pc->stats.pos);
		save_load->saveGame();
		mapr->save_game = false;
	}
}

/**
 * Recursively update the action bar powers based on equipment
 */
void GameStatePlay::updateActionBar(unsigned index) {
	if (menu->act->slots_count == 0 || index > menu->act->slots_count - 1) return;

	if (items->items.empty()) return;

	for (unsigned i = index; i < menu->act->slots_count; i++) {
		if (menu->act->hotkeys[i] == 0) continue;

		for (int j=0; j<menu->inv->inventory[EQUIPMENT].getSlotNumber(); j++) {
			int id = menu->inv->inventory[EQUIPMENT][j].item;

			for (unsigned k=0; k<items->items[id].replace_power.size(); k++) {
				if (items->items[id].replace_power[k].x == menu->act->hotkeys_mod[i] &&
				    items->items[id].replace_power[k].y != menu->act->hotkeys_mod[i]) {
						menu->act->hotkeys_mod[i] = items->items[id].replace_power[k].y;
						return updateActionBar(i);
				}
			}
		}
	}
}

/**
 * Process all actions for a single frame
 * This includes some message passing between child object
 */
void GameStatePlay::logic() {
	if (inpt->window_resized)
		refreshWidgets();

	checkCutscene();

	// check menus first (top layer gets mouse click priority)
	menu->logic();

	if (!isPaused()) {

		// these actions only occur when the game isn't paused
		if (pc->stats.alive) checkLoot();
		checkEnemyFocus();
		if (pc->stats.alive) {
			mapr->checkHotspots();
			mapr->checkNearestEvent();
			checkNPCInteraction();
		}
		checkTitle();

		menu->act->checkAction(action_queue);
		pc->logic(action_queue, restrictPowerUse(), npc_id != -1);

		// Transform powers change the actionbar layout,
		// so we need to prevent accidental clicks if a new power is placed under the slot we clicked on.
		// It's a bit hacky, but it works
		if (pc->isTransforming()) {
			menu->act->resetSlots();
		}

		// transfer hero data to enemies, for AI use
		if (pc->stats.get(STAT_STEALTH) > 100) enemies->hero_stealth = 100;
		else enemies->hero_stealth = pc->stats.get(STAT_STEALTH);

		enemies->logic();
		hazards->logic();
		loot->logic();
		enemies->checkEnemiesforSoul();
		npcs->logic();

		snd->logic(pc->stats.pos);

		comb->logic(mapr->cam);
	}

	// close menus when the player dies, but still allow them to be reopened
	if (pc->close_menus) {
		pc->close_menus = false;
		menu->closeAll();
	}

	// these actions occur whether the game is paused or not.
	checkTeleport();
	checkLootDrop();
	checkLog();
	checkBook();
	checkEquipmentChange();
	checkUsedItems();
	checkTrove();
	checkKiln();
	checkSaveEvent();
	checkNotifications();
	checkCancel();

	mapr->logic();
	mapr->enemies_cleared = enemies->isCleared();
	missions->logic();

	pc->checkTransform();

	// change hero powers on transformation
	if (pc->setPowers) {
		pc->setPowers = false;
		if (!pc->stats.humanoid && menu->pow->visible) menu->closeRight();
		// save ActionBar state and lock slots from removing/replacing power
		for (int i=0; i<ACTIONBAR_MAX ; i++) {
			menu->act->hotkeys_temp[i] = menu->act->hotkeys[i];
			menu->act->hotkeys[i] = 0;
		}
		int count = ACTIONBAR_MAIN;
		// put creature powers on action bar
		// TODO What if creature has more powers than the size of the action bar?
		for (size_t i=0; i<pc->charmed_stats->powers_ai.size(); i++) {
			if (pc->charmed_stats->powers_ai[i].id != 0 && powers->powers[pc->charmed_stats->powers_ai[i].id].beacon != true) {
				menu->act->hotkeys[count] = pc->charmed_stats->powers_ai[i].id;
				menu->act->locked[count] = true;
				count++;
			}
			if (count == ACTIONBAR_MAX) count = 0;
		}
		if (pc->stats.manual_untransform && pc->untransform_power > 0) {
			menu->act->hotkeys[count] = pc->untransform_power;
			menu->act->locked[count] = true;
		}
		else if (pc->stats.manual_untransform && pc->untransform_power == 0)
			logError("GameStatePlay: Untransform power not found, you can't untransform manually");

		menu->act->updated = true;

		// reapply equipment if the transformation allows it
		if (pc->stats.transform_with_equipment)
			menu->inv->applyEquipment();
	}
	// revert hero powers
	if (pc->revertPowers) {
		pc->revertPowers = false;

		// restore ActionBar state
		for (int i=0; i<ACTIONBAR_MAX; i++) {
			menu->act->hotkeys[i] = menu->act->hotkeys_temp[i];
			menu->act->locked[i] = false;
		}

		menu->act->updated = true;

		// also reapply equipment here, to account items that give bonuses to base stats
		menu->inv->applyEquipment();
	}

	// when the hero (re)spawns, reapply equipment & passive effects
	if (pc->respawn) {
		pc->stats.alive = true;
		pc->stats.corpse = false;
		pc->stats.cur_state = AVATAR_STANCE;
		menu->inv->applyEquipment();
		menu->inv->changed_equipment = true;
		checkEquipmentChange();
		powers->activatePassives(&pc->stats);
		pc->stats.logic();
		pc->stats.recalc();
		pc->respawn = false;
	}

	// use a normal mouse cursor is menus are open
	if (menu->menus_open) {
		curs->setCursor(CURSOR_NORMAL);
	}

	// update the action bar as it may have been changed by items
	if (menu->act->updated) {
		menu->act->updated = false;

		// set all hotkeys to their base powers
		for (unsigned i = 0; i < menu->act->slots_count; i++) {
			menu->act->hotkeys_mod[i] = menu->act->hotkeys[i];
		}

		updateActionBar();
	}

	// reload music if changed in the pause menu
	if (menu->exit->reload_music) {
		mapr->loadMusic();
		menu->exit->reload_music = false;
	}
}


/**
 * Render all graphics for a single frame
 */
void GameStatePlay::render() {

	// Create a list of Renderables from all objects not already on the map.
	// split the list into the beings alive (may move) and dead beings (must not move)
	std::vector<Renderable> rens;
	std::vector<Renderable> rens_dead;

	pc->addRenders(rens);

	enemies->addRenders(rens, rens_dead);

	npcs->addRenders(rens); // npcs cannot be dead

	loot->addRenders(rens, rens_dead);

	hazards->addRenders(rens, rens_dead);


	// render the static map layers plus the renderables
	mapr->render(rens, rens_dead);

	// mouseover tooltips
	loot->renderTooltips(mapr->cam);

	if (mapr->map_change) {
		menu->mini->prerender(&mapr->collider, mapr->w, mapr->h);
		mapr->map_change = false;
	}
	menu->mini->setMapTitle(mapr->title);
	menu->mini->render(pc->stats.pos);
	menu->render();

	// render combat text last - this should make it obvious you're being
	// attacked, even if you have menus open
	if (!isPaused())
		comb->render();
}

void GameStatePlay::showLoading() {
	if (loading_bg == NULL) return;

	Rect dest;
	dest.x = VIEW_W_HALF - loading_bg->getGraphicsWidth()/2;
	dest.y = VIEW_H_HALF - loading_bg->getGraphicsHeight()/2;

	loading_bg->setDest(dest);
	render_device->render(loading_bg);
	loading->render();

	render_device->commitFrame();
}

bool GameStatePlay::isPaused() {
	return menu->pause;
}

void GameStatePlay::resetNPC() {
	npc_id = -1;
	npc_from_map = true;
	menu->npc->setNPC(NULL);
	menu->vendor->setNPC(NULL);
	menu->talker->setNPC(NULL);
}

bool GameStatePlay::checkPrimaryStat(const std::string& first, const std::string& second) {
	int high = 0;
	size_t high_index = PRIMARY_STATS.size();
	size_t low_index = PRIMARY_STATS.size();

	for (size_t i = 0; i < PRIMARY_STATS.size(); ++i) {
		int stat = pc->stats.get_primary(i);
		if (stat > high) {
			if (high_index != PRIMARY_STATS.size()) {
				low_index = high_index;
			}
			high = stat;
			high_index = i;
		}
		else if (stat == high && low_index == PRIMARY_STATS.size()) {
			low_index = i;
		}
		else if (low_index == PRIMARY_STATS.size() || (low_index < PRIMARY_STATS.size() && stat > pc->stats.get_primary(low_index))) {
			low_index = i;
		}
	}

	// if the first primary stat doesn't match, we don't care about the second one
	if (high_index != PRIMARY_STATS.size() && first != PRIMARY_STATS[high_index].id)
		return false;

	if (!second.empty()) {
		if (low_index != PRIMARY_STATS.size() && second != PRIMARY_STATS[low_index].id)
			return false;
	}
	else if (second.empty() && pc->stats.get_primary(high_index) == pc->stats.get_primary(low_index)) {
		// titles that require a single stat are ignored if two stats are equal
		return false;
	}

	return true;
}

GameStatePlay::~GameStatePlay() {
	if (loading_bg)	delete loading_bg;
	delete missions;
	delete npcs;
	delete hazards;
	delete enemies;
	delete pc;
	delete mapr;
	delete menu;
	delete loot;
	delete camp;
	delete items;
	delete powers;

	delete loading;

	delete enemyg;

	// NULL-ify shared game resources
	menu_powers = NULL;
	loot = NULL;
	enemyg = NULL;
	powers = NULL;
	mapr = NULL;
	enemies = NULL;
	camp = NULL;
	items = NULL;
	pc = NULL;
	menu = NULL;
}

