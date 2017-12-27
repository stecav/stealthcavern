/*
Copyright © 2011-2012 Thane Brimhall
Copyright © 2013-2015 Justin Jacobs
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

#include "EnemyGroupManager.h"
#include "FileParser.h"
#include "Settings.h"
#include "SharedGameResources.h"
#include "SharedResources.h"
#include "UtilsFileSystem.h"
#include "UtilsParsing.h"

#include <cassert>

EnemyGroupManager::EnemyGroupManager() {
	std::vector<std::string> enemy_paths = mods->list("enemies", false);

	for (unsigned i=0; i<enemy_paths.size(); ++i) {
		FileParser infile;

		// @CLASS EnemyGroupManager|Description of enemies in enemies/
		if (!infile.open(enemy_paths[i]))
			return;

		Enemy_Generation new_enemy;
		infile.new_section = true;
		bool first = true;
		while (infile.next()) {
			if (infile.new_section || first) {
				new_enemy.type = enemy_paths[i];
				first = false;
			}

			if (infile.key == "generation") {
				// @ATTR generation|int|Generation of the enemy
				new_enemy.generation = toInt(infile.val);
			}
			else if (infile.key == "rarity") {
				// @ATTR rarity|["common", "uncommon", "rare"]|Enemy rarity
				new_enemy.rarity = infile.val;
			}
			else if (infile.key == "categories") {
				// @ATTR categories|list(predefined_string)|Comma separated list of enemy categories
				std::string cat;
				while ( (cat = popFirstString(infile.val)) != "") {
					_categories[cat].push_back(new_enemy);
				}
			}
		}
		infile.close();
	}
}

EnemyGroupManager::~EnemyGroupManager() {
}

Enemy_Generation EnemyGroupManager::getRandomEnemy(const std::string& category, int mingeneration, int maxgeneration) const {
	std::vector<Enemy_Generation> enemyCategory;
	std::map<std::string, std::vector<Enemy_Generation> >::const_iterator it = _categories.find(category);
	if (it != _categories.end()) {
		enemyCategory = it->second;
	}
	else {
		logError("EnemyGroupManager: Could not find enemy category %s, returning empty enemy", category.c_str());
		return Enemy_Generation();
	}

	// load only the data that fit the criteria
	std::vector<Enemy_Generation> enemyCandidates;
	for (size_t i = 0; i < enemyCategory.size(); ++i) {
		Enemy_Generation new_enemy = enemyCategory[i];
		if ((new_enemy.generation >= mingeneration && new_enemy.generation <= maxgeneration) || (mingeneration == 0 && maxgeneration == 0)) {
			// add more than one time to increase chance of getting
			// this enemy as result, "rarity" property
			int add_times = 0;
			if (new_enemy.rarity == "common") {
				add_times = 6;
			}
			else if (new_enemy.rarity == "uncommon") {
				add_times = 3;
			}
			else if (new_enemy.rarity == "rare") {
				add_times = 1;
			}
			else {
				logError("EnemyGroupManager: 'rarity' property for enemy '%s' not valid (common|uncommon|rare): %s",
						new_enemy.type.c_str(), new_enemy.rarity.c_str());
			}

			// do add, the given number of times
			for (int j = 0; j < add_times; ++j) {
				enemyCandidates.push_back(new_enemy);
			}
		}
	}

	if (enemyCandidates.empty()) {
		logError("EnemyGroupManager: Could not find a suitable enemy category for (%s, %d, %d)", category.c_str(), mingeneration, maxgeneration);
		return Enemy_Generation();
	}
	else {
		return enemyCandidates[rand() % enemyCandidates.size()];
	}
}

std::vector<Enemy_Generation> EnemyGroupManager::getEnemiesInCategory(const std::string& category) const {
	std::map<std::string, std::vector<Enemy_Generation> >::const_iterator it = _categories.find(category);
	if (it == _categories.end()) {
		logError("EnemyGroupManager: Could not find enemy category %s, returning empty enemy list", category.c_str());
		return std::vector<Enemy_Generation>();
	}
	return it->second;
}
