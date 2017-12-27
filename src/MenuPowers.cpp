/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
Copyright © 2012 Stefan Beller
Copyright © 2013-2014 Henrik Andersson
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
 * class MenuPowers
 */

#include "CommonIncludes.h"
#include "Menu.h"
#include "MenuPowers.h"
#include "Settings.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "StatBlock.h"
#include "UtilsParsing.h"
#include "WidgetLabel.h"
#include "WidgetSlot.h"
#include "TooltipData.h"
#include "MenuActionBar.h"

#include <climits>

MenuPowers::MenuPowers(StatBlock *_stats, MenuActionBar *_action_bar)
	: stats(_stats)
	, action_bar(_action_bar)
	, skip_section(false)
	, powers_unlock(NULL)
	, overlay_disabled(NULL)
	, points_left(0)
	, default_background("")
	, tab_control(NULL)
	, tree_loaded(false)
	, newPowerNotification(false)
{

	closeButton = new WidgetButton("images/menus/buttons/button_x.png");

	// Read powers data from config file
	FileParser infile;
	// @CLASS MenuPowers: Menu layout|Description of menus/powers.txt
	if (infile.open("menus/powers.txt")) {
		while (infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;

			// @ATTR label_title|label|Position of the "Powers" text.
			if (infile.key == "label_title") title = eatLabelInfo(infile.val);
			// @ATTR unspent_points|label|Position of the text that displays the amount of unused power points.
			else if (infile.key == "unspent_points") unspent_points = eatLabelInfo(infile.val);
			// @ATTR close|point|Position of the close button.
			else if (infile.key == "close") close_pos = toPoint(infile.val);
			// @ATTR tab_area|rectangle|Position and dimensions of the tree pages.
			else if (infile.key == "tab_area") tab_area = toRect(infile.val);

			else infile.error("MenuPowers: '%s' is not a valid key.", infile.key.c_str());
		}
		infile.close();
	}

	loadGraphics();

	menu_powers = this;

	color_bonus = font->getColor("menu_bonus");
	color_penalty = font->getColor("menu_penalty");
	color_flavor = font->getColor("item_flavor");

	align();
}

MenuPowers::~MenuPowers() {
	if (powers_unlock) delete powers_unlock;
	if (overlay_disabled) delete overlay_disabled;

	for (size_t i=0; i<tree_surf.size(); i++) {
		if (tree_surf[i]) delete tree_surf[i];
	}
	for (size_t i=0; i<slots.size(); i++) {
		delete slots.at(i);
		delete upgradeButtons.at(i);
	}
	slots.clear();
	upgradeButtons.clear();

	delete closeButton;
	if (tab_control) delete tab_control;
	menu_powers = NULL;
}

void MenuPowers::align() {
	Menu::align();

	label_powers.set(window_area.x+title.x, window_area.y+title.y, title.justify, title.valign, msg->get("POWERS"), font->getColor("menu_header"), title.font_style);

	closeButton->pos.x = window_area.x+close_pos.x;
	closeButton->pos.y = window_area.y+close_pos.y;

	stat_up.set(window_area.x+unspent_points.x, window_area.y+unspent_points.y, unspent_points.justify, unspent_points.valign, "", font->getColor("menu_bonus"), unspent_points.font_style);

	if (tab_control) {
		tab_control->setMainArea(window_area.x+tab_area.x, window_area.y+tab_area.y, tab_area.w, tab_area.h);
		tab_control->updateHeader();
	}

	for (size_t i=0; i<slots.size(); i++) {
		if (!slots[i]) continue;

		slots[i]->setPos(window_area.x, window_area.y);

		if (upgradeButtons[i] != NULL) {
			upgradeButtons[i]->setPos(window_area.x, window_area.y);
		}
	}

}

void MenuPowers::loadGraphics() {

	Image *graphics;

	setBackground("images/menus/powers.png");

	graphics = render_device->loadImage("images/menus/powers_unlock.png");
	if (graphics) {
		powers_unlock = graphics->createSprite();
		graphics->unref();
	}

	graphics = render_device->loadImage("images/menus/disabled.png");
	if (graphics) {
		overlay_disabled = graphics->createSprite();
		graphics->unref();
	}
}

/**
 * Loads a given power tree and sets up the menu accordingly
 *
 * @param filename Path to the file that will be loaded
 */
