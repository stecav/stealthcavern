/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2014 Henrik Andersson
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
 * class MenuMiniMap
 */

#ifndef MENU_MINI_MAP_H
#define MENU_MINI_MAP_H

#include "CommonIncludes.h"
#include "Utils.h"
#include "WidgetLabel.h"

class MapCollision;
class WidgetLabel;

class MenuMiniMap : public Menu {
private:
	Color color_wall;
	Color color_obst;
	Color color_hero;

	Sprite *map_surface;
	Point map_size;

	Rect pos;
	LabelInfo text_pos;
	WidgetLabel *label;

	void createMapSurface();
	void renderIso(const FPoint& hero_pos);
	void renderOrtho(const FPoint& hero_pos);
	void prerenderOrtho(MapCollision *collider);
	void prerenderIso(MapCollision *collider);

public:
	MenuMiniMap();
	~MenuMiniMap();
	void align();

	void render();
	void render(const FPoint& hero_pos);
	void prerender(MapCollision *collider, int map_w, int map_h);
	void setMapTitle(const std::string& map_title);
};


#endif
