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
#include "YesNoMessageBox.h"
#include "keydefs.h"
#include "TabView.h"
#if XASH_XBOX
#include "Legend.h"
#endif

#define ART_BANNER	     	"gfx/shell/head_config"

class CMenuOptions: public CMenuFramework
{
private:
	void _Init( void ) override;

public:
	typedef CMenuFramework BaseClass;
	CMenuOptions() : CMenuFramework("CMenuOptions") { }

	// update dialog
	CMenuYesNoMessageBox msgBox;

#if XASH_XBOX
	// XM.1 item 12: A Select / B Back, the plan's own generic baseline -
	// this screen has no X/Y or D-pad-adjusted row.
	CMenuLegend legend;
#endif
};

/*
=================
CMenuOptions::Init
=================
*/
void CMenuOptions::_Init( void )
{
	banner.SetPicture( ART_BANNER );

	msgBox.SetMessage( L( "Check the Internet for updates?" ) );
	SET_EVENT( msgBox.onPositive, UI_OpenUpdatePage( false, true ) );

	msgBox.Link( this );

	AddItem( banner );
	AddButton( L( "Controls" ), L( "Change keyboard and mouse settings" ),
		PC_CONTROLS, UI_Controls_Menu, QMF_NOTIFY );
	AddButton( L( "GameUI_Audio" ), L( "Change sound volume and quality" ),
		PC_AUDIO, UI_Audio_Menu, QMF_NOTIFY );
#if XASH_XBOX
	// XM.1 item 7: points straight at VideoOptions.cpp now that it is
	// reshaped for Xbox, skipping Video.cpp's own two-button hub - that
	// hub only exists to choose between "Video options" (VideoOptions.cpp)
	// and "Video modes" (VideoModes.cpp), and VideoModes.cpp is hidden
	// entirely on Xbox (no resolution table, no windowed/borderless, no
	// r_refdll - none of it applies to a fixed 640x480 console), so the
	// hub would offer exactly one working choice. Status text updated to
	// match what the reshaped screen actually has now (brightness/gamma,
	// texture filtering, a read-only video-output row) - the old text
	// promised "screen size" and "video mode" controls item 7 found do
	// not do what they say on Xbox and dropped (fork-plan.md item 7's own
	// note).
	AddButton( L( "GameUI_Video" ), L( "Change brightness and gamma" ),
		PC_VIDEO, UI_VidOptions_Menu, QMF_NOTIFY );
#else
	AddButton( L( "GameUI_Video" ), L( "Change screen size, video mode and gamma" ),
		PC_VIDEO, UI_Video_Menu, QMF_NOTIFY );
#endif // XASH_XBOX
#if !XASH_XBOX
	AddButton( L( "Touch" ), L( "Change touch settings and buttons" ),
		PC_TOUCH, UI_Touch_Menu, QMF_NOTIFY, 't' );
#endif // !XASH_XBOX
	AddButton( L( "GameUI_Joystick" ), L( "Change gamepad axis and button settings" ),
		PC_GAMEPAD, UI_GamePad_Menu, QMF_NOTIFY, 'g' );
#if XASH_XBOX
	// XM.1 (fork-plan.md): "Controls" and "Video" stay pointed at their
	// stock screens for now - renaming them to "Controller"/"Display"
	// belongs to whichever change actually rebuilds those screens (items
	// 5 and 7), not this one; doing it here first would label a button
	// with a screen that does not exist yet. "Game" has no such
	// dependency - GameOptions.cpp is already reshaped for Xbox.
	//
	// Codex review, post-merge, round 2: no WON strip picture ever means
	// "Game", so this can't reuse any EDefaultBtns id - not PC_ADV_OPT
	// (ordinal 60, inside the strip's populated 0-61 range, so it drew
	// the "Adv. Options" bitmap CreateGame.cpp/PlayerSetup.cpp already
	// use it for), and not PC_ADV_OPT2 either (round 1's fix: ordinal 68,
	// unpopulated only because today's strip stops at 62 - a future or
	// modded strip reaching that far would recreate the same mismatch,
	// since Btns.cpp:59's pic_count is measured from the loaded bitmap,
	// not a fixed constant). AddButton()'s EDefaultBtns overload always
	// calls SetPicture(), which sets hPic whenever the strip covers that
	// id - there's no id that is guaranteed unpopulated forever.
	//
	// round 3: a plain class member built the same way but without ever
	// calling SetPicture() fixed the picture, but broke shutdown - it
	// never went into m_apBtns, so ~CMenuFramework() (Framework.cpp:31-38)
	// dereferenced a null slot there for it while destroying every OTHER
	// button it owns. This is that same construction (heap-allocated,
	// stored in m_apBtns, m_iBtnsNum incremented atomically with the
	// store - AddButton()'s own contract, Framework.cpp:106-126), with
	// the SetPicture() call simply left out so hPic/button_id stay at
	// PicButton()'s own constructor defaults (0/-1) and
	// PicButton.cpp:259's `if( hPic && ... )` always takes the text
	// branch, independent of any strip's size.
	{
		CMenuPicButton *game = new CMenuPicButton();
		game->SetNameAndStatus( L( "Game" ), L( "Auto-aim, crosshair, weapon switching" ) );
		game->onReleased = UI_GameOptions_Menu;
		game->iFlags |= QMF_NOTIFY;
		game->SetCoord( 72, 230 + m_iBtnsNum * 50 );
		AddItem( *game );
		m_apBtns[m_iBtnsNum++] = game;
	}
#else
	AddButton( L( "Update" ), L( "Check for updates" ),
		PC_UPDATE, msgBox.MakeOpenEvent(), QMF_NOTIFY );
#endif // XASH_XBOX
	AddButton( L( "Done" ), L( "Go back to the Main menu" ),
		PC_DONE, VoidCb( &CMenuOptions::Hide ), QMF_NOTIFY );

#if XASH_XBOX
	legend.SetRealCoord( 72, 438 );
	legend.Add( LEGEND_A, L( "Select" ) );
	legend.Add( LEGEND_B, L( "Back" ) );
	AddItem( legend );
#endif
}

ADD_MENU( menu_options, CMenuOptions, UI_Options_Menu );