void MenuPowers::loadPowerTree(const std::string &filename) {
	// only load the power tree once per instance
	if (tree_loaded) return;

	// First, parse the power tree file

	FileParser infile;
	// @CLASS MenuPowers: Power tree layout|Description of powers/trees/
	if (infile.open(filename)) {
		while (infile.next()) {
			if (infile.new_section) {
				// for sections that are stored in collections, add a new object here
				if (infile.section == "power") {
					slots.push_back(NULL);
					upgradeButtons.push_back(NULL);
					power_cell.push_back(Power_Menu_Cell());
				}
				else if (infile.section == "upgrade")
					power_cell_upgrade.push_back(Power_Menu_Cell());
				else if (infile.section == "tab")
					tabs.push_back(Power_Menu_Tab());
			}

			if (infile.section == "") {
				// @ATTR background|filename|Filename of the default background image
				if (infile.key == "background") default_background = infile.val;
			}
			else if (infile.section == "tab")
				loadTab(infile);
			else if (infile.section == "power")
				loadPower(infile);
			else if (infile.section == "upgrade")
				loadUpgrade(infile);
		}
		infile.close();
	}

	// save a copy of the base level powers, as they get overwritten during upgrades
	power_cell_base = power_cell;

	// store the appropriate level for all upgrades
	for (size_t i=0; i<power_cell_upgrade.size(); ++i) {
		for (size_t j=0; j<power_cell_base.size(); j++) {
			std::vector<int>::iterator it = std::find(power_cell_base[j].upgrades.begin(), power_cell_base[j].upgrades.end(), power_cell_upgrade[i].id);
			if (it != power_cell_base[j].upgrades.end()) {
				power_cell_upgrade[i].upgrade_level = static_cast<int>(std::distance(power_cell_base[j].upgrades.begin(), it) + 2);
				break;
			}
		}
	}

	// combine base and upgrade powers into a single list
	for (size_t i=0; i<power_cell_base.size(); ++i) {
		power_cell_all.push_back(power_cell_base[i]);
	}
	for (size_t i=0; i<power_cell_upgrade.size(); ++i) {
		power_cell_all.push_back(power_cell_upgrade[i]);
	}

	// save cell indexes for required powers
	for (size_t i=0; i<power_cell_all.size(); ++i) {
		for (size_t j=0; j<power_cell_all[i].requires_power.size(); ++j) {
			int cell_index = getCellByPowerIndex(power_cell_all[i].requires_power[j], power_cell_all);
			power_cell_all[i].requires_power_cell.push_back(cell_index);
		}
	}

	// load any specified graphics into the tree_surf vector
	Image *graphics;
	if (tabs.empty() && default_background != "") {
		graphics = render_device->loadImage(default_background);
		if (graphics) {
			tree_surf.push_back(graphics->createSprite());
			graphics->unref();
		}
	}
	else {
		for (size_t i=0; i<tabs.size(); ++i) {
			if (tabs[i].background == "")
				tabs[i].background = default_background;

			if (tabs[i].background == "") {
				tree_surf.push_back(NULL);
				continue;
			}

			graphics = render_device->loadImage(tabs[i].background);
			if (graphics) {
				tree_surf.push_back(graphics->createSprite());
				graphics->unref();
			}
			else {
				tree_surf.push_back(NULL);
			}
		}
	}

	// If we have more than one tab, create tab_control
	if (!tabs.empty()) {
		tab_control = new WidgetTabControl();

		if (tab_control) {
			// Initialize the tab control.
			tab_control->setMainArea(window_area.x+tab_area.x, window_area.y+tab_area.y, tab_area.w, tab_area.h);

			// Define the header.
			for (size_t i=0; i<tabs.size(); i++)
				tab_control->setTabTitle(static_cast<unsigned>(i), msg->get(tabs[i].title));
			tab_control->updateHeader();

			tablist.add(tab_control);
		}

		tablist_pow.resize(tabs.size());
	}

	// create power slots
	for (size_t i=0; i<slots.size(); i++) {
		if (static_cast<size_t>(power_cell[i].id) < powers->powers.size()) {
			slots[i] = new WidgetSlot(powers->powers[power_cell[i].id].icon);
			slots[i]->setBasePos(power_cell[i].pos.x, power_cell[i].pos.y);

			if (!tablist_pow.empty()) {
				tablist_pow[power_cell[i].tab].add(slots[i]);
				tablist_pow[power_cell[i].tab].setPrevTabList(&tablist);
				tablist_pow[power_cell[i].tab].lock();
			}
			else {
				tablist.add(slots[i]);
			}

			if (upgradeButtons[i] != NULL) {
				upgradeButtons[i]->setBasePos(power_cell[i].pos.x + ICON_SIZE, power_cell[i].pos.y);
			}
		}
	}

	applyPowerUpgrades();

	tree_loaded = true;

	align();
}

void MenuPowers::loadTab(FileParser &infile) {
	// @ATTR tab.title|string|The name of this power tree tab
	if (infile.key == "title") tabs.back().title = infile.val;
	// @ATTR tab.background|filename|Filename of the background image for this tab's power tree
	else if (infile.key == "background") tabs.back().background = infile.val;
}

void MenuPowers::loadPower(FileParser &infile) {
	// @ATTR power.id|int|A power id from powers/powers.txt for this slot.
	if (infile.key == "id") {
		int id = popFirstInt(infile.val);
		if (id > 0) {
			skip_section = false;
			power_cell.back().id = id;
		}
		else {
			infile.error("MenuPowers: Power index out of bounds 1-%d, skipping power.", INT_MAX);
		}
		return;
	}

	if (power_cell.back().id <= 0) {
		skip_section = true;
		power_cell.pop_back();
		slots.pop_back();
		upgradeButtons.pop_back();
		logError("MenuPowers: There is a power without a valid id as the first attribute. IDs must be the first attribute in the power menu definition.");
	}

	if (skip_section)
		return;

	// @ATTR power.tab|int|Tab index to place this power on, starting from 0.
	if (infile.key == "tab") power_cell.back().tab = toInt(infile.val);
	// @ATTR power.position|point|Position of this power icon; relative to MenuPowers "pos".
	else if (infile.key == "position") power_cell.back().pos = toPoint(infile.val);

	// @ATTR power.requires_primary|predefined_string, int : Primary stat name, Required value|Power requires this primary stat to be at least the specificed value.
	else if (infile.key == "requires_primary") {
		std::string prim_stat = popFirstString(infile.val);
		size_t prim_stat_index = getPrimaryStatIndex(prim_stat);

		if (prim_stat_index != PRIMARY_STATS.size()) {
			power_cell.back().requires_primary[prim_stat_index] = toInt(infile.val);
		}
		else {
			infile.error("MenuPowers: '%s' is not a valid primary stat.", prim_stat.c_str());
		}
	}
	// @ATTR power.requires_point|bool|Power requires a power point to unlock.
	else if (infile.key == "requires_point") power_cell.back().requires_point = toBool(infile.val);
	// @ATTR power.requires_generation|int|Power requires at least this generation for the hero.
	else if (infile.key == "requires_generation") power_cell.back().requires_generation = toInt(infile.val);
	// @ATTR power.requires_power|power_id|Power requires another power id.
	else if (infile.key == "requires_power") power_cell.back().requires_power.push_back(toInt(infile.val));

	// @ATTR power.visible_requires_status|repeatable(string)|Hide the power if we don't have this campaign status.
	else if (infile.key == "visible_requires_status") power_cell.back().visible_requires_status.push_back(infile.val);
	// @ATTR power.visible_requires_not_status|repeatable(string)|Hide the power if we have this campaign status.
	else if (infile.key == "visible_requires_not_status") power_cell.back().visible_requires_not.push_back(infile.val);

	// @ATTR power.upgrades|list(power_id)|A list of upgrade power ids that this power slot can upgrade to. Each of these powers should have a matching upgrade section.
	else if (infile.key == "upgrades") {
		if (power_cell.back().upgrades.empty()) {
			upgradeButtons.back() = new WidgetButton("images/menus/buttons/button_plus.png");
		}

		std::string repeat_val = popFirstString(infile.val);
		while (repeat_val != "") {
			power_cell.back().upgrades.push_back(toInt(repeat_val));
			repeat_val = popFirstString(infile.val);
		}

		if (!power_cell.back().upgrades.empty())
			power_cell.back().upgrade_level = 1;
	}

	else infile.error("MenuPowers: '%s' is not a valid key.", infile.key.c_str());
}

