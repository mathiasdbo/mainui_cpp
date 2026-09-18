/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "Framework.h"
#include "Bitmap.h"
#include "PicButton.h"
#include "Slider.h"
#include "CheckBox.h"
#include "YesNoMessageBox.h"
#include "Action.h"
#if XASH_XBOX
#include "Legend.h"
#endif

#define ART_BANNER	  	"gfx/shell/head_vidoptions"
#define ART_GAMMA		"gfx/shell/gamma"

#define LEGACY_VIEWSIZE 0

class CMenuVidOptions : public CMenuFramework
{
private:
	void _Init() override;
	void _VidInit() override;
	void Reload() override;

public:
	CMenuVidOptions() : CMenuFramework( "CMenuVidOptions" ) { }
	void SaveAndPopMenu() override;
	void UpdateConfig();
	void GetConfig();

	int		outlineWidth;

	class CMenuVidPreview : public CMenuBitmap
	{
		void Draw() override;
	} testImage;

	CMenuSlider	gammaIntensity;
	CMenuSlider	brightness;
	CMenuCheckBox	filtering;

#if XASH_XBOX
	// XM.1 item 7: text heading in place of ART_BANNER, matching every
	// other Xbox screen this fork reshaped (Audio.cpp, GameOptions.cpp).
	CMenuAction	heading;
	// A read-only annotation, not a setting - real Xbox hardware state
	// (vid_width/vid_height/vid_refresh/vid_aspect, read-only cvars set
	// by vid_common.c's VID_SetXboxVideoCvars), never a literal (Codex
	// review of the plan, round 2: the design canvas's own "640x480 -
	// 4:3 - 60 Hz, set in Dashboard" is the NTSC 4:3 case, not the
	// string). Recomputed in _VidInit(), which BaseWindow.cpp's
	// own Show() runs on every visit to this screen, not just once.
	CMenuAction	videoOutput;
	char		videoOutputText[64];
	CMenuPicButton	done;
	// XM.1 item 12: A Select / B Back / D-pad Adjust - both sliders above
	// are D-pad-adjustable (Slider.cpp, Utils.h's IsLeftArrow/IsRightArrow
	// already recognize K_DPAD_LEFT/RIGHT directly).
	CMenuLegend legend;
#else
	CMenuPicButton	done;
#if LEGACY_VIEWSIZE
	CMenuSlider	screenSize;
#endif
	CMenuCheckBox	vbo;
	CMenuCheckBox	swwater;
	CMenuCheckBox	overbright;
	CMenuCheckBox	detailtex;
	CMenuCheckBox	hudscale;
	CMenuYesNoMessageBox	msgBox;
#endif // XASH_XBOX

	HIMAGE		hTestImage;
};

/*
=================
CMenuVidOptions::UpdateConfig
=================
*/
void CMenuVidOptions::UpdateConfig( void )
{
	float val1 = RemapVal( gammaIntensity.GetCurrentValue(), 0.0, 1.0, 1.8, 3.0 );
	float val2 = RemapVal( brightness.GetCurrentValue(), 0.0, 1.0, 0.0, 3.0 );
	EngFuncs::CvarSetValue( "gamma", val1 );
	EngFuncs::CvarSetValue( "brightness", val2 );
	EngFuncs::ProcessImage( hTestImage, val1, val2 );
}

void CMenuVidOptions::GetConfig( void )
{
	float val1 = EngFuncs::GetCvarFloat( "gamma" );
	float val2 = EngFuncs::GetCvarFloat( "brightness" );

	gammaIntensity.SetCurrentValue( RemapVal( val1, 1.8f, 3.0f, 0.0f, 1.0f ) );
	brightness.SetCurrentValue( RemapVal( val2, 0.0f, 3.0f, 0.0f, 1.0f ) );
	EngFuncs::ProcessImage( hTestImage, val1, val2 );

	gammaIntensity.SetOriginalValue( val1 );
	brightness.SetOriginalValue( val2 );
}

