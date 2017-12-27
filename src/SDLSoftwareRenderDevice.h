/*
Copyright © 2013 Kurt Rinnert
Copyright © 2013 Igor Paliychuk
Copyright © 2014 Henrik Andersson
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

#ifndef SDLSOFTWARERENDERDEVICE_H
#define SDLSOFTWARERENDERDEVICE_H

#include "RenderDevice.h"

/** Provide rendering device using SDL_BlitSurface backend.
 *
 * Provide an SDL_BlitSurface implementation for renderning a Renderable to
 * the screen.  Simply dispatches rendering to SDL_BlitSurface().
 *
 * As this is for the Stealth Cavern engine, the implementation uses the engine's
 * global settings context, which is included by the interface.
 *
 * @class SDLSoftwareRenderDevice
 * @see RenderDevice
 * @author Kurt Rinnert
 * @date 2013-07-06
 *
 */


/** SDL Image */
class SDLSoftwareImage : public Image {
public:
	explicit SDLSoftwareImage(RenderDevice *device);
	virtual ~SDLSoftwareImage();
	int getWidth() const;
	int getHeight() const;

	void fillWithColor(const Color& color);
	void drawPixel(int x, int y, const Color& color);
	Image* resize(int width, int height);

	SDL_Surface *surface;

private:
	Uint32 MapRGBA(Uint8 r, Uint8 g, Uint8 b, Uint8 a);
};

class SDLSoftwareRenderDevice : public RenderDevice {

public:

	SDLSoftwareRenderDevice();
	int createContext(bool allow_fallback = true);

	virtual int render(Renderable& r, Rect& dest);
	virtual int render(Sprite* r);
	virtual int renderToImage(Image* src_image, Rect& src, Image* dest_image, Rect& dest);

	int renderText(FontStyle *font_style, const std::string& text, const Color& color, Rect& dest);
	Image* renderTextToImage(FontStyle* font_style, const std::string& text, const Color& color, bool blended = true);
	void drawPixel(int x, int y, const Color& color);
	void drawRectangle(const Point& p0, const Point& p1, const Color& color);
	void blankScreen();
	void commitFrame();
	void destroyContext();
	void windowResize();
	void showMouseCursor();
	void hideMouseCursor();
	Image *createImage(int width, int height);
	void setGamma(float g);
	void updateTitleBar();

	Image* loadImage(const std::string& filename,
					 const std::string& errormessage = "Couldn't load image",
					 bool IfNotFoundExit = false);
private:
	Uint32 MapRGBA(Uint8 r, Uint8 g, Uint8 b, Uint8 a);
	void drawLine(int x0, int y0, int x1, int y1, const Color& color);
	void setSDL_RGBA(Uint32 *rmask, Uint32 *gmask, Uint32 *bmask, Uint32 *amask);

	SDL_Surface* screen;
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* texture;
	SDL_Surface* titlebar_icon;
	char* title;
};

#endif // SDLSOFTWARERENDERDEVICE_H
