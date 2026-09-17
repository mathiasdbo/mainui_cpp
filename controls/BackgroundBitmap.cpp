/*
BackgroundBitmap.cpp -- background menu item
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

#include "extdll_menu.h"
#include "BaseMenu.h"
#include "BackgroundBitmap.h"
#include "Utils.h"
#include "BaseWindow.h"

bool CMenuBackgroundBitmap::s_bEnableLogoMovie = false;
bool CMenuBackgroundBitmap::s_bGameHasSteamBackground = false;
bool CMenuBackgroundBitmap::s_bGameHasWONBackground = false;
CMenuBackgroundBitmap::bstate_e CMenuBackgroundBitmap::s_state;

CMenuBackgroundBitmap::bimage_t CMenuBackgroundBitmap::s_WONBackground;

Size CMenuBackgroundBitmap::s_SteamBackgroundImageSize;
CUtlVector<CMenuBackgroundBitmap::bimage_t> CMenuBackgroundBitmap::s_SteamBackground;

CMenuBackgroundBitmap::CMenuBackgroundBitmap() : CMenuBitmap()
{
	szPic = 0;
	iFlags = QMF_INACTIVE|QMF_DISABLESCAILING;
	bForceColor = false;
}

void CMenuBackgroundBitmap::VidInit()
{
	pos.x = pos.y = 0;

	if( m_pParent )
	{
		// fill parent
		if( m_pParent->iFlags & QMF_DISABLESCAILING )
		{
			size = m_pParent->size;
		}
		else
		{
			size = m_pParent->size.Scale();
		}
	}
	else
	{
		size = Size( ScreenWidth, ScreenHeight );
	}

	colorBase.SetDefault( 0xFF505050 );

	CMenuBaseItem::VidInit();
}

void CMenuBackgroundBitmap::DrawInGameBackground()
{
	UI_FillRect( m_scPos, m_scSize, uiColorBlack );
}

void CMenuBackgroundBitmap::DrawColor()
{
	if( bDrawStroke )
	{
		UI_DrawRectangleExt( m_scPos, m_scSize, colorStroke, 1 );
	}

	UI_FillRect( m_scPos, m_scSize, colorBase );
}

void CMenuBackgroundBitmap::DrawBackgroundPiece( const bimage_t &image, Point p, int xOffset, int yOffset, float xScale, float yScale )
{
	EngFuncs::PIC_Set( image.hImage, 255, 255, 255, 255 );

	int dx = (int)ceil( image.coord.x * xScale );
	int dy = (int)ceil( image.coord.y * yScale );
	int dw = (int)ceil( image.size.w * xScale );
	int dt = (int)ceil( image.size.h * yScale );

	EngFuncs::PIC_Draw( p.x + dx + xOffset, p.y + dy + yOffset, dw, dt );
}

void CMenuBackgroundBitmap::DrawSteamBackgroundLayout( Point p, int xOffset, int yOffset, float xScale, float yScale )
{
	// iterate and draw all the background pieces
	for( int i = 0; i < s_SteamBackground.Count(); i++ )
		DrawBackgroundPiece( s_SteamBackground[i], p, xOffset, yOffset, xScale, yScale );
}

/*
=================
CMenuBackgroundBitmap::Draw
=================
*/
void CMenuBackgroundBitmap::Draw()
{
	if( bForceColor )
	{
		DrawColor();
		return;
	}

	if( EngFuncs::ClientInGame() )
	{
		if( EngFuncs::GetCvarFloat( "cl_background" ) )
		{
			return;
		}

		if( EngFuncs::GetCvarFloat( "ui_renderworld" ) )
		{
			DrawInGameBackground();
			return;
		}
	}

	if( szPic )
	{
		UI_DrawPic( m_scPos, m_scSize, uiColorWhite, szPic );
		return;
	}

	if( FBitSet( ui_prefer_won_background->flags, FCVAR_CHANGED ))
	{
		// XM item 0 step 3: only one profile stays resident (LoadBackground()
		// already freed the loser), so flipping the preference at runtime has
		// to move residency over, not just repoint s_state - (re)load
		// whichever side is now preferred if it isn't loaded, and only free
		// the side losing the flip once the replacement actually loaded
		// (Codex review round 1: PIC_Load can fail - out of memory, a
		// missing file - and freeing the old side unconditionally would
		// then discard a background that was still working for one that
		// silently isn't there).
		bstate_e prevState = s_state;

		UpdatePreference();

		if( s_state != prevState )
		{
			bool switched = true;

			if( s_state == DRAW_WON && !s_WONBackground.hImage )
				switched = LoadWONBackground( true ) || LoadWONBackground( false );
			else if( s_state == DRAW_STEAM && s_SteamBackground.Count() == 0 )
				switched = LoadSteamBackground( true ) || LoadSteamBackground( false );

			if( switched )
			{
				if( prevState == DRAW_WON )
					FreeWONBackground();
				else if( prevState == DRAW_STEAM )
					FreeSteamBackground();
			}
			else
			{
				s_state = prevState; // reload failed - keep showing what's still resident
			}
		}

		// because the cvar is set by user, tell them if chosen background is not available
		if( ui_prefer_won_background->value && s_state != DRAW_WON )
			UI_ShowMessageBox( L( "WON background is not available" ));
	}

	Point p;
	float xScale, yScale;
	Size s;

	if( s_state == DRAW_COLOR )
	{
		DrawColor();
		return;
	}
	else if( s_state == DRAW_WON )
		s = s_WONBackground.size;
	else
		s = s_SteamBackgroundImageSize;

	// Disable parallax effect. It's just funny, but not really needed
#if 0
	float flParallaxScale = 0.02;
	p.x = (uiStatic.cursorX - ScreenWidth) * flParallaxScale;
	p.y = (uiStatic.cursorY - ScreenHeight) * flParallaxScale;

	// work out scaling factors
	// work out scaling factors
	if( ScreenWidth * s.h > ScreenHeight * s.w )
	{
		xScale = ScreenWidth / s.w * (1 + flParallaxScale);
		yScale = xScale;
	}
	else
	{
		yScale = ScreenHeight / s.h * (1 + flParallaxScale);
		xScale = yScale;
	}
#else
	p.x = p.y = 0;

	// Stretch the background image if the user chose to do so
	if( ui_background_stretch->value )
	{
		xScale = ScreenWidth / s.w;
		yScale = ScreenHeight / s.h;
	}
	else
	{
		// work out scaling factors
		if( ScreenWidth * s.h > ScreenHeight * s.w )
		{
			xScale = ScreenWidth / s.w;
			yScale = xScale;
		}
		else
		{
			yScale = ScreenHeight / s.h;
			xScale = yScale;
		}
	}
#endif

	int xOffset = 0, yOffset = 0;
	if( !ui_background_stretch->value )
	{
		// center wide background (for example if background is wider than our window)
		if( s.w * xScale > ScreenWidth )
			xOffset = ( ScreenWidth - s.w * xScale ) / 2;
		else if( s.h * yScale > ScreenHeight )
			yOffset = ( ScreenHeight - s.h * yScale ) / 2;
	}

	if( s_state == DRAW_WON )
		DrawBackgroundPiece( s_WONBackground, p, xOffset, yOffset, xScale, yScale );
	else
		DrawSteamBackgroundLayout( p, xOffset, yOffset, xScale, yScale );
}

