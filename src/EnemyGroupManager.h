/*
Copyright © 2011-2012 Thane Brimhall
		Manuel A. Fernandez Montecelo <manuel.montezelo@gmail.com>

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

#ifndef ENEMYGROUPMANAGER_H
#define ENEMYGROUPMANAGER_H

#include "CommonIncludes.h"

class Enemy_Generation {
public:
	std::string type;
	int generation;
	std::string rarity;

	Enemy_Generation() : generation(0), rarity("common") {}

};

/**
 * class EnemyGroupManager
 *
 * Loads Enemies into category lists and manages spawning randomized groups of
 * enemies.
 */
class EnemyGroupManager {
public:
	EnemyGroupManager();
	~EnemyGroupManager();

	/** To get a random enemy with the given characteristics
	 *
	 * @param category Enemy of the desired category
	 * @param mingeneration Enemy of the desired generation (minimum)
	 * @param maxgeneration Enemy of the desired generation (maximum)
	 *
	 * @return A random enemy generation description if a suitable was found.
	 *         Null if none was found.
	 */
	Enemy_Generation getRandomEnemy(const std::string& category, int mingeneration, int maxgeneration) const;

	/** To get enemies that fit in a category
	 *
	 * @param category Enemies of the desired category
	 *
	 * @return generation descriptions of enemies in the category.
	 *         Empty buffer if none was found.
	 */
	std::vector<Enemy_Generation> getEnemiesInCategory(const std::string& category) const;

private:

	/** Container to store enemy data */
	std::map <std::string, std::vector<Enemy_Generation> > _categories;
};

#endif
