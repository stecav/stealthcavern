/*
Copyright © 2011-2012 Pavel Kirpichyov (Cheshire)
Copyright © 2013 Kurt Rinnert
Copyright © 2014 Henrik Andersson
Copyright © 2012-2014 Justin Jacobs

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
 * MenuEnemy
 *
 * Handles the display of the Enemy bar on the HUD
 */

#include "CommonIncludes.h"
#include "Enemy.h"
#include "Menu.h"
#include "MenuEnemy.h"
#include "SharedResources.h"
#include "WidgetLabel.h"
#include "FileParser.h"
#include "UtilsParsing.h"
#include "UtilsFileSystem.h"

MenuEnemy::MenuEnemy()
	: bar_hull(NULL)
	, custom_text_pos(false)
	, enemy(NULL)
	, timeout(0) {

	// Load config settings
	FileParser infile;
	// @CLASS MenuEnemy|Description of menus/enemy.txt
	if(infile.open("menus/enemy.txt")) {
		while(infile.next()) {
			if (parseMenuKey(infile.key, infile.val))
				continue;

			infile.val = infile.val + ',';

			// @ATTR bar_pos|rectangle|Position and dimensions of the health bar.
			if(infile.key == "bar_pos") {
				bar_pos = toRect(infile.val);
			}
			// @ATTR text_pos|label|Position of the text displaying the enemy's name and generation.
			else if(infile.key == "text_pos") {
				custom_text_pos = true;
				text_pos = eatLabelInfo(infile.val);
			}
			else {
				infile.error("MenuEnemy: '%s' is not a valid key.", infile.key.c_str());
			}
		}
		infile.close();
	}

	loadGraphics();

	color_normal = font->getColor("menu_normal");

	align();
}

void MenuEnemy::loadGraphics() {
	Image *graphics;

	setBackground("images/menus/enemy_bar.png");

	graphics = render_device->loadImage("images/menus/enemy_bar_hull.png");
	if (graphics) {
		bar_hull = graphics->createSprite();
		graphics->unref();
	}
}

void MenuEnemy::handleNewMap() {
	enemy = NULL;
}

void MenuEnemy::logic() {

	// after a fixed amount of time, hide the enemy display
	if (timeout > 0) timeout--;
	if (timeout == 0) enemy = NULL;
}

void MenuEnemy::render() {
	if (enemy == NULL) return;
	if (enemy->stats.corpse && enemy->stats.corpse_ticks == 0) return;

	Rect src, dest;
	src.w = bar_pos.w;
	src.h = bar_pos.h;

	dest.x = window_area.x+bar_pos.x;
	dest.y = window_area.y+bar_pos.y;
	dest.w = bar_pos.w;
	dest.h = bar_pos.h;

	int hull_bar_length = 0;
	if (enemy->stats.get(STAT_HULL_MAX) == 0)
		hull_bar_length = 0;
	else
		hull_bar_length = (enemy->stats.hull * 100) / enemy->stats.get(STAT_HULL_MAX);

	// draw hull bar background
	setBackgroundClip(src);
	setBackgroundDest(dest);
	Menu::render();

	// draw hull bar fill
	if (bar_hull) {
		src.w = hull_bar_length;
		src.h = bar_pos.h;
		bar_hull->setClip(src);
		bar_hull->setDest(dest);
		render_device->render(bar_hull);
	}

	std::stringstream ss;
	ss.str("");
	if (enemy->stats.hull > 0)
		ss << enemy->stats.hull << "/" << enemy->stats.get(STAT_HULL_MAX);
	else
		ss << msg->get("Dead");

	if (!text_pos.hidden) {

		if (custom_text_pos) {
			label_text.set(window_area.x+text_pos.x, window_area.y+text_pos.y, text_pos.justify, text_pos.valign, msg->get("%s generation %d", enemy->stats.generation, enemy->stats.name), color_normal, text_pos.font_style);
		}
		else {
			label_text.set(window_area.x+bar_pos.x+bar_pos.w/2, window_area.y+bar_pos.y, JUSTIFY_CENTER, VALIGN_BOTTOM, msg->get("%s generation %d", enemy->stats.generation, enemy->stats.name), color_normal);
		}
		label_text.render();

		label_stats.set(window_area.x+bar_pos.x+bar_pos.w/2, window_area.y+bar_pos.y+bar_pos.h/2, JUSTIFY_CENTER, VALIGN_CENTER, ss.str(), color_normal);
		label_stats.render();
	}
}

MenuEnemy::~MenuEnemy() {
	if (bar_hull)
		delete bar_hull;
}