void CMenuVidOptions::SaveAndPopMenu( void )
{
#if !XASH_XBOX
#if LEGACY_VIEWSIZE
	screenSize.WriteCvar();
#endif
	detailtex.WriteCvar();
	vbo.WriteCvar();
	swwater.WriteCvar();
	overbright.WriteCvar();
	hudscale.WriteCvar();
#endif // !XASH_XBOX
	filtering.WriteCvar();
	// gamma and brightness is already written

	CMenuFramework::SaveAndPopMenu();
}

/*
=================
CMenuVidOptions::Ownerdraw
=================
*/
void CMenuVidOptions::CMenuVidPreview::Draw( )
{
	int		color = 0xFFFF0000; // 255, 0, 0, 255
	int		viewport[4];
	int		viewsize, size, sb_lines;

#if LEGACY_VIEWSIZE
	viewsize = EngFuncs::GetCvarFloat( "viewsize" );
#else
	viewsize = 120;
#endif

	if( viewsize >= 120 )
		sb_lines = 0;	// no status bar at all
	else if( viewsize >= 110 )
		sb_lines = 24;	// no inventory
	else sb_lines = 48;

	size = Q_min( viewsize, 100 );

	viewport[2] = m_scSize.w * size / 100;
	viewport[3] = m_scSize.h * size / 100;

	if( viewport[3] > m_scSize.h - sb_lines )
		viewport[3] = m_scSize.h - sb_lines;
	if( viewport[3] > m_scSize.h )
		viewport[3] = m_scSize.h;

	viewport[2] &= ~7;
	viewport[3] &= ~1;

	viewport[0] = (m_scSize.w - viewport[2]) / 2;
	viewport[1] = (m_scSize.h - sb_lines - viewport[3]) / 2;

	UI_DrawPic( m_scPos.x + viewport[0], m_scPos.y + viewport[1], viewport[2], viewport[3], uiColorWhite, szPic );
	UI_DrawRectangleExt( m_scPos, m_scSize, color, ((CMenuVidOptions*)Parent())->outlineWidth );
}

