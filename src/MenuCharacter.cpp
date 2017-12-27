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
 * class MenuCharacter
 */

#include <string>

#include "FileParser.h"
#include "Menu.h"
#include "MenuCharacter.h"
#include "SharedResources.h"
#include "Settings.h"
#include "StatBlock.h"
#include "UtilsParsing.h"
#include "WidgetButton.h"
#include "WidgetListBox.h"

MenuCharacter::MenuCharacter(StatBlock *_stats) {
	stats = _stats;

	// 2 is added here to account for CSTAT_NAME and CSTAT_GENERATION
	cstat.resize(PRIMARY_STATS.size() + 2);
	
	// Labels for major stats
	cstat[CSTAT_NAME].label_text = msg->get("Name");
	cstat[CSTAT_GENERATION].label_text = msg->get("Generation");
	for (size_t i = 0; i < PRIMARY_STATS.size(); ++i) {
		cstat[i+2].label_text = PRIMARY_STATS[i].name;
	}

	skill_points = 0;

	visible = false;

	for (size_t i = 0; i < cstat.size(); ++i) {
		cstat[i].label = new WidgetLabel();
		cstat[i].value = new WidgetLabel();
		cstat[i].hover.x = cstat[i].hover.y = 0;
		cstat[i].hover.w = cstat[i].hover.h = 0;
		cstat[i].visible = true;
	}
	show_stat.resize(STAT_COUNT + DAMAGE_TYPES_COUNT);
	for (size_t i = 0; i < STAT_COUNT + DAMAGE_TYPES_COUNT; i++) {
		show_stat[i] = true;
	}

	// these two are hidden by default, as they are currently unused
	show_stat[STAT_HULL_PERCENT] = false;
	show_stat[STAT_PSI_PERCENT] = false;
	show_stat[STAT_CAPACITOR_PERCENT] = false;
	show_stat[STAT_SHIELD_PERCENT] = false;

	show_resists = true;

	statlist_rows = 10;
	statlist_scrollbar_offset = 0;

	closeButton = new WidgetButton("images/menus/buttons/button_x.png");

	// Upgrade buttons
	primary_up.resize(PRIMARY_STATS.size());
	upgradeButton.resize(PRIMARY_STATS.size());

	for (size_t i = 0; i < PRIMARY_STATS.size(); ++i) {
		primary_up[i] = false;
		upgradeButton[i] = new WidgetButton("images/menus/buttons/upgrade.png");
		upgradeButton[i]->enabled = false;
	}

	// menu title
	labelCharacter = new WidgetLabel();

	// unspent points
	labelUnspent = new WidgetLabel();

	// Load config settings
	FileParser infile;
	// @CLASS MenuCharacter|Description of menus/character.txt
	if (infile.open("menus/character.txt")) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;

			// @ATTR close|point|Position of the close button.
			if(infile.key == "close") {
				Point pos = toPoint(infile.val);
				closeButton->setBasePos(pos.x, pos.y);
			}
			// @ATTR label_title|label|Position of the "Character" text.
			else if(infile.key == "label_title") title = eatLabelInfo(infile.val);
			// @ATTR upgrade_primary|predefined_string, point : Primary stat name, Button position|Position of the button used to add a stat point to this primary stat.
			else if(infile.key == "upgrade_primary") {
				std::string prim_stat = popFirstString(infile.val);
				size_t prim_stat_index = getPrimaryStatIndex(prim_stat);

				if (prim_stat_index != PRIMARY_STATS.size()) {
					Point pos = toPoint(infile.val);
					upgradeButton[prim_stat_index]->setBasePos(pos.x, pos.y);
				}
				else {
					infile.error("MenuCharacter: '%s' is not a valid primary stat.", prim_stat.c_str());
				}
			}
			// @ATTR statlist|point|Position of the scrollbox containing non-primary stats.
			else if(infile.key == "statlist") statlist_pos = toPoint(infile.val);
			// @ATTR statlist_rows|int|The height of the statlist in rows.
			else if (infile.key == "statlist_rows") statlist_rows = toInt(infile.val);
			// @ATTR statlist_scrollbar_offset|int|Right margin in pixels for the statlist's scrollbar.
			else if (infile.key == "statlist_scrollbar_offset") statlist_scrollbar_offset = toInt(infile.val);

			// @ATTR label_name|label|Position of the "Name" text.
			else if(infile.key == "label_name") {
				cstat[CSTAT_NAME].label_info = eatLabelInfo(infile.val);
				cstat[CSTAT_NAME].visible = !cstat[CSTAT_NAME].label_info.hidden;
			}
			// @ATTR label_generation|label|Position of the "Generation" text.
			else if(infile.key == "label_generation") {
				cstat[CSTAT_GENERATION].label_info = eatLabelInfo(infile.val);
				cstat[CSTAT_GENERATION].visible = !cstat[CSTAT_GENERATION].label_info.hidden;
			}
			// @ATTR label_primary|predefined_string, label : Primary stat name, Text positioning|Position of the text label for this primary stat.
			else if(infile.key == "label_primary") {
				std::string prim_stat = popFirstString(infile.val);
				size_t prim_stat_index = getPrimaryStatIndex(prim_stat);

				if (prim_stat_index != PRIMARY_STATS.size()) {
					cstat[prim_stat_index+2].label_info = eatLabelInfo(infile.val);
					cstat[prim_stat_index+2].visible = !cstat[prim_stat_index+2].label_info.hidden;
				}
				else {
					infile.error("MenuCharacter: '%s' is not a valid primary stat.", prim_stat.c_str());
				}
			}

			// @ATTR name|rectangle|Position of the player's name and dimensions of the tooltip hotspot.
			else if(infile.key == "name") {
				cstat[CSTAT_NAME].value_pos = toRect(infile.val);
				cstat[CSTAT_NAME].value->setBasePos(cstat[CSTAT_NAME].value_pos.x + 4, cstat[CSTAT_NAME].value_pos.y + (cstat[CSTAT_NAME].value_pos.h/2)); // TODO remove 4 from x value
			}
			// @ATTR generation|rectangle|Position of the player's generation and dimensions of the tooltip hotspot.
			else if(infile.key == "generation") {
				cstat[CSTAT_GENERATION].value_pos = toRect(infile.val);
				cstat[CSTAT_GENERATION].value->setBasePos(cstat[CSTAT_GENERATION].value_pos.x + (cstat[CSTAT_GENERATION].value_pos.w/2), cstat[CSTAT_GENERATION].value_pos.y + (cstat[CSTAT_GENERATION].value_pos.h/2));
			}
			// @ATTR primary|predefined_string, rectangle : Primary stat name, Hotspot position|Position of this primary stat value display and dimensions of its tooltip hotspot.
			else if(infile.key == "primary") {
				std::string prim_stat = popFirstString(infile.val);
				size_t prim_stat_index = getPrimaryStatIndex(prim_stat);

				if (prim_stat_index != PRIMARY_STATS.size()) {
					Rect r = toRect(infile.val);
					cstat[prim_stat_index+2].value_pos = r;
					cstat[prim_stat_index+2].value->setBasePos(r.x + (r.w/2), r.y + (r.h/2));
				}
				else {
					infile.error("MenuCharacter: '%s' is not a valid primary stat.", prim_stat.c_str());
				}
			}

			// @ATTR unspent|label|Position of the label showing the number of unspent stat points.
			else if(infile.key == "unspent") unspent_pos = eatLabelInfo(infile.val);

			// @ATTR show_resists|bool|Hide the elemental "Resistance" stats in the statlist if set to false.
			else if (infile.key == "show_resists") show_resists = toBool(infile.val);

			// @ATTR show_stat|string, bool : Stat name, Visible|Hide the matching stat in the statlist if set to false.
			else if (infile.key == "show_stat") {
				std::string stat_name = popFirstString(infile.val);

				for (unsigned i=0; i<STAT_COUNT; ++i) {
					if (stat_name == STAT_KEY[i]) {
						show_stat[i] = toBool(popFirstString(infile.val));
						break;
					}
				}
				for (size_t i = 0; i < DAMAGE_TYPES.size(); ++i) {
					if (stat_name == DAMAGE_TYPES[i].min) {
						show_stat[STAT_COUNT + (i*2)] = toBool(popFirstString(infile.val));
					}
					else if (stat_name == DAMAGE_TYPES[i].max) {
						show_stat[STAT_COUNT + (i*2) + 1] = toBool(popFirstString(infile.val));
					}
				}
			}

			else {
				infile.error("MenuCharacter: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}

	// stat list
	statList = new WidgetListBox(statlist_rows, "images/menus/buttons/listbox_char.png");
	tablist.add(statList);
	statList->can_select = false;
	statList->scrollbar_offset = statlist_scrollbar_offset;
	statList->setBasePos(statlist_pos.x, statlist_pos.y);

	// HACK: During gameplay, the stat list can refresh rapidly when the charcter menu is open and the player has certain effects
	// frequently refreshing trimmed text is slow for Cyrillic characters, so disable it here
	statList->disable_text_trim = true;

	setBackground("images/menus/character.png");

	align();

	base_stats.resize(PRIMARY_STATS.size());
	base_stats_add.resize(PRIMARY_STATS.size());
	base_bonus.resize(PRIMARY_STATS.size());

	for (size_t i = 0; i < PRIMARY_STATS.size(); ++i) {
		base_stats[i] = &stats->primary[i];
		base_stats_add[i] = &stats->primary_additional[i];
		base_bonus[i] = &stats->per_primary[i];
	}
}

void MenuCharacter::align() {
	Menu::align();

	// close button
	closeButton->setPos(window_area.x, window_area.y);
	
	// menu title
	labelCharacter->set(window_area.x+title.x, window_area.y+title.y, title.justify, title.valign, msg->get("CHARACTER"), font->getColor("menu_header"), title.font_style);

	// upgrade buttons
	for (size_t i = 0; i < PRIMARY_STATS.size(); ++i) {
		upgradeButton[i]->setPos(window_area.x, window_area.y);
	}

	// stat list
	statList->setPos(window_area.x, window_area.y);

	for (size_t i = 0; i < cstat.size(); ++i) {
		// setup static labels
		cstat[i].label->set(window_area.x + cstat[i].label_info.x,
							window_area.y + cstat[i].label_info.y,
							cstat[i].label_info.justify,
							cstat[i].label_info.valign,
							cstat[i].label_text,
							font->getColor("menu_normal"),
							cstat[i].label_info.font_style);

		// setup hotspot locations
		cstat[i].setHover(window_area.x + cstat[i].value_pos.x, window_area.y + cstat[i].value_pos.y, cstat[i].value_pos.w, cstat[i].value_pos.h);

		// setup value labels
		cstat[i].value->setPos(window_area.x, window_area.y);
	}

	labelUnspent->setX(window_area.x + unspent_pos.x);
	labelUnspent->setY(window_area.y + unspent_pos.y);
}

/**
 * Rebuild all stat values and tooltip info
 */
void MenuCharacter::refreshStats() {

	stats->refresh_stats = false;

	std::stringstream ss;

	// update stat text
	cstat[CSTAT_NAME].value->set(cstat[CSTAT_NAME].value->pos.x, cstat[CSTAT_NAME].value->pos.y, JUSTIFY_LEFT, VALIGN_CENTER, stats->name, font->getColor("menu_normal"));

	ss.str("");
	ss << stats->generation;
	cstat[CSTAT_GENERATION].value->set(cstat[CSTAT_GENERATION].value->pos.x, cstat[CSTAT_GENERATION].value->pos.y, JUSTIFY_CENTER, VALIGN_CENTER, ss.str(), font->getColor("menu_normal"));

	for (size_t i = 0; i < PRIMARY_STATS.size(); ++i) {
		ss.str("");
		ss << stats->get_primary(i);
		cstat[i+2].value->set(cstat[i+2].value->pos.x, cstat[i+2].value->pos.y, JUSTIFY_CENTER, VALIGN_CENTER, ss.str(), bonusColor(stats->primary_additional[i]));
	}
	ss.str("");
	if (skill_points > 0) ss << skill_points << " " << msg->get("points remaining");
	else ss.str("");
	labelUnspent->set(window_area.x+unspent_pos.x, window_area.y+unspent_pos.y, unspent_pos.justify, unspent_pos.valign, ss.str(), font->getColor("menu_bonus"), unspent_pos.font_style);

	// scrolling stat list
	unsigned stat_index = 0;
	for (unsigned i=0; i<STAT_COUNT; ++i) {
		if (!show_stat[i]) continue;

		// insert damage stats before absorb min
		if (i == STAT_ABS_MIN) {
			for (size_t j = 0; j < DAMAGE_TYPES.size(); ++j) {
				if (show_stat[STAT_COUNT + (j*2)]) {
					// min
					ss.str("");
					ss << DAMAGE_TYPES[j].text_min << ": " << stats->getDamageMin(j);
					statList->set(stat_index, ss.str(), damageTooltip(j*2));
					stat_index++;
				}

				if (show_stat[STAT_COUNT + (j*2) + 1]) {
					// max
					ss.str("");
					ss << DAMAGE_TYPES[j].text_max << ": " << stats->getDamageMax(j);
					statList->set(stat_index, ss.str(), damageTooltip((j*2) + 1));
					stat_index++;
				}
			}
		}
		ss.str("");
		ss << STAT_NAME[i] << ": " << stats->get((STAT)i);

		if (STAT_PERCENT[i]) ss << "%";

		std::string stat_tooltip = statTooltip(i);
		std::string full_tooltip = "";
		if (STAT_DESC[i] != "")
			full_tooltip += STAT_DESC[i];
		if (full_tooltip != "" && stat_tooltip != "")
			full_tooltip += "\n";
		full_tooltip += stat_tooltip;
		
		statList->set(stat_index, ss.str(), full_tooltip);
		stat_index++;
	}
	for (unsigned i=0; i<TALENTS.size(); ++i) {
		ss.str("");
		ss << TALENTS[i].name << ": " << stats->talent[i];
		statList->set(stat_index, ss.str(), "");
		stat_index++;
	}
	if (show_resists) {
		for (unsigned int j=0; j<stats->vulnerable.size(); ++j) {
			ss.str("");
			ss << msg->get("%s resistance", ELEMENTS[j].name.c_str()) << ": " << (100 - stats->vulnerable[j]) << "%";
			statList->set(j+stat_index-2, ss.str(),"");
		}
	}

	// update tool tips
	cstat[CSTAT_NAME].tip.clear();
	cstat[CSTAT_NAME].tip.addText(stats->getLongClass());

	cstat[CSTAT_GENERATION].tip.clear();
	cstat[CSTAT_GENERATION].tip.addText(msg->get("Soul: %d", stats->soul));
	if (static_cast<unsigned>(stats->generation) < stats->soul_table.size()) {
		cstat[CSTAT_GENERATION].tip.addText(msg->get("Next: %d", stats->soul_table[stats->generation]));
	}

	for (size_t j = 2; j < cstat.size(); ++j) {
		cstat[j].tip.clear();
		cstat[j].tip.addText(cstat[j].label_text);
		cstat[j].tip.addText(msg->get("base (%d), bonus (%d)", *(base_stats[j-2]), *(base_stats_add[j-2])));
		bool have_bonus = false;
		for (unsigned i=0; i<STAT_COUNT; ++i) {
			if (base_bonus[j-2]->at(i) > 0) {
				if (!have_bonus) {
					cstat[j].tip.addText("\n" + msg->get("Related stats:"));
					have_bonus = true;
				}
				cstat[j].tip.addText(STAT_NAME[i]);
			}
		}
	}
}


/**
 * Color-coding for positive/negative/no bonus
 */
Color MenuCharacter::bonusColor(int stat) {
	if (stat > 0) return font->getColor("menu_bonus");
	if (stat < 0) return font->getColor("menu_penalty");
	return font->getColor("menu_label");
}

/**
 * Create tooltip text showing the per_* values of a stat
 */
std::string MenuCharacter::statTooltip(int stat) {
	std::string tooltip_text;

	if (stats->per_generation[stat] > 0)
		tooltip_text += msg->get("Each generation grants %d. ", stats->per_generation[stat]);
		
	for (size_t i = 0; i < PRIMARY_STATS.size(); ++i) {
		if (stats->per_primary[i][stat] > 0)
			tooltip_text += msg->get("Each point of %s grants %d. ", stats->per_primary[i][stat], PRIMARY_STATS[i].name.c_str());
	}

	return tooltip_text;
}


/**
 * Create tooltip text showing the per_* values of a damage stat
 */
std::string MenuCharacter::damageTooltip(size_t dmg_type) {
	std::string tooltip_text;

	if (stats->per_generation[STAT_COUNT + dmg_type] > 0)
		tooltip_text += msg->get("Each level grants %d. ", stats->per_generation[STAT_COUNT + dmg_type]);

	for (size_t i = 0; i < PRIMARY_STATS.size(); ++i) {
		if (stats->per_primary[i][STAT_COUNT + dmg_type] > 0)
			tooltip_text += msg->get("Each point of %s grants %d. ", stats->per_primary[i][STAT_COUNT + dmg_type], PRIMARY_STATS[i].name.c_str());
	}

	size_t real_dmg_type = dmg_type / 2;

	std::string full_tooltip = "";
	if (DAMAGE_TYPES[real_dmg_type].description != "")
		full_tooltip += DAMAGE_TYPES[real_dmg_type].description;
	if (full_tooltip != "" && tooltip_text != "")
		full_tooltip += "\n";
	full_tooltip += tooltip_text;

	return full_tooltip;
}

void MenuCharacter::logic() {
	if (!visible) return;

	tablist.logic();

	if (closeButton->checkClick()) {
		visible = false;
		snd->play(sfx_close);
	}

	int spent = 0;
	for (size_t i = 0; i < PRIMARY_STATS.size(); ++i) {
		spent += stats->primary[i];
	}
	// players start with 1 point in each stat, so we nullify that here
	spent -= static_cast<int>(PRIMARY_STATS.size());

	skill_points = (stats->generation * stats->stat_points_per_generation) - spent;
	
	if (skill_points == 0) {
		// upgrade buttons
		for (size_t i = 0; i < PRIMARY_STATS.size(); ++i) {
			upgradeButton[i]->enabled = false;
			tablist.remove(upgradeButton[i]);
		}

		if (tablist.getCurrent() >= static_cast<int>(tablist.size())) {
			tablist.defocus();
			tablist.getNext();
		}
	}

	if (stats->hull > 0 && spent < (stats->generation * stats->stat_points_per_generation) && spent < stats->max_spendable_stat_points) {
		for (size_t i = 0; i < PRIMARY_STATS.size(); ++i) {
			if (stats->primary[i] < stats->max_points_per_stat) {
				upgradeButton[i]->enabled = true;
				tablist.add(upgradeButton[i]);
			}
		}
	}

	for (size_t i = 0; i < PRIMARY_STATS.size(); ++i) {
		if (upgradeButton[i]->checkClick())
			primary_up[i] = true;
	}

	statList->checkClick();

	if (stats->refresh_stats) refreshStats();
}



void MenuCharacter::render() {
	if (!visible) return;

	// background
	Menu::render();

	// close button
	closeButton->render();

	// title
	labelCharacter->render();

	// unspent points
	labelUnspent->render();

	// labels and values
	for (size_t i = 0; i < cstat.size(); ++i) {
		if (cstat[i].visible) {
			cstat[i].label->render();
			cstat[i].value->render();
		}
	}

	// upgrade buttons
	for (size_t i = 0; i < PRIMARY_STATS.size(); ++i) {
		if (upgradeButton[i]->enabled) upgradeButton[i]->render();
	}

	statList->render();
}


/**
 * Display various mouseovers tooltips depending on cursor location
 */
TooltipData MenuCharacter::checkTooltip() {

	for (size_t i = 0; i < cstat.size(); ++i) {
		if (isWithinRect(cstat[i].hover, inpt->mouse) && !cstat[i].tip.isEmpty() && cstat[i].visible)
			return cstat[i].tip;
	}

	return statList->checkTooltip(inpt->mouse);
}

/**
 * User might click this menu to upgrade a stat.  Check for this situation.
 * Return true if a stat was upgraded.
 */
bool MenuCharacter::checkUpgrade() {
	int spent = 0;
	for (size_t i = 0; i < PRIMARY_STATS.size(); ++i) {
		spent += stats->primary[i];
	}
	// players start with 1 point in each stat, so we nullify that here
	spent -= static_cast<int>(PRIMARY_STATS.size());

	skill_points = (stats->generation * stats->stat_points_per_generation) - spent;

	// check to see if there are skill points available
	if (spent < (stats->generation * stats->stat_points_per_generation) && spent < stats->max_spendable_stat_points) {

		for (size_t i = 0; i < PRIMARY_STATS.size(); ++i) {
			if (primary_up[i]) {
				stats->primary[i]++;
				snd->play(sfx_attributeup);
				stats->recalc(); // equipment applied by MenuManager
				primary_up[i] = false;
				return true;
			}
		}
	}

	return false;
}

MenuCharacter::~MenuCharacter() {
	delete closeButton;
	delete labelCharacter;
	delete labelUnspent;
	for (size_t i = 0; i < cstat.size(); ++i) {
		delete cstat[i].label;
		delete cstat[i].value;
	}
	for (size_t i = 0; i < upgradeButton.size(); ++i) {
		delete upgradeButton[i];
	}
	delete statList;
}