void MenuPowers::loadUpgrade(FileParser &infile) {
	// @ATTR upgrade.id|int|A power id from powers/powers.txt for this upgrade.
	if (infile.key == "id") {
		int id = popFirstInt(infile.val);
		if (id > 0) {
			skip_section = false;
			power_cell_upgrade.back().id = (id);
		}
		else {
			skip_section = true;
			power_cell_upgrade.pop_back();
			infile.error("MenuPowers: Power index out of bounds 1-%d, skipping power.", INT_MAX);
		}
		return;
	}

	if (skip_section)
		return;

	// @ATTR upgrade.requires_primary|predefined_string, int : Primary stat name, Required value|Upgrade requires this primary stat to be at least the specificed value.
	if (infile.key == "requires_primary") {
		std::string prim_stat = popFirstString(infile.val);
		size_t prim_stat_index = getPrimaryStatIndex(prim_stat);

		if (prim_stat_index != PRIMARY_STATS.size()) {
			power_cell_upgrade.back().requires_primary[prim_stat_index] = toInt(infile.val);
		}
		else {
			infile.error("MenuPowers: '%s' is not a valid primary stat.", prim_stat.c_str());
		}
	}
	// @ATTR upgrade.requires_point|bool|Upgrade requires a power point to unlock.
	else if (infile.key == "requires_point") power_cell_upgrade.back().requires_point = toBool(infile.val);
	// @ATTR upgrade.requires_generation|int|Upgrade requires at least this generation for the hero.
	else if (infile.key == "requires_generation") power_cell_upgrade.back().requires_generation = toInt(infile.val);
	// @ATTR upgrade.requires_power|int|Upgrade requires another power id.
	else if (infile.key == "requires_power") power_cell_upgrade.back().requires_power.push_back(toInt(infile.val));

	// @ATTR upgrade.visible_requires_status|repeatable(string)|Hide the upgrade if we don't have this campaign status.
	else if (infile.key == "visible_requires_status") power_cell_upgrade.back().visible_requires_status.push_back(infile.val);
	// @ATTR upgrade.visible_requires_not_status|repeatable(string)|Hide the upgrade if we have this campaign status.
	else if (infile.key == "visible_requires_not_status") power_cell_upgrade.back().visible_requires_not.push_back(infile.val);

	else infile.error("MenuPowers: '%s' is not a valid key.", infile.key.c_str());
}

bool MenuPowers::checkRequirements(int pci) {
	if (pci == -1)
		return false;

	for (size_t i = 0; i < power_cell_all[pci].requires_power_cell.size(); ++i)
		if (!checkUnlocked(power_cell_all[pci].requires_power_cell[i]))
			return false;

	for (size_t i = 0; i < PRIMARY_STATS.size(); ++i) {
		if (stats->get_primary(i) < power_cell_all[pci].requires_primary[i])
			return false;
	}

	return true;
}

bool MenuPowers::checkUnlocked(int pci) {
	// If we didn't find power in power_menu, than it has no requirements
	if (pci == -1) return true;

	if (!checkCellVisible(pci)) return false;

	// If power_id is saved into vector, it's unlocked anyway
	// check power_cell_unlocked and stats->powers_list
	for (size_t i=0; i<power_cell_unlocked.size(); ++i) {
		if (power_cell_unlocked[i].id == power_cell_all[pci].id)
			return true;
	}
	if (std::find(stats->powers_list.begin(), stats->powers_list.end(), power_cell_all[pci].id) != stats->powers_list.end()) return true;

	// Check the rest requirements
	if (checkRequirements(pci) && !power_cell_all[pci].requires_point) return true;
	return false;
}

bool MenuPowers::checkCellVisible(int pci) {
	// If we didn't find power in power_menu, than it has no requirements
	if (pci == -1) return true;

	for (size_t i=0; i<power_cell_all[pci].visible_requires_status.size(); ++i)
		if (!camp->checkStatus(power_cell_all[pci].visible_requires_status[i]))
			return false;

	for (size_t i=0; i<power_cell_all[pci].visible_requires_not.size(); ++i)
		if (camp->checkStatus(power_cell_all[pci].visible_requires_not[i]))
			return false;

	return true;
}

/**
 * Check if we can unlock power.
 */
bool MenuPowers::checkUnlock(int pci) {
	// If we didn't find power in power_menu, than it has no requirements
	if (pci == -1) return true;

	if (!checkCellVisible(pci)) return false;

	// If we already have a power, don't try to unlock it
	if (checkUnlocked(pci)) return false;

	// Check base requirements
	if (checkRequirements(pci)) return true;
	return false;
}

bool MenuPowers::checkUpgrade(int pci) {
	if (points_left < 1)
		return false;

	int id = getCellByPowerIndex(power_cell[pci].id, power_cell_all);
	if (!checkUnlocked(id))
		return false;

	int next_index = getNextLevelCell(pci);
	if (next_index == -1)
		return false;
	if (!power_cell_upgrade[next_index].requires_point)
		return false;

	int id_upgrade = getCellByPowerIndex(power_cell_upgrade[next_index].id, power_cell_all);
	if (!checkUnlock(id_upgrade))
		return false;

	return true;
}

int MenuPowers::getCellByPowerIndex(int power_index, const std::vector<Power_Menu_Cell>& cell) {
	// Powers can not have an id of 0
	if (power_index == 0) return -1;

	// Find cell with our power
	for (size_t i=0; i<cell.size(); i++)
		if (cell[i].id == power_index)
			return static_cast<int>(i);

	return -1;
}

/**
 * Find cell in upgrades with next upgrade for current power_cell
 */
int MenuPowers::getNextLevelCell(int pci) {
	if (power_cell[pci].upgrades.empty()) {
		return -1;
	}

	std::vector<int>::iterator level_it;
	level_it = std::find(power_cell[pci].upgrades.begin(),
					power_cell[pci].upgrades.end(),
					power_cell[pci].id);

	if (level_it == power_cell[pci].upgrades.end()) {
		// current power is base power, take first upgrade
		int index = power_cell[pci].upgrades[0];
		return getCellByPowerIndex(index, power_cell_upgrade);
	}
	// current power is an upgrade, take next upgrade if avaliable
	int index = static_cast<int>(std::distance(power_cell[pci].upgrades.begin(), level_it));
	if (static_cast<int>(power_cell[pci].upgrades.size()) > index + 1) {
		return getCellByPowerIndex(*(++level_it), power_cell_upgrade);
	}
	else {
		return -1;
	}
}

