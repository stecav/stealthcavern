/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
Copyright © 2012 Stefan Beller
Copyright © 2013 Henrik Andersson
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
 * class SaveLoad
 *
 * Save and Load functions for the GameStatePlay.
 *
 * I put these in a separate cpp file just to keep GameStatePlay.cpp devoted to its core.
 *
 */

#include "SaveLoad.h"
#include "CommonIncludes.h"
#include "FileParser.h"
#include "GameStatePlay.h"
#include "MapRenderer.h"
#include "MenuActionBar.h"
#include "MenuCharacter.h"
#include "Menu.h"
#include "MenuHUDLog.h"
#include "MenuInventory.h"
#include "MenuLog.h"
#include "MenuManager.h"
#include "MenuTrove.h"
#include "MenuTalker.h"
#include "Settings.h"
#include "Utils.h"
#include "UtilsFileSystem.h"
#include "UtilsParsing.h"
#include "SharedGameResources.h"

SaveLoad::SaveLoad()
	: game_slot(0) {
}

SaveLoad::~SaveLoad() {
}

/**
 * Before exiting the game, save to file
 */
void SaveLoad::saveGame() {

	if (game_slot <= 0) return;

	// if needed, create the save file structure
	createSaveDir(game_slot);

	// remove items with zero quantity from inventory
	menu->inv->inventory[EQUIPMENT].clean();
	menu->inv->inventory[CARRIED].clean();

	std::ofstream outfile;

	std::stringstream ss;
	ss << PATH_USER << "saves/" << SAVE_PREFIX << "/" << game_slot << "/avatar.txt";

	outfile.open(path(&ss).c_str(), std::ios::out);

	if (outfile.is_open()) {

		// comment
		outfile << "## stealthcavern-engine save file ##" << "\n";

		// hero name
		outfile << "name=" << pc->stats.name << "\n";

		// permadeath
		outfile << "permadeath=" << pc->stats.permadeath << "\n";

		// hero visual option
		outfile << "option=" << pc->stats.gfx_base << "," << pc->stats.gfx_head << "," << pc->stats.gfx_portrait << "\n";

		// hero class
		outfile << "class=" << pc->stats.character_class << "," << pc->stats.character_subclass << "\n";

		// current soul
		outfile << "soul=" << pc->stats.soul << "\n";
		
		// current limitbreak
		outfile << "limitbreak=" << pc->stats.limitbreak << "\n";

		// hull and capacitor
		outfile << "hullcapacitor=" << pc->stats.hull << ","  << pc->stats.shield << "," << pc->stats.psi << "," << pc->stats.capacitor << "\n";

		// stat spec
		outfile << "build=";
		for (size_t i = 0; i < PRIMARY_STATS.size(); ++i) {
			outfile << pc->stats.primary[i];
			outfile << ",";
		}
		for (size_t i = 0; i < TALENTS.size(); ++i) {
			outfile << pc->stats.talent[i];
			if (i < TALENTS.size() - 1)
				outfile << ",";
		}
		outfile << "\n";

		// equipped gear
		outfile << "equipped_quantity=" << menu->inv->inventory[EQUIPMENT].getQuantities() << "\n";
		outfile << "equipped=" << menu->inv->inventory[EQUIPMENT].getItems() << "\n";

		// carried items
		outfile << "carried_quantity=" << menu->inv->inventory[CARRIED].getQuantities() << "\n";
		outfile << "carried=" << menu->inv->inventory[CARRIED].getItems() << "\n";

		// spawn point
		outfile << "spawn=" << mapr->respawn_map << "," << static_cast<int>(mapr->respawn_point.x) << "," << static_cast<int>(mapr->respawn_point.y) << "\n";

		// action bar
		outfile << "actionbar=";
		for (unsigned i = 0; i < static_cast<unsigned>(ACTIONBAR_MAX); i++) {
			if (i < menu->act->slots_count)
			{
				if (pc->stats.transformed) outfile << menu->act->hotkeys_temp[i];
				else outfile << menu->act->hotkeys[i];
			}
			else
			{
				outfile << 0;
			}
			if (i < ACTIONBAR_MAX - 1) outfile << ",";
		}
		outfile << "\n";

		//shapeshifter value
		if (pc->stats.transform_type == "untransform" || pc->stats.transform_duration != -1) outfile << "transformed=" << "\n";
		else outfile << "transformed=" << pc->stats.transform_type << "," << pc->stats.manual_untransform << "\n";

		// restore hero powers
		if (pc->stats.transformed && pc->hero_stats) {
			pc->stats.powers_list = pc->hero_stats->powers_list;
		}

		// enabled powers
		outfile << "powers=";
		for (unsigned int i=0; i<pc->stats.powers_list.size(); i++) {
			if (i < pc->stats.powers_list.size()-1) {
				if (pc->stats.powers_list[i] > 0)
					outfile << pc->stats.powers_list[i] << ",";
			}
			else {
				if (pc->stats.powers_list[i] > 0)
					outfile << pc->stats.powers_list[i];
			}
		}
		outfile << "\n";

		// restore transformed powers
		if (pc->stats.transformed && pc->charmed_stats) {
			pc->stats.powers_list = pc->charmed_stats->powers_list;
		}

		// campaign data
		outfile << "campaign=";
		outfile << camp->getAll();

		outfile << std::endl;

		if (outfile.bad()) logError("SaveLoad: Unable to save the game. No write access or disk is full!");
		outfile.close();
		outfile.clear();
	}

	// Save trove
	ss.str("");
	if (pc->stats.permadeath)
		ss << PATH_USER << "saves/" << SAVE_PREFIX << "/" << game_slot << "/trove_HC.txt";
	else
		ss << PATH_USER << "saves/" << SAVE_PREFIX << "/trove.txt";

	outfile.open(path(&ss).c_str(), std::ios::out);

	if (outfile.is_open()) {

		// comment
		outfile << "## stealthcavern trove file ##" << "\n";

		outfile << "quantity=" << menu->trove->stock.getQuantities() << "\n";
		outfile << "item=" << menu->trove->stock.getItems() << "\n";

		outfile << std::endl;

		if (outfile.bad()) logError("SaveLoad: Unable to save trove. No write access or disk is full!");
		outfile.close();
		outfile.clear();
	}

	// display a log message saying that we saved the game
	menu->missionlog->add(msg->get("Game saved."), LOG_TYPE_MESSAGES);
	menu->hudlog->add(msg->get("Game saved."));
}

