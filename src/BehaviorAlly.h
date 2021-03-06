/*
Copyright © 2013 Ryan Dansie

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


#ifndef BEHAVIORALLY_H
#define BEHAVIORALLY_H

#include "BehaviorStandard.h"

class BehaviorAlly : public BehaviorStandard {
public:
	explicit BehaviorAlly(Enemy *_e);
	virtual ~BehaviorAlly();
protected:
private:
	virtual void findTarget();
	virtual void checkMoveStateStance();
	virtual void checkMoveStateMove();
};

#endif // BEHAVIORALLY_H