bool CMenuBackgroundBitmap::LoadSteamBackground( bool gamedirOnly )
{
	char *afile = NULL, *pfile;
	char token[4096];

	bool loaded = false;
	// XM item 0 step 3 (Codex review round 1): remember what was already
	// in s_SteamBackground before this attempt, so a failed parse rolls
	// back only what THIS call appended, rather than leaving a partial
	// tile set behind that a later Count() > 0 check could mistake for a
	// complete, displayable background.
	const int startCount = s_SteamBackground.Count();

	// try 25'th anniversary update background first
	if( FBitSet( gMenu.m_gameinfo.flags, GFL_HD_BACKGROUND ))
		afile = (char *)EngFuncs::COM_LoadFile( "resource/HD_BackgroundLayout.txt" );

	if( !afile )
		afile = (char *)EngFuncs::COM_LoadFile( "resource/BackgroundLayout.txt" );

	if( !afile )
		return false;

	pfile = afile;

	pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token ) );
	if( !pfile || strcmp( token, "resolution" )) // resolution at first!
		goto freefile;

	pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token ) );
	if( !pfile ) goto freefile;

	s_SteamBackgroundImageSize.w = atoi( token );

	pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token ) );
	if( !pfile ) goto freefile;

	s_SteamBackgroundImageSize.h = atoi( token );

	// Now read all tiled background list
	while(( pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token ) )))
	{
		bimage_t img;
		char tileName[sizeof( img.name )];

		// XM item 0 step 3 (Codex review round 2): token gets overwritten by
		// each COM_ParseFile call below, so the filename has to be saved
		// before parsing the rest of this tile's line - and PIC_Load has to
		// be the LAST fallible step, right before AddToTail, so a malformed
		// later field (a missing "scaled" token, a missing coordinate) can
		// never leave an already-loaded texture that this tile never
		// actually keeps a name for and the failure-path rollback below can
		// never reach.
		Q_strncpy( tileName, token, sizeof( tileName ));

		if( !EngFuncs::FileExists( tileName, gamedirOnly ))
			goto freefile;

		// ignore "scaled" attribute. What does it mean?
		pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token ) );
		if( !pfile ) goto freefile;

		pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token ) );
		if( !pfile ) goto freefile;
		img.coord.x = atoi( token );

		pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token ) );
		if( !pfile ) goto freefile;
		img.coord.y = atoi( token );

		img.hImage = EngFuncs::PIC_Load( tileName, PIC_NOFLIP_TGA );

		if( !img.hImage ) goto freefile;

		Q_strncpy( img.name, tileName, sizeof( img.name ));
		img.size.w = EngFuncs::PIC_Width( img.hImage );
		img.size.h = EngFuncs::PIC_Height( img.hImage );

		s_SteamBackground.AddToTail( img );
	}

	loaded = true;

