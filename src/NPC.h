/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2013 Henrik Andersson
Copyright © 2012-2015 Justin Jacobs

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
 * class NPC
 */

#ifndef NPC_H
#define NPC_H

#include "CommonIncludes.h"
#include "Entity.h"
#include "ItemStorage.h"
#include "Utils.h"

const int NPC_VENDOR_MAX_STOCK = 80;
const int NPC_VOX_INTRO = 0;
const int NPC_VOX_MISSION = 1;

class NPC : public Entity {
private:
	bool isDialogType(const EVENT_COMPONENT_TYPE &type);

	std::vector<Event_Component> random_table;
	Point random_table_count;
	std::vector<std::string> portrait_filenames;

public:
	NPC();
	~NPC();
	void load(const std::string& npc_id);
	void loadGraphics();
	int loadSound(const std::string& fname, int type);
	void logic();
	bool playSound(int type, int id=-1);
	void getDialogNodes(std::vector<int> &result);
	std::string getDialogTopic(unsigned int dialog_node);
	bool checkMovement(unsigned int dialog_node);
	bool checkVendor();
	bool processDialog(unsigned int dialog_node, unsigned int& event_cursor);
	void processEvent(unsigned int dialog_node, unsigned int cursor);
	virtual Renderable getRender();

	// general info
	std::string name;
	std::string filename;
	std::string gfx; // filename of sprite.
	FPoint pos; // map position

	int direction;

	// talker info
	Sprite* npc_portrait;
	Sprite* hero_portrait;
	std::vector<Sprite*> portraits;
	bool talker;

	// vendor info
	bool vendor;
	ItemStorage stock;
	int stock_count;
	std::vector<std::string> vendor_requires_status;
	std::vector<std::string> vendor_requires_not_status;

	// vocals
	std::vector<SoundManager::SoundID> vox_intro;
	std::vector<SoundManager::SoundID> vox_missions;

	// story and dialog options
	// outer vector is addressing the dialog and the inner vector is
	// addressing the events during one dialog
	std::vector<std::vector<Event_Component> > dialog;
};

#endif