/**
 * When loading the game, load from file if possible
 */
void SaveLoad::loadGame() {
	if (game_slot <= 0) return;

	int saved_hull = 0;
	int saved_shield = 0;
	int saved_psi = 0;
	int saved_capacitor = 0;
	int currency = 0;

	FileParser infile;
	std::vector<int> hotkeys(ACTIONBAR_MAX, -1);

	std::stringstream ss;
	ss << PATH_USER << "saves/" << SAVE_PREFIX << "/" << game_slot << "/avatar.txt";

	if (infile.open(path(&ss), false)) {
		while (infile.next()) {
			if (infile.key == "name") pc->stats.name = infile.val;
			else if (infile.key == "permadeath") {
				pc->stats.permadeath = toBool(infile.val);
			}
			else if (infile.key == "option") {
				pc->stats.gfx_base = popFirstString(infile.val);
				pc->stats.gfx_head = popFirstString(infile.val);
				pc->stats.gfx_portrait = popFirstString(infile.val);
			}
			else if (infile.key == "class") {
				pc->stats.character_class = popFirstString(infile.val);
				pc->stats.character_subclass = popFirstString(infile.val);
			}
			else if (infile.key == "soul") {
				pc->stats.soul = toUnsignedLong(infile.val);
			}
			else if (infile.key == "limitbreak") {
				pc->stats.limitbreak = toInt(infile.val);
			}
			else if (infile.key == "hullcapacitor") {
				saved_hull = popFirstInt(infile.val);
				saved_shield = popFirstInt(infile.val);
				saved_psi = popFirstInt(infile.val);
				saved_capacitor = popFirstInt(infile.val);
			}
			else if (infile.key == "build") {
				for (size_t i = 0; i < PRIMARY_STATS.size(); ++i) {
					pc->stats.primary[i] = popFirstInt(infile.val);
					if (pc->stats.primary[i] < 0 || pc->stats.primary[i] > pc->stats.max_points_per_stat) {
						logInfo("SaveLoad: Primary stat value for '%s' is out of bounds, setting to zero.", PRIMARY_STATS[i].id.c_str());
						pc->stats.primary[i] = 0;
					}
				}
				for (size_t i = 0; i < TALENTS.size(); ++i) {
					pc->stats.talent[i] = popFirstInt(infile.val);
					if (pc->stats.talent[i] < 0 || pc->stats.talent[i] > pc->stats.max_points_per_talent) {
						logInfo("SaveLoad: Talent value for '%s' is out of bounds, setting to zero.", TALENTS[i].id.c_str());
						pc->stats.talent[i] = 0;
					}
				}
			}
			else if (infile.key == "currency") {
				currency = toInt(infile.val);
			}
			else if (infile.key == "equipped") {
				menu->inv->inventory[EQUIPMENT].setItems(infile.val);
			}
			else if (infile.key == "equipped_quantity") {
				menu->inv->inventory[EQUIPMENT].setQuantities(infile.val);
			}
			else if (infile.key == "carried") {
				menu->inv->inventory[CARRIED].setItems(infile.val);
			}
			else if (infile.key == "carried_quantity") {
				menu->inv->inventory[CARRIED].setQuantities(infile.val);
			}
			else if (infile.key == "spawn") {
				mapr->teleport_mapname = popFirstString(infile.val);
				if (mapr->teleport_mapname != "" && fileExists(mods->locate(mapr->teleport_mapname))) {
					mapr->teleport_destination.x = static_cast<float>(popFirstInt(infile.val)) + 0.5f;
					mapr->teleport_destination.y = static_cast<float>(popFirstInt(infile.val)) + 0.5f;
					mapr->teleportation = true;
					// prevent spawn.txt from putting us on the starting map
					mapr->clearEvents();
				}
				else {
					logError("SaveLoad: Unable to find %s, loading maps/spawn.txt", mapr->teleport_mapname.c_str());
					mapr->teleport_mapname = "maps/spawn.txt";
					mapr->teleport_destination.x = 0.5f;
					mapr->teleport_destination.y = 0.5f;
					mapr->teleportation = true;
				}
			}
			else if (infile.key == "actionbar") {
				for (int i = 0; i < ACTIONBAR_MAX; i++) {
					hotkeys[i] = popFirstInt(infile.val);
					if (hotkeys[i] < 0) {
						logError("SaveLoad: Hotkey power on position %d has negative id, skipping", i);
						hotkeys[i] = 0;
					}
					else if (static_cast<unsigned>(hotkeys[i]) > powers->powers.size()-1) {
						logError("SaveLoad: Hotkey power id (%d) out of bounds 1-%d, skipping", hotkeys[i], static_cast<int>(powers->powers.size()));
						hotkeys[i] = 0;
					}
					else if (hotkeys[i] != 0 && static_cast<unsigned>(hotkeys[i]) < powers->powers.size() && powers->powers[hotkeys[i]].name == "") {
						logError("SaveLoad: Hotkey power with id=%d, found on position %d does not exist, skipping", hotkeys[i], i);
						hotkeys[i] = 0;
					}
				}
				menu->act->set(hotkeys);
			}
			else if (infile.key == "transformed") {
				pc->stats.transform_type = popFirstString(infile.val);
				if (pc->stats.transform_type != "") {
					pc->stats.transform_duration = -1;
					pc->stats.manual_untransform = toBool(popFirstString(infile.val));
				}
			}
			else if (infile.key == "powers") {
				std::string power;
				while ( (power = popFirstString(infile.val)) != "") {
					if (toInt(power) > 0)
						pc->stats.powers_list.push_back(toInt(power));
				}
			}
			else if (infile.key == "campaign") camp->setAll(infile.val);
		}

		infile.close();
	}
	else logError("SaveLoad: Unable to open %s!", ss.str().c_str());

	// add legacy currency to inventory
	menu->inv->addCurrency(currency);

	// apply stats, inventory, and powers
	applyPlayerData();

	// trigger passive effects here? Saved HP/MP values might depend on passively boosted HP/MP
	// powers->activatePassives(pc->stats);
	if (saved_hull > pc->stats.get(STAT_HULL_MAX)) {
		logError("SaveLoad: Hull value is out of bounds, setting to maximum");
		pc->stats.hull = pc->stats.get(STAT_HULL_MAX);
	}
	else if (saved_hull < 1)
	{ 
		logError("SaveLoad: Hull value is out of bounds, setting to minimum");
		pc->stats.hull = 1; 
	}
	else 
		pc->stats.hull = saved_hull;

	if (saved_capacitor > pc->stats.get(STAT_CAPACITOR_MAX)) {
		logError("SaveLoad: Capacitor value is out of bounds, setting to maximum");
		pc->stats.capacitor = pc->stats.get(STAT_CAPACITOR_MAX);
	}
	else if (saved_capacitor < 1)
	{ 
		logError("SaveLoad: Capacitor value is out of bounds, setting to minimum");
		pc->stats.capacitor = 1; 
	}
	else 
		pc->stats.capacitor = saved_capacitor;
		
	if (saved_psi > pc->stats.get(STAT_PSI_MAX)) {
		logError("SaveLoad: PSI value is out of bounds, setting to maximum");
		pc->stats.psi = pc->stats.get(STAT_PSI_MAX);
	}
	else if (saved_psi < 0)
	{ 
		logError("SaveLoad: PSI value is out of bounds, setting to minimum");
		pc->stats.psi = 0; 
	}
	else 
		pc->stats.psi = saved_psi;
	
	if (saved_shield > pc->stats.get(STAT_SHIELD_MAX)) {
		logError("SaveLoad: Shield value is out of bounds, setting to maximum");
		pc->stats.shield = pc->stats.get(STAT_SHIELD_MAX);
	}	
	else if (saved_shield < 0)
	{ 
		logError("SaveLoad: Shield value is out of bounds, setting to minimum");
		pc->stats.shield = 0; 
	}
	else 
		pc->stats.shield = saved_shield;

	// reset character menu
	menu->chr->refreshStats();

	loadPowerTree();
}