/*
=================
CMenuVidOptions::Init
=================
*/
#if XASH_XBOX
void CMenuVidOptions::_Init( void )
{
	// XM.1 item 7 (fork-plan.md): Brightness/Gamma keep their stock
	// sliders and preview image unchanged - both work identically on
	// Xbox, and gamma.bmp degrades clean on HL25 content the same way
	// every other WON-only bitmap in this fork already does (absent,
	// no error - checked directly, not assumed).
	//
	// Three stock rows are gone, not merely hidden, because none of them
	// mean what their label promises here: r_detailtextures/gl_vbo/
	// r_ripple/gl_overbright are ref_gl-only settings, and ref_gl is
	// never compiled or linked for Xbox at all (tools/xbox-link-check.sh
	// builds ref/soft exclusively) - r_refdll_loaded is always "soft" at
	// runtime, so stock's own Reload() already grays every one of these
	// permanently through its existing gl_active check, and a row that
	// can never be un-grayed is clutter, not a setting. "Texture
	// filtering" needed no Xbox-specific change at all for the same
	// reason: Reload()'s own soft_active branch already links it to
	// sw_texfilt whenever ref_gl isn't active, which is unconditionally
	// true on this console.
	//
	// "Screen size" (viewsize) and "HUD size" (hud_scale) are dropped
	// too, and this correction is the point of this note, not a footnote
	// to it: checked against their real client-side consumers rather
	// than the plan's own assumption. viewsize does not control
	// inventory or status-bar visibility in the real client at all - its
	// only real effect (SCR_TileClear, cl_scrn.c) is a Quake-era
	// letterboxed-viewport shrink, a legacy console aesthetic with no
	// couch-TV benefit on an already-640x480 screen; the "no inventory"/
	// "no status bar" bands this file's OWN disabled preview widget
	// hints at were never real client behaviour, only a hand-drawn
	// approximation inside that one thumbnail. hud_scale fares worse:
	// CL_GetScreenInfo (cl_dll/dll_int/cl_game.c) can only enlarge the
	// HUD when the reference width it is compared against exceeds the
	// real screen width, but Xbox's screen is a fixed 640
	// (XVideoSetMode(640,480,...), divergence #50) and
	// hud_scale_minimal_width defaults to exactly 640 too - so with that
	// default, NO value of hud_scale can ever enlarge anything on this
	// console; the canvas's "Large, for a TV" cannot be built by wiring
	// a UI control to this cvar alone. Making that actually work needs
	// hud_scale_minimal_width lowered as its own, separately-verified
	// engine change (a real divergence, not a UI hookup) - a tracked
	// follow-up, not guessed into this row.
	hTestImage = EngFuncs::PIC_Load( ART_GAMMA, PIC_KEEP_SOURCE | PIC_EXPAND_SOURCE );

	testImage.iFlags = QMF_INACTIVE;
	testImage.SetRect( 390, 225, 480, 450 );
	testImage.SetPicture( ART_GAMMA );

	heading.iFlags = QMF_INACTIVE|QMF_DROPSHADOW;
	heading.szName = L( "GameUI_Video" );
	heading.colorBase = uiColorHeading;
	heading.SetCharSize( QM_BIGFONT );
	heading.SetRect( 72, 200, 400, 32 );

	// Codex review round 1 (item 8/6's own lesson, applied not
	// re-discovered): CMenuSlider draws its label above its SetCoord y,
	// so the first slider reuses stock Audio.cpp's own proven-safe
	// y=280, clear of the heading above it.
	gammaIntensity.szName = L( "GameUI_Gamma" );
	gammaIntensity.SetCoord( 72, 280 );
	gammaIntensity.Setup( 0.0, 1.0, 0.025 );
	gammaIntensity.onChanged = VoidCb( &CMenuVidOptions::UpdateConfig );
	gammaIntensity.onCvarGet = VoidCb( &CMenuVidOptions::GetConfig );

	brightness.szName = L( "GameUI_Brightness" );
	brightness.SetCoord( 72, 340 );
	brightness.Setup( 0, 1.0, 0.025 );
	brightness.onChanged = VoidCb( &CMenuVidOptions::UpdateConfig );
	brightness.onCvarGet = VoidCb( &CMenuVidOptions::GetConfig );

	filtering.szName = L( "Texture filtering" );
	filtering.SetCoord( 72, 400 );

	videoOutput.iFlags = QMF_INACTIVE|QMF_DROPSHADOW;
	videoOutput.szName = videoOutputText;
	videoOutput.colorBase = uiColorHelp;
	videoOutput.SetCharSize( QM_SMALLFONT );
	videoOutput.SetRect( 72, 450, 400, 26 );
	videoOutputText[0] = '\0';

	done.SetNameAndStatus( L( "GameUI_OK" ), nullptr );
	done.SetPicture( PC_DONE );
	done.onReleased = VoidCb( &CMenuVidOptions::SaveAndPopMenu );
	done.iFlags |= QMF_NOTIFY;
	done.SetCoord( 72, 510 );

	legend.SetRealCoord( 72, 438 );
	legend.Add( LEGEND_A, L( "Select" ) );
	legend.Add( LEGEND_B, L( "Back" ) );
	legend.Add( LEGEND_DPAD, L( "Adjust" ) );

	AddItem( heading );
	AddItem( gammaIntensity );
	AddItem( brightness );
	AddItem( filtering );
	AddItem( videoOutput );
	AddItem( done );
	AddItem( testImage );
	AddItem( legend );

	gammaIntensity.LinkCvar( "gamma" );
	brightness.LinkCvar( "brightness" );
	// skip filtering.LinkCvar, different names in ref_gl and ref_soft -
	// Reload() links it to sw_texfilt every time, ref_gl never being
	// active on Xbox
}
#else
void CMenuVidOptions::_Init( void )
{
	hTestImage = EngFuncs::PIC_Load( ART_GAMMA, PIC_KEEP_SOURCE | PIC_EXPAND_SOURCE );

	banner.SetPicture( ART_BANNER );

	testImage.iFlags = QMF_INACTIVE;
	testImage.SetRect( 390, 225, 480, 450 );
	testImage.SetPicture( ART_GAMMA );

	int height = 280;

#if LEGACY_VIEWSIZE
	screenSize.szName = L( "Screen size" );
	screenSize.SetCoord( 72, height );
	screenSize.Setup( 30, 120, 10 );
	screenSize.onChanged = CMenuEditable::WriteCvarCb;

	height += 60;
#endif

	gammaIntensity.szName = L( "GameUI_Gamma" );
	gammaIntensity.SetCoord( 72, height );
	gammaIntensity.Setup( 0.0, 1.0, 0.025 );
	gammaIntensity.onChanged = VoidCb( &CMenuVidOptions::UpdateConfig );
	gammaIntensity.onCvarGet = VoidCb( &CMenuVidOptions::GetConfig );
	height += 60;

	brightness.SetCoord( 72, height );
	brightness.szName = L( "GameUI_Brightness" );
	brightness.Setup( 0, 1.0, 0.025 );
	brightness.onChanged = VoidCb( &CMenuVidOptions::UpdateConfig );
	brightness.onCvarGet = VoidCb( &CMenuVidOptions::GetConfig );
	height += 60;

	done.szName = L( "GameUI_OK" );
	done.SetCoord( 72, height );
	done.SetPicture( PC_DONE );
	done.onReleased = VoidCb( &CMenuVidOptions::SaveAndPopMenu );
	height += 60;

	detailtex.szName = L( "Detail textures" );
	detailtex.SetCoord( 72, height );
	height += 50;

	vbo.szName = L( "Use VBO" );
	vbo.SetCoord( 72, height );
	height += 50;

	swwater.szName = L( "Water ripples" );
	swwater.SetCoord( 72, height );
	height += 50;

	overbright.szName = L( "Overbrights" );
	overbright.SetCoord( 72, height );
	height += 50;

	filtering.szName = L( "Texture filtering" );
	filtering.SetCoord( 72, height );
	height += 50;

	hudscale.szName = L( "Auto scale HUD" );
	hudscale.SetCoord( 72, height );
	height += 50;

	msgBox.SetMessage( L( "^1WARNING: This is an experimental option and turning it on might break mods!^7\n\nReload the game or reconnect to the server to apply the settings."));
	msgBox.onNegative.pExtra = &hudscale;
	SET_EVENT_MULTI( msgBox.onNegative,
	{
		CMenuCheckBox *cb = (CMenuCheckBox *)pExtra;

		cb->bChecked = false;
		cb->SetCvarValue( 0.0f );
	});

	SET_EVENT_MULTI( hudscale.onChanged,
	{
		CMenuVidOptions *parent = (CMenuVidOptions *)pSelf->Parent();

		if( EngFuncs::ClientInGame( ))
		{
			// bring up warning message box
			// FIXME: try to save the game, apply the settings, and then load it back. This will be useful when changing resolution too.
			parent->msgBox.Show();
		}
	});

	SET_EVENT_MULTI( hudscale.onCvarWrite,
	{
		CMenuCheckBox *cb = (CMenuCheckBox *)pSelf;

		if( cb->bChecked )
		{
			// automatically scale HUD as if we have 1024x768 screen
			// (saving aspect ratio)
			// FIXME: allow configuring this value?
			EngFuncs::CvarSetValue( cb->CvarName(), 1024.0f );
		}
		else
		{
			EngFuncs::CvarSetValue( cb->CvarName(), 0.0f );
		}
	});

	SET_EVENT_MULTI( hudscale.onCvarGet,
	{
		CMenuCheckBox *cb = (CMenuCheckBox *)pSelf;

		cb->bChecked = EngFuncs::GetCvarFloat( cb->CvarName() ) >= 640.0f;
	});

	AddItem( banner );
	AddItem( done );
#if LEGACY_VIEWSIZE
	AddItem( screenSize );
#endif
	AddItem( gammaIntensity );
	AddItem( brightness );
	AddItem( detailtex );
	AddItem( vbo );
	AddItem( swwater );
	AddItem( overbright );
	AddItem( filtering );
	AddItem( hudscale );
	AddItem( testImage );

#if LEGACY_VIEWSIZE
	screenSize.LinkCvar( "viewsize" );
#endif

	gammaIntensity.LinkCvar( "gamma" );
	brightness.LinkCvar( "brightness" );

	detailtex.LinkCvar( "r_detailtextures" );
	swwater.LinkCvar( "r_ripple" );
	vbo.LinkCvar( "gl_vbo" );
	overbright.LinkCvar( "gl_overbright" );
	// skip filtering.LinkCvar, different names in ref_gl and ref_soft
	hudscale.LinkCvar( "hud_scale" );
}
#endif // XASH_XBOX

