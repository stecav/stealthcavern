/*
Copyright © 2012 David Bariod
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

#ifndef UTILS_DEBUG_H
#define UTILS_DEBUG_H

extern std::ostream & operator<< (std::ostream &, const SDL_Event &);
extern std::ostream & operator<< (std::ostream &, const SDL_WindowEvent &);
extern std::ostream & operator<< (std::ostream &, const SDL_KeyboardEvent &);
extern std::ostream & operator<< (std::ostream &, const SDL_Keysym &);
extern std::ostream & operator<< (std::ostream &, const SDL_MouseMotionEvent &);
extern std::ostream & operator<< (std::ostream &, const SDL_MouseButtonEvent &);
extern std::ostream & operator<< (std::ostream &, const SDL_JoyAxisEvent &);
extern std::ostream & operator<< (std::ostream &, const SDL_JoyBallEvent &);
extern std::ostream & operator<< (std::ostream &, const SDL_JoyHatEvent &);
extern std::ostream & operator<< (std::ostream &, const SDL_JoyButtonEvent &);
extern std::ostream & operator<< (std::ostream &, const SDL_QuitEvent &);
extern std::ostream & operator<< (std::ostream &, const SDL_SysWMEvent &);
extern std::ostream & operator<< (std::ostream &, const Rect &);
extern std::ostream & operator<< (std::ostream &, const Point &);

#endif