/**
 * Load a class definition, index
 */
void SaveLoad::loadClass(int index) {
	if (game_slot <= 0) return;

	if (index < 0 || static_cast<unsigned>(index) >= HERO_CLASSES.size()) {
		logError("SaveLoad: Class index out of bounds.");
		return;
	}

	pc->stats.character_class = HERO_CLASSES[index].name;
	for (size_t i = 0; i < PRIMARY_STATS.size(); ++i) {
		pc->stats.primary[i] += HERO_CLASSES[index].primaryStats[i];
	}
	for (size_t i = 0; i < TALENTS.size(); ++i) {
		pc->stats.talent[i] += HERO_CLASSES[index].talents[i];
	}
	menu->inv->addCurrency(HERO_CLASSES[index].currency);
	menu->inv->inventory[EQUIPMENT].setItems(HERO_CLASSES[index].equipment);
	for (unsigned i=0; i<HERO_CLASSES[index].powers.size(); i++) {
		pc->stats.powers_list.push_back(HERO_CLASSES[index].powers[i]);
	}
	for (unsigned i=0; i<HERO_CLASSES[index].statuses.size(); i++) {
		camp->setStatus(HERO_CLASSES[index].statuses[i]);
	}
	menu->act->set(HERO_CLASSES[index].hotkeys);

	// Add carried items
	std::string carried = HERO_CLASSES[index].carried;
	ItemStack stack;
	stack.quantity = 1;
	while (carried != "") {
		stack.item = popFirstInt(carried);
		menu->inv->add(stack, CARRIED, -1, false, false);
	}

	// apply stats, inventory, and powers
	applyPlayerData();

	// reset character menu
	menu->chr->refreshStats();

	loadPowerTree();
}

