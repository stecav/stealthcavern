/*
Copyright © 2011-2012 Clint Bellanger
Copyright © 2012 Igor Paliychuk
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

#ifndef MENU_CONFIRM_H
#define MENU_CONFIRM_H

#include "CommonIncludes.h"
#include "Menu.h"
#include "WidgetButton.h"

class MenuConfirm : public Menu {
protected:
	WidgetButton *buttonConfirm;
	WidgetButton *buttonClose;
	WidgetLabel label;

	std::string boxMsg;
	bool hasConfirmButton;

public:
	MenuConfirm(const std::string&, const std::string&);
	~MenuConfirm();

	void logic();
	void align();
	virtual void render();

	bool confirmClicked;
	bool cancelClicked;
	bool isWithinButtons;
};

#endif