void MenuPowers::replaceCellWithUpgrade(int pci, int uci) {
	power_cell[pci].id = power_cell_upgrade[uci].id;
	for (size_t i = 0; i < PRIMARY_STATS.size(); ++i) {
		power_cell[pci].requires_primary[i] = power_cell_upgrade[uci].requires_primary[i];
	}
	power_cell[pci].requires_generation = power_cell_upgrade[uci].requires_generation;
	power_cell[pci].requires_power = power_cell_upgrade[uci].requires_power;
	power_cell[pci].requires_power_cell = power_cell_upgrade[uci].requires_power_cell;
	power_cell[pci].requires_point = power_cell_upgrade[uci].requires_point;
	power_cell[pci].passive_on = power_cell_upgrade[uci].passive_on;
	power_cell[pci].upgrade_level = power_cell_upgrade[uci].upgrade_level;

	if (slots[pci])
		slots[pci]->setIcon(powers->powers[power_cell_upgrade[uci].id].icon);
}

/**
 * Upgrade power cell "pci" to the next level
 */
void MenuPowers::upgradePower(int pci) {
	int i = getNextLevelCell(pci);
	if (i == -1)
		return;

	// if power was present in ActionBar, update it there
	action_bar->addPower(power_cell_upgrade[i].id, power_cell[pci].id);

	// if we have tab_control
	if (tab_control) {
		int active_tab = tab_control->getActiveTab();
		if (power_cell[pci].tab == active_tab) {
			replaceCellWithUpgrade(pci, i);
			stats->powers_list.push_back(power_cell_upgrade[i].id);
			stats->check_title = true;
		}
	}
	// if have don't have tabs
	else {
		replaceCellWithUpgrade(pci, i);
		stats->powers_list.push_back(power_cell_upgrade[i].id);
		stats->check_title = true;
	}
	setUnlockedPowers();
}

void MenuPowers::setUnlockedPowers() {
	std::vector<int> power_ids;
	power_cell_unlocked.clear();

	for (size_t i=0; i<power_cell.size(); ++i) {
		if (std::find(stats->powers_list.begin(), stats->powers_list.end(), power_cell[i].id) != stats->powers_list.end()) {
			// base power
			if (std::find(power_ids.begin(), power_ids.end(), power_cell[i].id) == power_ids.end()) {
				power_ids.push_back(power_cell_base[i].id);
				power_cell_unlocked.push_back(power_cell_base[i]);
			}
			if (power_cell[i].id == power_cell_base[i].id)
				continue;

			//upgrades
			for (size_t j=0; j<power_cell[i].upgrades.size(); ++j) {
				if (std::find(power_ids.begin(), power_ids.end(), power_cell[i].upgrades[j]) == power_ids.end()) {
					int id = getCellByPowerIndex(power_cell[i].upgrades[j], power_cell_upgrade);
					if (id != -1) {
						power_ids.push_back(power_cell[i].upgrades[j]);
						power_cell_unlocked.push_back(power_cell_upgrade[id]);

						if (power_cell[i].id == power_cell[i].upgrades[j])
							break;
					}
					else {
						break;
					}
				}
			}
		}
	}
}

int MenuPowers::getPointsUsed() {
	int used = 0;
	for (size_t i=0; i<power_cell_unlocked.size(); ++i) {
		if (power_cell_unlocked[i].requires_point)
			used++;
	}

	return used;
}

