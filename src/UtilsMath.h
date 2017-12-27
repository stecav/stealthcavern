/*
Copyright © 2012 Piotr Rak
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

#pragma once
#ifndef UTILS_MATH_H
#define UTILS_MATH_H 1

#include <cstdlib>
#include <algorithm> // for std::min()/std::max()
#include "math.h"

#ifndef M_PI
#define M_PI 3.1415926535898f
#endif

/**
 * Returns sign of value.
 */
inline int signum(const int value) {
	return (0 < value) - (value < 0);
}

/**
 * Returns random number between minVal and maxVal.
 */
inline int randBetween(int minVal, int maxVal) {
	if (minVal == maxVal) return minVal;
	int d = maxVal - minVal;
	return minVal + (rand() % (d + signum(d)));
}

/**
 * Returns true with random percent chance.
 */
inline bool percentChance(int percent) {
	return rand() % 100 < percent;
}

#endif // UTILS_MATH_H
