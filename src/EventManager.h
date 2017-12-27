/*
Copyright © 2013 Igor Paliychuk
Copyright © 2013-2016 Justin Jacobs

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

/*
 * class EventManager
 */

#ifndef EVENT_MANAGER_H
#define EVENT_MANAGER_H

#include "FileParser.h"
#include "Utils.h"

enum EVENT_ACTIVATE_TYPE {
	EVENT_ON_TRIGGER = 0,
	EVENT_ON_MAPEXIT = 1,
	EVENT_ON_LEAVE = 2,
	EVENT_ON_LOAD = 3,
	EVENT_ON_CLEAR = 4,
	EVENT_STATIC = 5
};

class Event {
public:
	std::string type;
	int activate_type;
	std::vector<Event_Component> components;
	Rect location;
	Rect hotspot;
	int cooldown; // events that run multiple times pause this long in frames
	int cooldown_ticks;
	bool keep_after_trigger; // if this event has been triggered once, should this event be kept? If so, this event can be triggered multiple times.
	FPoint center;
	Rect reachable_from;

	Event();
	~Event();

	Event_Component* getComponent(const EVENT_COMPONENT_TYPE &_type);
	void deleteAllComponents(const EVENT_COMPONENT_TYPE &_type);
};

class EventManager {
public:
	EventManager();
	~EventManager();
	static void loadEvent(FileParser &infile, Event* evnt);
	static void loadEventComponent(FileParser &infile, Event* evnt, Event_Component* ec);
	static bool loadEventComponentString(std::string &key, std::string &val, Event* evnt, Event_Component* ec);

	static bool executeEvent(Event &e);
	static bool isActive(const Event &e);
	static void executeScript(const std::string& filename, float x, float y);
};


#endif
