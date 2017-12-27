/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2014-2015 Justin Jacobs

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

#ifndef MISSION_LOG_H
#define MISSION_LOG_H

#include "CommonIncludes.h"
#include "Utils.h"

class MenuLog;

class MissionLog {
private:
	MenuLog *log;

	// inner vector is a chain of events per mission, outer vector is a
	// list of missions.
	std::vector<std::vector<Event_Component> >missions;

	std::vector<size_t> active_mission_ids;
	std::vector<std::string> mission_names;

public:
	explicit MissionLog(MenuLog *_log);
	~MissionLog();
	void loadAll();
	void load(const std::string& filename);
	void logic();
	void createMissionList();
	bool newMissionNotification;
};

#endif