/**
 * This is used to load the trove when starting a new game
 */
void SaveLoad::loadTrove() {
	// Load trove
	FileParser infile;
	std::stringstream ss;
	if (pc->stats.permadeath)
		ss << PATH_USER << "saves/" << SAVE_PREFIX << "/" << game_slot << "/trove_HC.txt";
	else
		ss << PATH_USER << "saves/" << SAVE_PREFIX << "/trove.txt";

	if (infile.open(path(&ss), false)) {
		while (infile.next()) {
			if (infile.key == "item") {
				menu->trove->stock.setItems(infile.val);
			}
			else if (infile.key == "quantity") {
				menu->trove->stock.setQuantities(infile.val);
			}
		}
		infile.close();
	}
	else logError("SaveLoad: Unable to open %s!", ss.str().c_str());

	menu->trove->stock.clean();
}

/**
 * Performs final calculations after loading a save or a new class
 */
void SaveLoad::applyPlayerData() {
	menu->inv->fillEquipmentSlots();

	// remove items with zero quantity from inventory
	menu->inv->inventory[EQUIPMENT].clean();
	menu->inv->inventory[CARRIED].clean();

	// Load trove
	loadTrove();

	// initialize vars
	pc->stats.recalc();
	pc->stats.loadHeroSFX();
	menu->inv->applyEquipment();
	pc->stats.logic(); // run stat logic once to apply items bonuses

	// just for aesthetics, turn the hero to face the camera
	pc->stats.direction = 6;

	// set up MenuTalker for this hero
	menu->talker->setHero(pc->stats);

	// load sounds (gender specific)
	pc->loadSounds();

	// apply power upgrades
	menu->pow->applyPowerUpgrades();
}

void SaveLoad::loadPowerTree() {
	for (unsigned i=0; i<HERO_CLASSES.size(); ++i) {
		if (pc->stats.character_class == HERO_CLASSES[i].name) {
			if (HERO_CLASSES[i].power_tree != "") {
				menu->pow->loadPowerTree(HERO_CLASSES[i].power_tree);
				return;
			}
			else
				break;
		}
	}

	// fall back to the default power tree
	menu->pow->loadPowerTree("powers/trees/default.txt");
}