void MenuPowers::createTooltip(TooltipData* tip, int slot_num, const std::vector<Power_Menu_Cell>& power_cells, bool show_unlock_prompt) {
	if (power_cells[slot_num].upgrade_level > 0)
		tip->addText(powers->powers[power_cells[slot_num].id].name + " (" + msg->get("Level %d", power_cells[slot_num].upgrade_level) + ")");
	else
		tip->addText(powers->powers[power_cells[slot_num].id].name);

	if (powers->powers[power_cells[slot_num].id].passive) tip->addText("Passive");
	tip->addColoredText(substituteVarsInString(powers->powers[power_cells[slot_num].id].description, pc), color_flavor);


	// add capacitor cost
	if (powers->powers[power_cells[slot_num].id].requires_capacitor > 0) {
		tip->addText(msg->get("Costs %d Capacitor", powers->powers[power_cells[slot_num].id].requires_capacitor));
	}
	// add psi cost
	if (powers->powers[power_cells[slot_num].id].requires_psi > 0) {
		tip->addText(msg->get("Costs %d PSI", powers->powers[power_cells[slot_num].id].requires_psi));
	}
	// add hull cost
	if (powers->powers[power_cells[slot_num].id].requires_hull > 0) {
		tip->addText(msg->get("Costs %d Hull", powers->powers[power_cells[slot_num].id].requires_hull));
	}
	// add limit cost
	if (powers->powers[power_cells[slot_num].id].requires_limitbreak > 0) {
		tip->addText(msg->get("Costs %d Limitbreak", powers->powers[power_cells[slot_num].id].requires_limitbreak));
	}
	// add cooldown time
	if (powers->powers[power_cells[slot_num].id].cooldown > 0) {
		std::stringstream ss;
		ss << msg->get("Cooldown:") << " " << getDurationString(powers->powers[power_cells[slot_num].id].cooldown);
		tip->addText(ss.str());
	}

	const Power &pwr = powers->powers[power_cells[slot_num].id];
	for (size_t i=0; i<pwr.post_effects.size(); ++i) {
		std::stringstream ss;
		EffectDef* effect_ptr = powers->getEffectDef(pwr.post_effects[i].id);

		// base stats
		if (effect_ptr == NULL) {
			if (pwr.post_effects[i].magnitude > 0) {
				ss << "+";
			}

			if (pwr.passive) {
				// since leveled passive powers stack, we need to get the total magnitude for all levels
				// TODO: don't stack leveled passive powers?
				ss << pwr.post_effects[i].magnitude * power_cells[slot_num].upgrade_level;
			}
			else {
				ss << pwr.post_effects[i].magnitude;
			}

			for (size_t j=0; j<STAT_COUNT; ++j) {
				if (pwr.post_effects[i].id == STAT_KEY[j]) {
					if (STAT_PERCENT[j])
						ss << "%";

					ss << " " << STAT_NAME[j];

					break;
				}
			}

			for (size_t j=0; j<ELEMENTS.size(); ++j) {
				if (pwr.post_effects[i].id == ELEMENTS[j].id + "_resist") {
					ss << "% " << msg->get("%s Resistance", ELEMENTS[j].name.c_str());
					break;
				}
			}
			for (size_t j=0; j<PRIMARY_STATS.size(); ++j) {
				if (pwr.post_effects[i].id == PRIMARY_STATS[j].id) {
					ss << " " << PRIMARY_STATS[j].name;
					break;
				}
			}
		}
		else {
			if (effect_ptr->type == "damage") {
				ss << pwr.post_effects[i].magnitude << " " << msg->get("Damage per second");
			}
			else if (effect_ptr->type == "damage_percent") {
				ss << pwr.post_effects[i].magnitude << "% " << msg->get("Damage per second");
			}
			else if (effect_ptr->type == "hpot") {
				ss << pwr.post_effects[i].magnitude << " " << msg->get("Hull per second");
			}
			else if (effect_ptr->type == "hpot_percent") {
				ss << pwr.post_effects[i].magnitude << "% " << msg->get("Hull per second");
			}
			else if (effect_ptr->type == "mpot") {
				ss << pwr.post_effects[i].magnitude << " " << msg->get("PSI per second");
			}
			else if (effect_ptr->type == "mpot_percent") {
				ss << pwr.post_effects[i].magnitude << "% " << msg->get("PSI per second");
			}
			else if (effect_ptr->type == "speed") {
				if (pwr.post_effects[i].magnitude == 0)
					ss << msg->get("Immobilize");
				else
					ss << msg->get("%d%% Speed", pwr.post_effects[i].magnitude);
			}
			else if (effect_ptr->type == "attack_speed") {
				ss << msg->get("%d%% Attack Speed", pwr.post_effects[i].magnitude);
			}
			else if (effect_ptr->type == "immunity") {
				ss << msg->get("Immunity");
			}
			else if (effect_ptr->type == "immunity_damage") {
				ss << msg->get("Immunity to damage over time");
			}
			else if (effect_ptr->type == "immunity_slow") {
				ss << msg->get("Immunity to slow");
			}
			else if (effect_ptr->type == "immunity_stun") {
				ss << msg->get("Immunity to stun");
			}
			else if (effect_ptr->type == "immunity_hull_steal") {
				ss << msg->get("Immunity to Hull steal");
			}
			else if (effect_ptr->type == "immunity_psi_steal") {
				ss << msg->get("Immunity to PSI steal");
			}
			else if (effect_ptr->type == "immunity_capacitor_steal") {
				ss << msg->get("Immunity to Capacitor steal");
			}
			else if (effect_ptr->type == "immunity_knockback") {
				ss << msg->get("Immunity to knockback");
			}
			else if (effect_ptr->type == "immunity_damage_reflect") {
				ss << msg->get("Immunity to damage reflection");
			}
			else if (effect_ptr->type == "stun") {
				ss << msg->get("Stun");
			}
			else if (effect_ptr->type == "revive") {
				ss << msg->get("Automatic revive on death");
			}
			else if (effect_ptr->type == "convert") {
				ss << msg->get("Convert");
			}
			else if (effect_ptr->type == "fear") {
				ss << msg->get("Fear");
			}
			else if (effect_ptr->type == PRIMARY_STATS[4].id.c_str()) {
				ss << pwr.post_effects[i].magnitude << " " << msg->get(PRIMARY_STATS[4].name.c_str());//Luck
			}
			else if (effect_ptr->type == PRIMARY_STATS[2].id.c_str()) {
				ss << pwr.post_effects[i].magnitude << " " << msg->get(PRIMARY_STATS[2].name.c_str());//Nimble
			}
			else if (effect_ptr->type == PRIMARY_STATS[0].id.c_str()) {
				ss << pwr.post_effects[i].magnitude << " " << msg->get(PRIMARY_STATS[0].name.c_str());//Brawn
			}
			else if (effect_ptr->type == PRIMARY_STATS[1].id.c_str()) {
				ss << pwr.post_effects[i].magnitude << " " << msg->get(PRIMARY_STATS[1].name.c_str());//Conjure
			}
			else if (effect_ptr->type == PRIMARY_STATS[7].id.c_str()) {
				ss << pwr.post_effects[i].magnitude << " " << msg->get(PRIMARY_STATS[7].name.c_str());//Concentration
			}
			else if (effect_ptr->type == PRIMARY_STATS[3].id.c_str()) {
				ss << pwr.post_effects[i].magnitude << " " << msg->get(PRIMARY_STATS[3].name.c_str());//Resolve
			}
			else if (effect_ptr->type == PRIMARY_STATS[6].id.c_str()) {
				ss << pwr.post_effects[i].magnitude << " " << msg->get(PRIMARY_STATS[6].name.c_str());//Resilience
			}
			else if (effect_ptr->type == PRIMARY_STATS[5].id.c_str()) {
				ss << pwr.post_effects[i].magnitude << " " << msg->get(PRIMARY_STATS[5].name.c_str());//Influence
			}
			else if (effect_ptr->type == "death_sentence") {
				ss << msg->get("Lifespan");
			}
			else if (effect_ptr->type == "shield") {
				if (pwr.base_damage == DAMAGE_TYPES.size())
					continue;

				if (pwr.mod_damage_mode == STAT_MODIFIER_MODE_MULTIPLY) {
					int magnitude = stats->getDamageMax(pwr.base_damage) * pwr.mod_damage_value_min / 100;
					ss << magnitude;
				}
				else if (pwr.mod_damage_mode == STAT_MODIFIER_MODE_ADD) {
					int magnitude = stats->getDamageMax(pwr.base_damage) + pwr.mod_damage_value_min;
					ss << magnitude;
				}
				else if (pwr.mod_damage_mode == STAT_MODIFIER_MODE_ABSOLUTE) {
					if (pwr.mod_damage_value_max == 0 || pwr.mod_damage_value_min == pwr.mod_damage_value_max)
						ss << pwr.mod_damage_value_min;
					else
						ss << pwr.mod_damage_value_min << "-" << pwr.mod_damage_value_max;
				}
				else {
					ss << stats->getDamageMax(pwr.base_damage);
				}

				ss << " " << msg->get("Magical Shield");
			}
			else if (effect_ptr->type == "heal") {
				if (pwr.base_damage == DAMAGE_TYPES.size())
					continue;

				int mag_min = stats->getDamageMin(pwr.base_damage);
				int mag_max = stats->getDamageMax(pwr.base_damage);

				if (pwr.mod_damage_mode == STAT_MODIFIER_MODE_MULTIPLY) {
					mag_min = mag_min * pwr.mod_damage_value_min / 100;
					mag_max = mag_max * pwr.mod_damage_value_min / 100;
					ss << mag_min << "-" << mag_max;
				}
				else if (pwr.mod_damage_mode == STAT_MODIFIER_MODE_ADD) {
					mag_min = mag_min + pwr.mod_damage_value_min;
					mag_max = mag_max + pwr.mod_damage_value_min;
					ss << mag_min << "-" << mag_max;
				}
				else if (pwr.mod_damage_mode == STAT_MODIFIER_MODE_ABSOLUTE) {
					if (pwr.mod_damage_value_max == 0 || pwr.mod_damage_value_min == pwr.mod_damage_value_max)
						ss << pwr.mod_damage_value_min;
					else
						ss << pwr.mod_damage_value_min << "-" << pwr.mod_damage_value_max;
				}
				else {
					ss << mag_min << "-" << mag_max;
				}

				ss << " " << msg->get("Healing");
			}
			else if (effect_ptr->type == "knockback") {
				ss << pwr.post_effects[i].magnitude << " " << msg->get("Knockback");
			}
			else if (pwr.post_effects[i].magnitude == 0) {
				// nothing
			}
		}

		if (!ss.str().empty()) {
			if (pwr.post_effects[i].duration > 0) {
				if (effect_ptr && effect_ptr->type == "death_sentence") {
					ss << ": " << getDurationString(pwr.post_effects[i].duration);
				}
				else {
					ss << " (" << getDurationString(pwr.post_effects[i].duration) << ")";
				}
			}

			tip->addColoredText(ss.str(), color_bonus);
		}
	}

	if (pwr.use_hazard || pwr.type == POWTYPE_REPEATER) {
		std::stringstream ss;

		// modifier_damage
		if (pwr.mod_damage_mode > -1) {
			if (pwr.mod_damage_mode == STAT_MODIFIER_MODE_ADD && pwr.mod_damage_value_min > 0)
				ss << "+";

			if (pwr.mod_damage_value_max == 0 || pwr.mod_damage_value_min == pwr.mod_damage_value_max) {
				ss << pwr.mod_damage_value_min;
			}
			else {
				ss << pwr.mod_damage_value_min << "-" << pwr.mod_damage_value_max;
			}

			if (pwr.mod_damage_mode == STAT_MODIFIER_MODE_MULTIPLY) {
				ss << "%";
			}
			ss << " ";

			if (pwr.base_damage != DAMAGE_TYPES.size()) {
				ss << DAMAGE_TYPES[pwr.base_damage].text;
			}

			if (pwr.count > 1 && pwr.type != POWTYPE_REPEATER)
				ss << " (x" << pwr.count << ")";

			if (!ss.str().empty())
				tip->addColoredText(ss.str(), color_bonus);
		}

		// modifier_accuracy
		if (pwr.mod_accuracy_mode > -1) {
			ss.str("");

			if (pwr.mod_accuracy_mode == STAT_MODIFIER_MODE_ADD && pwr.mod_accuracy_value > 0)
				ss << "+";

			ss << pwr.mod_accuracy_value;

			if (pwr.mod_accuracy_mode == STAT_MODIFIER_MODE_MULTIPLY) {
				ss << "%";
			}
			ss << " ";

			ss << msg->get("Base Accuracy");

			if (!ss.str().empty())
				tip->addColoredText(ss.str(), color_bonus);
		}

		// modifier_critical
		if (pwr.mod_crit_mode > -1) {
			ss.str("");

			if (pwr.mod_crit_mode == STAT_MODIFIER_MODE_ADD && pwr.mod_crit_value > 0)
				ss << "+";

			ss << pwr.mod_crit_value;

			if (pwr.mod_crit_mode == STAT_MODIFIER_MODE_MULTIPLY) {
				ss << "%";
			}
			ss << " ";

			ss << msg->get("Base Critical Chance");

			if (!ss.str().empty())
				tip->addColoredText(ss.str(), color_bonus);
		}

		if (pwr.trait_armor_penetration) {
			ss.str("");
			ss << msg->get("Ignores Absorbtion");
			tip->addColoredText(ss.str(), color_bonus);
		}
		if (pwr.trait_avoidance_ignore) {
			ss.str("");
			ss << msg->get("Ignores Avoidance");
			tip->addColoredText(ss.str(), color_bonus);
		}
		if (pwr.trait_crits_impaired > 0) {
			ss.str("");
			ss << msg->get("%d%% Chance to crit slowed targets", pwr.trait_crits_impaired);
			tip->addColoredText(ss.str(), color_bonus);
		}
		if (pwr.trait_elemental > -1) {
			ss.str("");
			ss << msg->get("%s Elemental Damage", ELEMENTS[pwr.trait_elemental].name.c_str());
			tip->addColoredText(ss.str(), color_bonus);
		}
	}

	std::set<std::string>::iterator it;
	for (it = powers->powers[power_cells[slot_num].id].requires_flags.begin(); it != powers->powers[power_cells[slot_num].id].requires_flags.end(); ++it) {
		for (size_t i=0; i<EQUIP_FLAGS.size(); ++i) {
			if ((*it) == EQUIP_FLAGS[i].id) {
				tip->addText(msg->get("Requires a %s", msg->get(EQUIP_FLAGS[i].name)));
			}
		}
	}
	// add requirement
	for (size_t i = 0; i < PRIMARY_STATS.size(); ++i) {
		if (power_cells[slot_num].requires_primary[i] > 0) {
			if (stats->get_primary(i) < power_cells[slot_num].requires_primary[i])
				tip->addColoredText(msg->get("Requires %s %d", power_cells[slot_num].requires_primary[i], PRIMARY_STATS[i].name.c_str()), color_penalty);
			else
				tip->addText(msg->get("Requires %s %d", power_cells[slot_num].requires_primary[i], PRIMARY_STATS[i].name.c_str()));
		}
	}

	// Draw required Level Tooltip
	if ((power_cells[slot_num].requires_generation > 0) && stats->generation < power_cells[slot_num].requires_generation) {
		tip->addColoredText(msg->get("Requires Generation %d", power_cells[slot_num].requires_generation), color_penalty);
	}
	else if ((power_cells[slot_num].requires_generation > 0) && stats->generation >= power_cells[slot_num].requires_generation) {
		tip->addText(msg->get("Requires Generation %d", power_cells[slot_num].requires_generation));
	}

	for (size_t j=0; j < power_cells[slot_num].requires_power.size(); ++j) {
		if (power_cells[slot_num].requires_power[j] == 0) continue;

		int req_index = getCellByPowerIndex(power_cells[slot_num].requires_power[j], power_cell_all);
		if (req_index == -1) continue;

		std::string req_power_name;
		if (power_cell_all[req_index].upgrade_level > 0)
			req_power_name = powers->powers[power_cell_all[req_index].id].name + " (" + msg->get("Level %d", power_cell_all[req_index].upgrade_level) + ")";
		else
			req_power_name = powers->powers[power_cell_all[req_index].id].name;


		// Required Power Tooltip
		int req_cell_index = getCellByPowerIndex(power_cells[slot_num].requires_power[j], power_cell_all);
		if (!checkUnlocked(req_cell_index)) {
			tip->addColoredText(msg->get("Requires Power: %s", req_power_name), color_penalty);
		}
		else {
			tip->addText(msg->get("Requires Power: %s", req_power_name));
		}

	}

	// Draw unlock power Tooltip
	if (power_cells[slot_num].requires_point && !(std::find(stats->powers_list.begin(), stats->powers_list.end(), power_cells[slot_num].id) != stats->powers_list.end())) {
		int unlock_id = getCellByPowerIndex(power_cells[slot_num].id, power_cell_all);
		if (show_unlock_prompt && points_left > 0 && checkUnlock(unlock_id)) {
			tip->addColoredText(msg->get("Click to Unlock (uses 1 Skill Point)"), color_bonus);
		}
		else {
			if (power_cells[slot_num].requires_point && points_left < 1)
				tip->addColoredText(msg->get("Requires 1 Skill Point"), color_penalty);
			else
				tip->addText(msg->get("Requires 1 Skill Point"));
		}
	}
}

