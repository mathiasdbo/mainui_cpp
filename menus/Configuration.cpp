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
	// XM.1 item 4: no WON strip picture actually means "Game" (see the
	// _Init() comment at the button's construction) - a plain member built
	// without ever calling SetPicture(), so hPic/button_id stay at their
	// PicButton() constructor defaults (0/-1) and PicButton.cpp:259's
	// `if( hPic && ... )` always takes the text branch, independent of how
	// many entries any current or future button strip bitmap supplies.
	CMenuPicButton game;
#endif // XASH_XBOX
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
	AddButton( L( "GameUI_Video" ), L( "Change screen size, video mode and gamma" ),
		PC_VIDEO, UI_Video_Menu, QMF_NOTIFY );
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
	// id - there's no id that is guaranteed unpopulated forever. The
	// `game` member below is built without calling SetPicture() at all,
	// so hPic stays 0 (the constructor default) regardless of strip size,
	// matching AddButton()'s own y-position formula (230 + m_iBtnsNum*50)
	// by hand since it never runs through AddButton() to get one.
	game.SetNameAndStatus( L( "Game" ), L( "Auto-aim, crosshair, weapon switching" ) );
	game.onReleased = UI_GameOptions_Menu;
	game.iFlags |= QMF_NOTIFY;
	game.SetCoord( 72, 230 + m_iBtnsNum * 50 );
	AddItem( game );
	// AddButton() below still auto-positions from m_iBtnsNum - bump it by
	// hand since `game` never went through AddButton() to do that itself,
	// or Done would land on the same y as Game just above it.
	m_iBtnsNum++;
#else
	AddButton( L( "Update" ), L( "Check for updates" ),
		PC_UPDATE, msgBox.MakeOpenEvent(), QMF_NOTIFY );
#endif // XASH_XBOX
	AddButton( L( "Done" ), L( "Go back to the Main menu" ),
		PC_DONE, VoidCb( &CMenuOptions::Hide ), QMF_NOTIFY );
}

ADD_MENU( menu_options, CMenuOptions, UI_Options_Menu );
