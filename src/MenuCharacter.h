/*
Copyright © 2011-2012 Clint Bellanger
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
 * class MenuCharacter
 */

#ifndef MENU_CHARACTER_H
#define MENU_CHARACTER_H

#include "CommonIncludes.h"
#include "Stats.h"
#include "TooltipData.h"
#include "WidgetLabel.h"

class StatBlock;
class WidgetButton;
class WidgetLabel;
class WidgetListBox;

const int CSTAT_NAME = 0;
const int CSTAT_GENERATION = 1;

class CharStat {
public:
	WidgetLabel *label;
	WidgetLabel *value;
	Rect hover;
	TooltipData tip;
	bool visible;
	LabelInfo label_info;
	std::string label_text;
	Rect value_pos;

	void setHover(int x, int y, int w, int h) {
		hover.x=x;
		hover.y=y;
		hover.w=w;
		hover.h=h;
	}
};

class MenuCharacter : public Menu {
private:
	StatBlock *stats;

	WidgetButton *closeButton;
	WidgetLabel *labelCharacter;
	WidgetLabel *labelUnspent;
	WidgetListBox *statList;
	std::vector<WidgetButton*> upgradeButton;

	void loadGraphics();
	Color bonusColor(int stat);
	std::string statTooltip(int stat);
	std::string damageTooltip(size_t dmg_type);
	int skill_points;
	std::vector<bool> primary_up;

	// label and widget positions
	LabelInfo title;
	Point statlist_pos;
	int statlist_rows;
	int statlist_scrollbar_offset;
	LabelInfo unspent_pos;
	std::vector<bool> show_stat;
	bool show_resists;

	std::vector<CharStat> cstat;

	std::vector<int*> base_stats;
	std::vector<int*> base_stats_add;
	std::vector< std::vector<int>* > base_bonus;

public:
	explicit MenuCharacter(StatBlock *stats);
	~MenuCharacter();
	void align();

	void logic();
	void render();
	void refreshStats();
	TooltipData checkTooltip();
	bool checkUpgrade();
	int getUnspent() {
		return skill_points;
	}
};

#endif
