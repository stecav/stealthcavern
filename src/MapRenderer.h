/*
Copyright © 2011-2012 Clint Bellanger
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
 * class MapRenderer
 *
 * Map data structure and rendering
 * This class is capable of rendering isometric and orthogonal maps.
 */

#ifndef MAP_RENDERER_H
#define MAP_RENDERER_H

#include "CommonIncludes.h"
#include "GameStatePlay.h"
#include "Map.h"
#include "MapParallax.h"
#include "MapCollision.h"
#include "Settings.h"
#include "TileSet.h"
#include "Utils.h"
#include "StatBlock.h"
#include "TooltipData.h"

class FileParser;
class WidgetTooltip;

class MapRenderer : public Map {
private:

	WidgetTooltip *tip;
	TooltipData tip_buf;
	Point tip_pos;
	bool show_tooltip;

	bool enemyGroupPlaceEnemy(float x, float y, Map_Group &g);
	void pushEnemyGroup(Map_Group &g);

	void clearQueues();

	void drawRenderable(std::vector<Renderable>::iterator r_cursor);

	void renderIsoLayer(const Map_Layer& layerdata);

	// renders only objects
	void renderIsoBackObjects(std::vector<Renderable> &r);

	// renders interleaved objects and layer
	void renderIsoFrontObjects(std::vector<Renderable> &r);
	void renderIso(std::vector<Renderable> &r, std::vector<Renderable> &r_dead);

	void renderOrthoLayer(const Map_Layer& layerdata);
	void renderOrthoBackObjects(std::vector<Renderable> &r);
	void renderOrthoFrontObjects(std::vector<Renderable> &r);
	void renderOrtho(std::vector<Renderable> &r, std::vector<Renderable> &r_dead);

	void clearLayers();

	void createTooltip(Event_Component *ec);

	FPoint shakycam;
	TileSet tset;
	
	MapParallax map_parallax;

public:
	// functions
	MapRenderer();
	~MapRenderer();

	MapRenderer(const MapRenderer &copy); // not implemented

	int load(const std::string& filename);
	void logic();
	void render(std::vector<Renderable> &r, std::vector<Renderable> &r_dead);

	void checkEvents(const FPoint& loc);
	void checkHotspots();
	void checkNearestEvent();
	void checkTooltip();
	
	// some events are automatically triggered when the map is loaded
	void executeOnLoadEvents();
	
	// some events are triggered on exiting the map
	void executeOnMapExitEvents();

	// some events can trigger powers
	void activatePower(int power_index, unsigned statblock_index, FPoint &target);

	bool isValidTile(const unsigned &tile);
	Point centerTile(const Point& p);

	// cam(x,y) is where on the map the camera is pointing
	FPoint cam;

	// indicates that the map was changed by an event, so the GameStatePlay
	// will tell the mini map to update.
	bool map_change;

	MapCollision collider;

	// event-created loot or items
	std::vector<Event_Component> loot;
	Point loot_count;

	// teleport handling
	bool teleportation;
	FPoint teleport_destination;
	std::string teleport_mapname;
	std::string respawn_map;
	FPoint respawn_point;

	// cutscene handling
	bool cutscene;
	std::string cutscene_file;

	// shaky cam
	int shaky_cam_ticks;

	// trove handling
	bool trove;
	FPoint trove_pos;
	// kiln handling
	bool kiln;
	FPoint kiln_pos;

	// enemy clear
	bool enemies_cleared;

	// event talker
	std::string event_npc;

	// trigger for save game events
	bool save_game;

	// map soundids
	std::vector<SoundManager::SoundID> sids;

	// npc handling
	int npc_id;

	// book from map event
	std::string show_book;

	void loadMusic();

	/**
	 * The index of the layer, which mixes with the objects on screen. Layers
	 * before that are painted below objects; Layers after are painted on top.
	 */
	unsigned index_objectlayer;

};


#endif