void MenuPowers::renderPowers(int tab_num) {

	Rect disabled_src;
	disabled_src.x = disabled_src.y = 0;
	disabled_src.w = disabled_src.h = ICON_SIZE;

	for (size_t i=0; i<power_cell.size(); i++) {
		bool power_in_vector = false;

		// Continue if slot is not filled with data
		if (power_cell[i].tab != tab_num) continue;

		int cell_index = getCellByPowerIndex(power_cell[i].id, power_cell_all);
		if (!checkCellVisible(cell_index)) continue;

		if (std::find(stats->powers_list.begin(), stats->powers_list.end(), power_cell[i].id) != stats->powers_list.end()) power_in_vector = true;

		if (slots[i])
			slots[i]->render();

		// highlighting
		if (power_in_vector || checkUnlocked(cell_index)) {
			Rect src_unlock;

			src_unlock.x = 0;
			src_unlock.y = 0;
			src_unlock.w = ICON_SIZE;
			src_unlock.h = ICON_SIZE;

			int selected_slot = -1;
			if (isTabListSelected()) {
				selected_slot = getSelectedCellIndex();
			}

			for (size_t j=0; j<power_cell.size(); j++) {
				if (selected_slot == static_cast<int>(j))
					continue;

				if (power_cell[j].id == power_cell[i].id && powers_unlock && slots[j]) {
					powers_unlock->setClip(src_unlock);
					powers_unlock->setDest(slots[j]->pos);
					render_device->render(powers_unlock);
				}
			}
		}
		else {
			if (overlay_disabled && slots[i]) {
				overlay_disabled->setClip(disabled_src);
				overlay_disabled->setDest(slots[i]->pos);
				render_device->render(overlay_disabled);
			}
		}

		if (slots[i])
			slots[i]->renderSelection();

		// upgrade buttons
		if (upgradeButtons[i])
			upgradeButtons[i]->render();
	}
}

