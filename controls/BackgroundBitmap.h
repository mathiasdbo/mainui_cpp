/*
BackgroundBitmap.h -- background menu item
Copyright (C) 2010 Uncle Mike
Copyright (C) 2017 a1batross

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifndef MENU_BACKGROUNDBITMAP_H
#define MENU_BACKGROUNDBITMAP_H

#include "Bitmap.h"
#include "utlvector.h"

#define ART_BACKGROUND		"gfx/shell/splash.bmp"
// XM item 0 step 3 (fork-plan.md), Codex review round 2: PIC_Load/PIC_Free
// cache and free by the EXACT string the caller passes, not the resolved
// filename - GL_TextureForName's lookup is a plain string match, done
// before FS_LoadImage's own extension probing ever runs
// (ref/soft/r_image.c:587, imagelib's img_main.c). UI_Precache()
// (BaseMenu.cpp) unconditionally preloads this same splash art under the
// key "gfx/shell/splash" (no extension) as part of its general icon
// precache. Loading WON's background under ART_BACKGROUND's own
// ".bmp"-suffixed key would create a SECOND, separately-cached copy of
// the identical decoded image - freeing FreeWONBackground()'s copy would
// then leave UI_Precache()'s copy resident regardless, defeating the
// whole point of this item's exclusivity fix. Using the exact same key
// UI_Precache() already uses makes LoadWONBackground()'s PIC_Load a
// cache hit against that one entry (or, if this runs first, the one
// entry UI_Precache() then hits) - there is only ever one resident copy,
// and FreeWONBackground() actually frees it. FileExists() still needs
// ART_BACKGROUND's real extension - it is a literal filesystem check
// with no extension-probing of its own.
#define ART_BACKGROUND_PIC	"gfx/shell/splash"

// Ultimate class that support multiple types of background: fillColor, WON-style, GameUI-style
class CMenuBackgroundBitmap: public CMenuBitmap
{
public:
	CMenuBackgroundBitmap();

	void VidInit( void ) override;
	void Draw( void ) override;
	void SetInactive(bool) override { }
	void ToggleInactive() override { }

	bool bForceColor; // dialogs should set this

	static void LoadBackground();
	static bool ShouldDrawLogoMovie() { return s_bEnableLogoMovie; }
private:
	struct bimage_t
	{
		HIMAGE hImage;
		Point coord;
		Size size;
		char name[64]; // XM item 0 step 3: PIC_Free() frees by name, not handle
	};

	enum bstate_e
	{
		DRAW_COLOR,
		DRAW_WON,
		DRAW_STEAM,
	};

	void DrawBackgroundPiece( const bimage_t &image, Point p, int xOffset, int yOffset, float xScale, float yScale );
	void DrawSteamBackgroundLayout( Point p, int xOffset, int yOffset, float xScale, float yScale );
	void DrawColor();
	void DrawInGameBackground();

	static bool LoadSteamBackground( const bool gamedirOnly ); // Steam background loader
	static bool LoadWONBackground( const bool gamedirOnly ); // WON background loader
	static void UpdatePreference();

	// XM item 0 step 3 (fork-plan.md): only the DISPLAYED profile's
	// background may stay resident - a mixed-content disc probes both by
	// loading both once, then frees whichever UpdatePreference() did not
	// select, and moves the residency over when the player flips
	// ui_prefer_won_background at runtime.
	static void FreeWONBackground();
	static void FreeSteamBackground();

	static bool s_bEnableLogoMovie, s_bGameHasSteamBackground, s_bGameHasWONBackground;

	static bstate_e s_state;

	static bimage_t s_WONBackground;

	static Size s_SteamBackgroundImageSize;
	static CUtlVector<bimage_t> s_SteamBackground;
};

#endif // MENU_BACKGROUNDBITMAP_H
