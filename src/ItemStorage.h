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
 * class ItemStorage
 */

#ifndef ITEM_STORAGE_H
#define ITEM_STORAGE_H

#include "CommonIncludes.h"

class ItemManager;
class ItemStack;

class ItemStorage {
protected:
	int slot_number;

public:
	void init(int _slot_number);
	~ItemStorage();

	ItemStack & operator [] (int slot);

	void setItems(const std::string& s);
	void setQuantities(const std::string& s);
	int getSlotNumber();
	std::string getItems();
	std::string getQuantities();
	ItemStack add(ItemStack stack, int slot = -1);
	void subtract(int slot, int quantity = 1);
	bool remove(int item, int quantity = 1);
	void sort();
	void clear();
	void clean();

	bool full(ItemStack stack);
	int count(int item);
	bool contain(int item, int quantity = 1);

	ItemStack * storage;
};

#endif