void MenuPowers::logic() {
	for (size_t i=0; i<power_cell_unlocked.size(); i++) {
		if (static_cast<size_t>(power_cell_unlocked[i].id) < powers->powers.size() && powers->powers[power_cell_unlocked[i].id].passive) {
			bool unlocked_power = std::find(stats->powers_list.begin(), stats->powers_list.end(), power_cell_unlocked[i].id) != stats->powers_list.end();
			std::vector<int>::iterator it = std::find(stats->powers_passive.begin(), stats->powers_passive.end(), power_cell_unlocked[i].id);

			int cell_index = getCellByPowerIndex(power_cell_unlocked[i].id, power_cell_all);
			if (it != stats->powers_passive.end()) {
				if (!checkRequirements(cell_index) && power_cell_unlocked[i].passive_on) {
					stats->powers_passive.erase(it);
					stats->effects.removeEffectPassive(power_cell_unlocked[i].id);
					power_cell[i].passive_on = false;
					stats->refresh_stats = true;
				}
			}
			else if (((checkRequirements(cell_index) && !power_cell_unlocked[i].requires_point) || unlocked_power) && !power_cell_unlocked[i].passive_on) {
				stats->powers_passive.push_back(power_cell_unlocked[i].id);
				power_cell_unlocked[i].passive_on = true;
				// for passives without special triggers, we need to trigger them here
				if (stats->effects.triggered_others)
					powers->activateSinglePassive(stats, power_cell_unlocked[i].id);
			}
		}
	}

	for (size_t i=0; i<power_cell.size(); i++) {
		//upgrade buttons logic
		if (upgradeButtons[i] != NULL) {
			upgradeButtons[i]->enabled = false;
			// enable button only if current level is unlocked and next level can be unlocked
			if (checkUpgrade(static_cast<int>(i))) {
				upgradeButtons[i]->enabled = true;
			}
			if ((!tab_control || power_cell[i].tab == tab_control->getActiveTab()) && upgradeButtons[i]->checkClick()) {
				upgradePower(static_cast<int>(i));
			}
		}
	}

	points_left = (stats->generation * stats->power_points_per_generation) - getPointsUsed();
	if (points_left > 0) {
		newPowerNotification = true;
	}

	if (!visible) return;

	tablist.logic();
	if (!tabs.empty()) {
		for (size_t i=0; i<tabs.size(); i++) {
			if (tab_control && tab_control->getActiveTab() == static_cast<int>(i)) {
				tablist.setNextTabList(&tablist_pow[i]);
			}
			tablist_pow[i].logic();
		}
	}

	if (closeButton->checkClick()) {
		visible = false;
		snd->play(sfx_close);
	}

	if (tab_control) {
		// make shure keyboard navigation leads us to correct tab
		for (size_t i=0; i<slots.size(); i++) {
			if (power_cell[i].tab == tab_control->getActiveTab())
				continue;

			if (slots[i] && slots[i]->in_focus)
				slots[i]->defocus();
		}

		tab_control->logic();
	}
}