freefile:
	if( !loaded )
	{
		// roll back this attempt's own partial tile list - see startCount above
		while( s_SteamBackground.Count() > startCount )
		{
			EngFuncs::PIC_Free( s_SteamBackground[s_SteamBackground.Count() - 1].name );
			s_SteamBackground.Remove( s_SteamBackground.Count() - 1 );
		}
	}

	EngFuncs::COM_FreeFile( afile );
	return loaded;
}

bool CMenuBackgroundBitmap::LoadWONBackground( bool gamedirOnly )
{
	if( EngFuncs::FileExists( ART_BACKGROUND, gamedirOnly ))
	{
		bimage_t img;

		img.hImage = EngFuncs::PIC_Load( ART_BACKGROUND_PIC );

		if( !img.hImage )
			return false;

		img.coord.x = img.coord.y = 0;
		img.size.w = EngFuncs::PIC_Width( img.hImage );
		img.size.h = EngFuncs::PIC_Height( img.hImage );
		Q_strncpy( img.name, ART_BACKGROUND_PIC, sizeof( img.name ));
		s_WONBackground = img;

		return true;
	}

	return false;
}

void CMenuBackgroundBitmap::UpdatePreference()
{
	if( ui_prefer_won_background->value )
	{
		if( s_bGameHasWONBackground )
			s_state = DRAW_WON;
		else if( s_bGameHasSteamBackground )
			s_state = DRAW_STEAM;
		else if( s_WONBackground.hImage )
			s_state = DRAW_WON;
		else if( s_SteamBackground.Count() > 0 )
			s_state = DRAW_STEAM;
		else
			s_state = DRAW_COLOR;
	}
	else
	{
		if( s_bGameHasSteamBackground )
			s_state = DRAW_STEAM;
		else if( s_bGameHasWONBackground )
			s_state = DRAW_WON;
		else if( s_SteamBackground.Count() > 0 )
			s_state = DRAW_STEAM;
		else if( s_WONBackground.hImage )
			s_state = DRAW_WON;
		else
			s_state = DRAW_COLOR;
	}

	// Enable logo.avi only for WON background, disable otherwise
	if( s_state == DRAW_WON )
		s_bEnableLogoMovie = EngFuncs::FileExists( "media/logo.avi", true );
	else
		s_bEnableLogoMovie = false;

	ClearBits( ui_prefer_won_background->flags, FCVAR_CHANGED );
}

