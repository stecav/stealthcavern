/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Stefan Beller
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
 * class MissionLog
 *
 * Helper text to remind the player of active missions
 */

#include "CommonIncludes.h"
#include "FileParser.h"
#include "Menu.h"
#include "MenuLog.h"
#include "MissionLog.h"
#include "Settings.h"
#include "SharedResources.h"
#include "UtilsFileSystem.h"
#include "UtilsParsing.h"
#include "SharedGameResources.h"

MissionLog::MissionLog(MenuLog *_log) {
	log = _log;

	newMissionNotification = false;
	loadAll();
}

MissionLog::~MissionLog() {
}

/**
 * Load each [mod]/missions/index.txt file
 */
void MissionLog::loadAll() {
	// load each items.txt file. Individual item IDs can be overwritten with mods.
	std::vector<std::string> files = mods->list("missions", false);
	std::sort(files.begin(), files.end());
	for (unsigned int i = 0; i < files.size(); i++)
		load(files[i]);
}

/**
 * Load the missions in the specific mission file.
 * Searches for the last-defined such file in all mods
 *
 * @param filename The mission file name and extension, no path
 */
void MissionLog::load(const std::string& filename) {
	FileParser infile;
	// @CLASS MissionLog|Description of mission files in missions/
	if (!infile.open(filename))
		return;

	mission_names.resize(mission_names.size()+1);
	mission_names.back() = "";

	while (infile.next()) {
		if (infile.new_section) {
			if (infile.section == "mission") {
				missions.push_back(std::vector<Event_Component>());
			}
		}

		if (infile.section == "") {
			if (infile.key == "name") {
				// @ATTR name|string|A displayed name for this mission.
				mission_names.back() = msg->get(infile.val);
			}

			continue;
		}

		if (missions.empty())
			continue;

		Event ev;
		if (infile.key == "requires_status") {
			// @ATTR mission.requires_status|list(string)|Mission requires this campaign status
			EventManager::loadEventComponent(infile, &ev, NULL);
		}
		else if (infile.key == "requires_not_status") {
			// @ATTR mission.requires_not_status|list(string)|Mission requires not having this campaign status.
			EventManager::loadEventComponent(infile, &ev, NULL);
		}
		else if (infile.key == "requires_generation") {
			// @ATTR mission.requires_generation|int|Mission requires hero generation
			EventManager::loadEventComponent(infile, &ev, NULL);
		}
		else if (infile.key == "requires_not_generation") {
			// @ATTR mission.requires_not_generation|int|Mission requires not hero generation
			EventManager::loadEventComponent(infile, &ev, NULL);
		}
		else if (infile.key == "requires_currency") {
			// @ATTR mission.requires_currency|int|Mission requires atleast this much currency
			EventManager::loadEventComponent(infile, &ev, NULL);
		}
		else if (infile.key == "requires_not_currency") {
			// @ATTR mission.requires_not_currency|int|Mission requires no more than this much currency
			EventManager::loadEventComponent(infile, &ev, NULL);
		}
		else if (infile.key == "requires_talent") {
			// @ATTR mission.requires_talent|int|Mission requires a talent
			EventManager::loadEventComponent(infile, &ev, NULL);
		}
		else if (infile.key == "requires_item") {
			// @ATTR mission.requires_item|list(item_id)|Mission requires specific item (not equipped)
			EventManager::loadEventComponent(infile, &ev, NULL);
		}
		else if (infile.key == "requires_not_item") {
			// @ATTR mission.requires_not_item|list(item_id)|Mission requires not having a specific item (not equipped)
			EventManager::loadEventComponent(infile, &ev, NULL);
		}
		else if (infile.key == "requires_class") {
			// @ATTR mission.requires_class|predefined_string|Mission requires this base class
			EventManager::loadEventComponent(infile, &ev, NULL);
		}
		else if (infile.key == "requires_not_class") {
			// @ATTR mission.requires_not_class|predefined_string|Mission requires not this base class
			EventManager::loadEventComponent(infile, &ev, NULL);
		}
		else if (infile.key == "mission_text") {
			// @ATTR mission.mission_text|string|Text that gets displayed in the Mission log when this mission is active.
			Event_Component ec;
			ec.type = EC_MISSION_TEXT;
			ec.s = msg->get(infile.val);

			// mission group id
			ec.x = static_cast<int>(mission_names.size()-1);

			ev.components.push_back(ec);
		}
		else {
			logError("MissionLog: %s is not a valid key.", infile.key.c_str());
		}

		for (size_t i=0; i<ev.components.size(); ++i) {
			if (ev.components[i].type != EC_NONE)
				missions.back().push_back(ev.components[i]);
		}
	}
	infile.close();
}

void MissionLog::logic() {
	createMissionList();
}

/**
 * All active missions are placed in the Mission tab of the Log Menu
 */
void MissionLog::createMissionList() {
	std::vector<size_t> temp_mission_ids;

	// check mission requirements
	for (size_t i=0; i<missions.size(); i++) {
		bool requirements_met = false;

		for (size_t j=0; j<missions[i].size(); j++) {
			if (missions[i][j].type == EC_MISSION_TEXT) {
				continue;
			}
			else {
				// check requirements
				// break (skip to next dialog node) if any requirement fails
				// if we reach an event that is not a requirement, succeed
				if (!camp->checkAllRequirements(missions[i][j])) {
					requirements_met = false;
					break;
				}
			}

			requirements_met = true;
		}

		if (requirements_met) {
			// passed requirement checks, add ID to active mission list
			temp_mission_ids.push_back(i);
		}
	}

	// check if we actually need to update the mission log
	bool refresh_mission_list = false;
	if (temp_mission_ids.size() != active_mission_ids.size()) {
		refresh_mission_list = true;
	}
	else {
		for (size_t i=0; i<temp_mission_ids.size(); ++i) {
			if (temp_mission_ids[i] != active_mission_ids[i]) {
				refresh_mission_list = true;
				break;
			}
		}
	}

	// update the mission log
	if (refresh_mission_list) {
		active_mission_ids = temp_mission_ids;
		newMissionNotification = true;

		log->clear(LOG_TYPE_MISSIONS);

		for (size_t i=active_mission_ids.size(); i>0; i--) {
			size_t k = active_mission_ids[i-1];

			size_t i_next = (i > 1) ? i-2 : 0;
			size_t k_next = active_mission_ids[i_next];

			// get the group id of the next active mission
			int next_mission_id = 0;
			for (size_t j=0; j<missions[k_next].size(); j++) {
				if (missions[k_next][j].type == EC_MISSION_TEXT) {
					next_mission_id = missions[k_next][j].x;
					break;
				}
			}

			for (size_t j=0; j<missions[k].size(); j++) {
				if (missions[k][j].type == EC_MISSION_TEXT) {
					log->add(missions[k][j].s, LOG_TYPE_MISSIONS, false);

					if (next_mission_id != missions[k][j].x) {
						if (mission_names[missions[k][j].x] != "")
							log->add(mission_names[missions[k][j].x], LOG_TYPE_MISSIONS, false, NULL, WIDGETLOG_FONT_BOLD);

						log->addSeparator(LOG_TYPE_MISSIONS);
					}
					else if (i == 1) {
						if (mission_names[missions[k][j].x] != "")
							log->add(mission_names[missions[k][j].x], LOG_TYPE_MISSIONS, false, NULL, WIDGETLOG_FONT_BOLD);
					}

					break;
				}
			}
		}
	}
}