void MenuPowers::render() {
	if (!visible) return;

	Rect src;
	Rect dest;

	// background
	dest = window_area;
	src.x = 0;
	src.y = 0;
	src.w = window_area.w;
	src.h = window_area.h;

	setBackgroundClip(src);
	setBackgroundDest(dest);
	Menu::render();


	if (tab_control) {
		tab_control->render();
		int active_tab = tab_control->getActiveTab();
		for (size_t i=0; i<tabs.size(); i++) {
			if (active_tab == static_cast<int>(i)) {
				// power tree
				Sprite *r = tree_surf[i];
				if (r) {
					r->setClip(src);
					r->setDest(dest);
					render_device->render(r);
				}

				// power icons
				renderPowers(active_tab);
			}
		}
	}
	else if (!tree_surf.empty()) {
		Sprite *r = tree_surf[0];
		if (r) {
			r->setClip(src);
			r->setDest(dest);
			render_device->render(r);
		}
		renderPowers(0);
	}
	else {
		renderPowers(0);
	}

	// close button
	closeButton->render();

	// text overlay
	if (!title.hidden) label_powers.render();

	// stats
	if (!unspent_points.hidden) {
		std::stringstream ss;

		ss.str("");
		if (points_left !=0) {
			ss << msg->get("Unspent skill points:") << " " << points_left;
		}
		stat_up.set(ss.str());
		stat_up.render();
	}
}

/**
 * Show mouseover descriptions of disciplines and powers
 */
TooltipData MenuPowers::checkTooltip(const Point& mouse) {

	TooltipData tip;

	for (size_t i=0; i<power_cell.size(); i++) {

		if (tab_control && (tab_control->getActiveTab() != power_cell[i].tab)) continue;

		int cell_index = getCellByPowerIndex(power_cell[i].id, power_cell_all);
		if (!checkCellVisible(cell_index)) continue;

		if (slots[i] && isWithinRect(slots[i]->pos, mouse)) {
			bool base_unlocked = checkUnlocked(cell_index) || std::find(stats->powers_list.begin(), stats->powers_list.end(), power_cell[i].id) != stats->powers_list.end();

			createTooltip(&tip, static_cast<int>(i), power_cell, !base_unlocked);
			if (!power_cell[i].upgrades.empty()) {
				int next_level = getNextLevelCell(static_cast<int>(i));
				if (next_level != -1) {
					tip.addText("\n" + msg->get("Next Level:"));
					createTooltip(&tip, next_level, power_cell_upgrade, base_unlocked);
				}
			}

			return tip;
		}
	}

	return tip;
}

/**
 * Click-to-drag a power (to the action bar)
 */
int MenuPowers::click(const Point& mouse) {
	int active_tab = (tab_control) ? tab_control->getActiveTab() : 0;

	for (size_t i=0; i<power_cell.size(); i++) {
		if (slots[i] && isWithinRect(slots[i]->pos, mouse) && (power_cell[i].tab == active_tab)) {
			if (TOUCHSCREEN) {
				if (!slots[i]->in_focus) {
					slots[i]->in_focus = true;
					if (!tabs.empty()) {
						tablist_pow[active_tab].setCurrent(slots[i]);
					}
					else {
						tablist.setCurrent(slots[i]);
					}
					return 0;
				}
			}

			int cell_index = getCellByPowerIndex(power_cell[i].id, power_cell_all);
			if (checkUnlock(cell_index) && points_left > 0 && power_cell[i].requires_point) {
				// unlock power
				stats->powers_list.push_back(power_cell[i].id);
				stats->check_title = true;
				setUnlockedPowers();
				action_bar->addPower(power_cell[i].id, 0);
				return 0;
			}
			else if (checkUnlocked(cell_index) && !powers->powers[power_cell[i].id].passive) {
				// pick up and drag power
				slots[i]->defocus();
				if (!tabs.empty()) {
					tablist_pow[active_tab].setCurrent(NULL);
				}
				else {
					tablist.setCurrent(NULL);
				}
				return power_cell[i].id;
			}
			else
				return 0;
		}
	}

	// nothing selected, defocus everything
	defocusTabLists();

	return 0;
}

void MenuPowers::upgradeByCell(int pci) {
	if (checkUpgrade(pci))
		upgradePower(pci);
}

/**
 * Apply power upgrades on savegame loading
 */
void MenuPowers::applyPowerUpgrades() {
	for (size_t i=0; i<power_cell.size(); i++) {
		if (!power_cell[i].upgrades.empty()) {
			std::vector<int>::iterator it;
			for (it = power_cell[i].upgrades.end(); it != power_cell[i].upgrades.begin(); ) {
				--it;
				std::vector<int>::iterator upgrade_it;
				upgrade_it = std::find(stats->powers_list.begin(), stats->powers_list.end(), *it);
				if (upgrade_it != stats->powers_list.end()) {
					int upgrade_index = getCellByPowerIndex((*upgrade_it), power_cell_upgrade);
					if (upgrade_index != -1)
						replaceCellWithUpgrade(static_cast<int>(i), upgrade_index);
					break;
				}
			}
		}
	}
	setUnlockedPowers();
}

void MenuPowers::resetToBasePowers() {
	for (size_t i=0; i<power_cell.size(); ++i) {
		power_cell[i] = power_cell_base[i];
	}
}

/**
 * Return true if required stats for power usage are met. Else return false.
 */
bool MenuPowers::meetsUsageStats(int power_index) {
	// Find cell with our power
	int id = getCellByPowerIndex(power_index, power_cell);

	// If we didn't find power in power_menu, than it has no stats requirements
	if (id == -1) return true;

	for (size_t i = 0; i < PRIMARY_STATS.size(); ++i) {
		if (stats->get_primary(i) < power_cell[id].requires_primary[i])
			return false;
	}

	return true;
}

bool MenuPowers::isTabListSelected() {
	return (getCurrentTabList() && (tabs.empty() || (tabs.size() > 0 && getCurrentTabList() != (&tablist))));
}

int MenuPowers::getSelectedCellIndex() {
	int current = getCurrentTabList()->getCurrent();

	if (tabs.empty()) {
		return current;
	}
	else {
		int active_tab = tab_control->getActiveTab();
		int index_offset = 0;

		for (int i=0; i<active_tab; ++i) {
			index_offset += static_cast<int>(tablist_pow[active_tab].size());
		}

		index_offset += current;

		return index_offset;
	}
}

void MenuPowers::setNextTabList(TabList *tl) {
	if (!tabs.empty()) {
		for (size_t i=0; i<tabs.size(); ++i) {
			tablist_pow[i].setNextTabList(tl);
		}
	}
}

TabList* MenuPowers::getCurrentTabList() {
	if (tablist.getCurrent() != -1) {
		return (&tablist);
	}
	else if (!tabs.empty()) {
		for (size_t i=0; i<tabs.size(); ++i) {
			if (tablist_pow[i].getCurrent() != -1)
				return (&tablist_pow[i]);
		}
	}

	return NULL;
}

void MenuPowers::defocusTabLists() {
	tablist.defocus();

	if (!tabs.empty()) {
		for (size_t i=0; i<tabs.size(); ++i) {
			tablist_pow[i].defocus();
		}
	}
}