// XM item 0 step 3 (fork-plan.md): LoadBackground() below still probes both
// profiles by loading both once - re-parsing just to decide which one
// exists would risk disagreeing with the real loaders' own validation, so
// the honest way to "probe, prefer, load one" is to load both, decide, and
// free the one not shown right away, before anything ever measures
// resident memory. Not Xbox-guarded: a mixed-content disc wastes the same
// memory holding two resident backgrounds on any platform, and freeing the
// one never drawn changes nothing a player can see.
void CMenuBackgroundBitmap::FreeWONBackground()
{
	if( !s_WONBackground.hImage )
		return;

	EngFuncs::PIC_Free( s_WONBackground.name );
	s_WONBackground.hImage = 0;
}

void CMenuBackgroundBitmap::FreeSteamBackground()
{
	for( int i = 0; i < s_SteamBackground.Count(); i++ )
		EngFuncs::PIC_Free( s_SteamBackground[i].name );

	s_SteamBackground.RemoveAll();
}

void CMenuBackgroundBitmap::LoadBackground()
{
	s_bEnableLogoMovie = false;
	s_bGameHasSteamBackground = false;
	s_bGameHasWONBackground = false;

	// XM item 0 (fork-plan.md): lowmemory alone would also block the retail
	// background here, but that load is budgeted and measured on its own -
	// "ui_xbox_menu_art" is the finer gate. Read live, not cached in
	// uiStatic at UI_Init time (BaseMenu.cpp's own UI_Init comment on why
	// this function is called from UI_UpdateMenu instead: cvars from user
	// configs are not live yet at Init) - measured live, 2026-09-16, a
	// cached read never saw a userconfig.d override.
	if( uiStatic.lowmemory && !EngFuncs::GetCvarFloat( "ui_xbox_menu_art" ))
		return;

	// XM item 0 step 3 (Codex review round 1): s_bGameHasXBackground drives
	// UpdatePreference()'s decision, so it has to mean "this background
	// exists at all" - found via THIS gamedir or via the base fallback -
	// not just "found in the game's own directory". Before this fix that
	// narrower meaning was harmless because both backgrounds were always
	// loaded unconditionally, so UpdatePreference()'s own secondary
	// fallback (checking what's actually resident) caught a base-only
	// background; now that the loser gets freed, that secondary signal is
	// gone for the profile not currently shown, and a base-only
	// background would otherwise read as unavailable forever.
	if( LoadSteamBackground( true ))
	{
		Con_DPrintf( "%s: found %s background in %s directory\n", __func__, "steam", "game" );
		s_bGameHasSteamBackground = true;
	}
	else if( LoadSteamBackground( false ))
	{
		Con_DPrintf( "%s: found %s background in %s directory\n", __func__, "steam", "base" );
		s_bGameHasSteamBackground = true;
	}

	if( LoadWONBackground( true ))
	{
		Con_DPrintf( "%s: found %s background in %s directory\n", __func__, "won", "game" );
		s_bGameHasWONBackground = true;
	}
	else if( LoadWONBackground( false ))
	{
		Con_DPrintf( "%s: found %s background in %s directory\n", __func__, "won", "base" );
		s_bGameHasWONBackground = true;
	}

	UpdatePreference();

	// XM item 0 step 3: keep only the displayed profile resident.
	if( s_state == DRAW_WON )
		FreeSteamBackground();
	else if( s_state == DRAW_STEAM )
		FreeWONBackground();
}