#if XASH_XBOX
void CMenuVidOptions::_VidInit()
{
	outlineWidth = 2;
	UI_ScaleCoords( NULL, NULL, &outlineWidth, NULL );

	// Read-only cvars (vid_common.c's VID_SetXboxVideoCvars), not a new
	// engine export - Codex review round 1 caught the export approach
	// extending a struct this codebase can't safely grow past its own
	// documented freeze point without a version check it doesn't have.
	// vid_width/vid_height already exist and already mirror the real,
	// X5-clamped 640x480.
	int width = (int)EngFuncs::GetCvarFloat( "vid_width" );
	int height = (int)EngFuncs::GetCvarFloat( "vid_height" );
	int refresh = (int)EngFuncs::GetCvarFloat( "vid_refresh" );
	const char *aspect = EngFuncs::GetCvarString( "vid_aspect" );

	snprintf( videoOutputText, sizeof( videoOutputText ), "%dx%d - %s - %d Hz", width, height, aspect, refresh );
}
#else
void CMenuVidOptions::_VidInit()
{
	outlineWidth = 2;
	UI_ScaleCoords( NULL, NULL, &outlineWidth, NULL );
}
#endif // XASH_XBOX

void CMenuVidOptions::Reload()
{
	bool gl_active = !strnicmp( EngFuncs::GetCvarString( "r_refdll_loaded" ), "gl", 2 );
	bool soft_active = !stricmp( EngFuncs::GetCvarString( "r_refdll_loaded" ), "soft" );

#if !XASH_XBOX
	detailtex.SetGrayed( !gl_active );
	detailtex.SetInactive( !gl_active );

	vbo.SetGrayed( !gl_active );
	vbo.SetInactive( !gl_active );

	if( gl_active )
	{
		if( EngFuncs::textfuncs.pfnIsCvarReadOnly( "gl_vbo" ) > 0 )
		{
			SET_EVENT_MULTI( vbo.onCvarChange,
			{
				CMenuCheckBox *cb = (CMenuCheckBox *)pSelf;
				cb->bChecked = false;
				UI_ShowMessageBox( L( "Not supported on your GPU" ));
			});

			vbo.onCvarWrite = CEventCallback::NoopCb;
		}
		else
		{
			vbo.onCvarWrite.Reset();
			vbo.onCvarChange.Reset();
		}
	}

	swwater.SetGrayed( !gl_active );
	swwater.SetInactive( !gl_active );

	overbright.SetGrayed( !gl_active );
	overbright.SetInactive( !gl_active );
#endif // !XASH_XBOX

	// Xbox needs this half unconditionally: ref_gl is never linked here
	// (r_refdll_loaded is always "soft"), so this always takes the
	// soft_active branch below, linking filtering to sw_texfilt - the
	// same runtime dispatch stock PC uses, unmodified.
	if( soft_active || gl_active )
	{
		filtering.SetGrayed( false );
		filtering.SetInactive( false );

		if( soft_active )
		{
			// no need to invert, 0 disables filtering
			filtering.LinkCvar( "sw_texfilt" );
		}
		else
		{
			SET_EVENT_MULTI( filtering.onCvarGet,
			{
				CMenuCheckBox *cb = (CMenuCheckBox *)pSelf;

				if( EngFuncs::GetCvarFloat( cb->CvarName( )))
					cb->bChecked = false;
				else cb->bChecked = true;
			});
			SET_EVENT_MULTI( filtering.onCvarWrite,
			{
				CMenuCheckBox *cb = (CMenuCheckBox *)pSelf;
				if( cb->bChecked )
					EngFuncs::CvarSetValue( cb->CvarName(), 0.0f );
				else EngFuncs::CvarSetValue( cb->CvarName(), 1.0f );
			});

			filtering.LinkCvar( "gl_texture_nearest" );
		}
	}
	else
	{
		filtering.SetGrayed( true );
		filtering.SetInactive( true );
	}

	CMenuFramework::Reload();
}

ADD_MENU( menu_vidoptions, CMenuVidOptions, UI_VidOptions_Menu );
