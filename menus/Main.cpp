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
#include "Action.h"
#include "Bitmap.h"
#include "PicButton.h"
#include "YesNoMessageBox.h"
#include "keydefs.h"
#include "MenuStrings.h"
#include "PlayerIntroduceDialog.h"
#include "gameinfo.h"
#include "AnimatedBanner.h"
#include "MovieBanner.h"

#define ART_MINIMIZE_N	"gfx/shell/min_n"
#define ART_MINIMIZE_F	"gfx/shell/min_f"
#define ART_MINIMIZE_D	"gfx/shell/min_d"
#define ART_CLOSEBTN_N	"gfx/shell/cls_n"
#define ART_CLOSEBTN_F	"gfx/shell/cls_f"
#define ART_CLOSEBTN_D	"gfx/shell/cls_d"

class CMenuMain: public CMenuFramework
{
public:
	CMenuMain() : CMenuFramework( "CMenuMain" ) { }

	bool KeyDown( int key ) override;

private:
	void _Init() override;
	void _VidInit( ) override;
	void Think() override;

	void VidInit(bool connected);

#if !XASH_XBOX
	void QuitDialogCb();
	void DisconnectCb();
	void DisconnectDialogCb();
#endif
	void HazardCourseDialogCb();
	void HazardCourseCb();

	CMenuAnimatedBanner animatedBanner;
	CMenuMovieBanner movieBanner;

#if !XASH_XBOX
	CMenuPicButton	console;
	CMenuPicButton	disconnect;
#endif
	CMenuPicButton	resumeGame;
	CMenuPicButton	newGame;
	CMenuPicButton	hazardCourse;
	CMenuPicButton	configuration;
	// XM.1 (fork-plan.md): the WON single saveRestore button ("Save\Load
	// Game" when connected, "Load Game" otherwise) is split into its own
	// Load Game and Save Game rows - Save Game only ever made sense
	// in-game, and the design gives it a row of its own instead of
	// folding it into Load Game's label.
	CMenuPicButton	loadGame;
	CMenuPicButton	saveGame;
#if !XASH_XBOX
	CMenuPicButton	multiPlayer;
	CMenuPicButton	customGame;
	CMenuPicButton	previews;
	CMenuPicButton	quit;
#endif

	// buttons on top right. Maybe should be drawn if fullscreen == 1?
	CMenuBitmap	minimizeBtn;
	CMenuBitmap	quitButton;

	// quit dialog / hazard-course restart confirmation
	CMenuYesNoMessageBox dialog;

	bool bTrainMap;
#if !XASH_XBOX
	bool bCustomGame;
#endif
};

#if !XASH_XBOX
void CMenuMain::QuitDialogCb()
{
	if( CL_IsActive() && EngFuncs::GetCvarFloat( "host_serverstate" ) && EngFuncs::GetCvarFloat( "maxplayers" ) == 1.0f )
		dialog.SetMessage( L( "StringsList_235" ) );
	else
		dialog.SetMessage( L( "GameUI_QuitConfirmationText" ) );

	dialog.onPositive.SetCommand( false, "quit\n" );
	dialog.Show();
}

void CMenuMain::DisconnectCb()
{
	EngFuncs::ClientCmd( false, "disconnect\n" );
	VidInit( false );
	CalcPosition();
	CalcSizes();
	VidInitItems();
}

void CMenuMain::DisconnectDialogCb()
{
	dialog.onPositive = VoidCb( &CMenuMain::DisconnectCb );
	dialog.SetMessage( L( "Really disconnect?" ) );
	dialog.Show();
}
#endif // !XASH_XBOX

void CMenuMain::HazardCourseDialogCb()
{
	dialog.onPositive = VoidCb( &CMenuMain::HazardCourseCb );;
	dialog.SetMessage( L( "StringsList_234" ) );
	dialog.Show();
}

/*
=================
CMenuMain::Key
=================
*/
bool CMenuMain::KeyDown( int key )
{
	if( UI::Key::IsEscape( key ) )
	{
		if ( CL_IsActive( ))
		{
			if( !dialog.IsVisible() )
				UI_CloseMenu();
		}
#if !XASH_XBOX
		else
		{
			QuitDialogCb( );
		}
#endif
		// XM.1: no Quit on Xbox, and Main has no menu above it to escape
		// to - B at Main is a no-op, matching a console title's own Main
		// menu (there is nothing to back out of).
		return true;
	}
	return CMenuFramework::KeyDown( key );
}

/*
=================
UI_Main_HazardCourse
=================
*/
void CMenuMain::HazardCourseCb()
{
	if( EngFuncs::GetCvarFloat( "host_serverstate" ) && EngFuncs::GetCvarFloat( "maxplayers" ) > 1 )
		EngFuncs::HostEndGame( "end of the game" );

	EngFuncs::CvarSetValue( "skill", 1.0f );
	EngFuncs::CvarSetValue( "deathmatch", 0.0f );
	EngFuncs::CvarSetValue( "teamplay", 0.0f );
	EngFuncs::CvarSetValue( "pausable", 1.0f ); // singleplayer is always allowing pause
	EngFuncs::CvarSetValue( "coop", 0.0f );
	EngFuncs::CvarSetValue( "maxplayers", 1.0f ); // singleplayer

	EngFuncs::PlayBackgroundTrack( NULL, NULL );

	EngFuncs::ClientCmd( false, "hazardcourse\n" );
}

void CMenuMain::_Init( void )
{
	if( gMenu.m_gameinfo.trainmap[0] && stricmp( gMenu.m_gameinfo.trainmap, gMenu.m_gameinfo.startmap ) != 0 )
		bTrainMap = true;
	else bTrainMap = false;

#if !XASH_XBOX
	if( EngFuncs::GetCvarFloat( "host_allow_changegame" ))
		bCustomGame = true;
	else bCustomGame = false;

	// console
	console.SetNameAndStatus( L( "GameUI_Console" ), L( "Show console" ) );
	console.iFlags |= QMF_NOTIFY;
	console.SetPicture( PC_CONSOLE );
	console.SetVisibility( gpGlobals->developer );
	SET_EVENT_MULTI( console.onReleased,
	{
		UI_SetActiveMenu( false );
		EngFuncs::KEY_SetDest( KEY_CONSOLE );
	});
#endif // !XASH_XBOX

	resumeGame.SetNameAndStatus( L( "GameUI_GameMenu_ResumeGame" ), L( "StringsList_188" ) );
	resumeGame.SetPicture( PC_RESUME_GAME );
	resumeGame.iFlags |= QMF_NOTIFY;
	resumeGame.onReleased = UI_CloseMenu;

#if !XASH_XBOX
	disconnect.SetNameAndStatus( L( "GameUI_GameMenu_Disconnect" ), L( "Disconnect from server" ) );
	disconnect.SetPicture( PC_DISCONNECT );
	disconnect.iFlags |= QMF_NOTIFY;
	disconnect.onReleased = VoidCb( &CMenuMain::DisconnectDialogCb );
#endif // !XASH_XBOX

	newGame.SetNameAndStatus( L( "GameUI_NewGame" ), L( "StringsList_189" ) );
	newGame.SetPicture( PC_NEW_GAME );
	newGame.iFlags |= QMF_NOTIFY;
	newGame.onReleased = UI_NewGame_Menu;

	hazardCourse.SetNameAndStatus( L( "GameUI_TrainingRoom" ), L( "StringsList_190" ) );
	hazardCourse.SetPicture( PC_HAZARD_COURSE );
	hazardCourse.iFlags |= QMF_NOTIFY;
	hazardCourse.onReleasedClActive = VoidCb( &CMenuMain::HazardCourseDialogCb );
	hazardCourse.onReleased = VoidCb( &CMenuMain::HazardCourseCb );

#if !XASH_XBOX
	multiPlayer.SetNameAndStatus( L( "GameUI_Multiplayer" ), L( "StringsList_198" ) );
	multiPlayer.SetPicture( PC_MULTIPLAYER );
	multiPlayer.iFlags |= QMF_NOTIFY;
	multiPlayer.onReleased = UI_MultiPlayer_Menu;
#endif // !XASH_XBOX

	configuration.SetNameAndStatus( L( "GameUI_Options" ), L( "StringsList_193" ) );
	configuration.SetPicture( PC_CONFIG );
	configuration.iFlags |= QMF_NOTIFY;
	configuration.onReleased = UI_Options_Menu;

	// XM.1: Load Game and Save Game, always their own rows now - reuses
	// SaveLoad.cpp's own standalone strings for Save Game (GameUI_LoadGame's
	// disconnected-state hint, StringsList_191, already fits Load Game
	// verbatim - it is what Main.cpp itself used for that exact case before
	// this split).
	loadGame.SetNameAndStatus( L( "GameUI_LoadGame" ), L( "StringsList_191" ) );
	loadGame.SetPicture( PC_LOAD_GAME );
	loadGame.iFlags |= QMF_NOTIFY;
	loadGame.onReleased = UI_LoadGame_Menu;

	saveGame.SetNameAndStatus( L( "GameUI_SaveGame" ), L( "GameUI_SaveGameHelp" ) );
	saveGame.SetPicture( PC_SAVE_GAME );
	saveGame.iFlags |= QMF_NOTIFY;
	saveGame.onReleased = UI_SaveGame_Menu;

#if !XASH_XBOX
	customGame.SetNameAndStatus( L( "GameUI_ChangeGame" ), L( "StringsList_530" ) );
	customGame.SetPicture( PC_CUSTOM_GAME );
	customGame.iFlags |= QMF_NOTIFY;
	customGame.onReleased = UI_CustomGame_Menu;

	previews.SetNameAndStatus( L( "Previews" ), L( "StringsList_400" ) );
	previews.SetPicture( PC_PREVIEWS );
	previews.iFlags |= QMF_NOTIFY;
	SET_EVENT( previews.onReleased, EngFuncs::ShellExecute( MenuStrings[ IDS_MEDIA_PREVIEWURL ], NULL, false ) );

	quit.SetNameAndStatus( L( "GameUI_GameMenu_Quit" ), L( "GameUI_QuitConfirmationText" ) );
	quit.SetPicture( PC_QUIT );
	quit.iFlags |= QMF_NOTIFY;
	quit.onReleased = VoidCb( &CMenuMain::QuitDialogCb );

	quitButton.onReleased = VoidCb( &CMenuMain::QuitDialogCb );
#endif // !XASH_XBOX

	quitButton.SetPicture( ART_CLOSEBTN_N, ART_CLOSEBTN_F, ART_CLOSEBTN_D );
	quitButton.iFlags = QMF_MOUSEONLY;
	quitButton.eFocusAnimation = QM_HIGHLIGHTIFFOCUS;

	minimizeBtn.SetPicture( ART_MINIMIZE_N, ART_MINIMIZE_F, ART_MINIMIZE_D );
	minimizeBtn.iFlags = QMF_MOUSEONLY;
	minimizeBtn.eFocusAnimation = QM_HIGHLIGHTIFFOCUS;
	minimizeBtn.onReleased.SetCommand( false, "minimize\n" );

	if ( gMenu.m_gameinfo.gamemode == GAME_MULTIPLAYER_ONLY || gMenu.m_gameinfo.startmap[0] == 0 )
		newGame.SetGrayed( true );

#if !XASH_XBOX
	if ( gMenu.m_gameinfo.gamemode == GAME_SINGLEPLAYER_ONLY )
		multiPlayer.SetGrayed( true );
#endif // !XASH_XBOX

	if ( gMenu.m_gameinfo.gamemode == GAME_MULTIPLAYER_ONLY )
	{
		loadGame.SetGrayed( true );
		saveGame.SetGrayed( true );
		hazardCourse.SetGrayed( true );
	}

#if !XASH_XBOX
	// too short execute string - not a real command
	if( strlen( MenuStrings[IDS_MEDIA_PREVIEWURL] ) <= 3 )
	{
		previews.SetGrayed( true );
	}
#endif // !XASH_XBOX

	// server.dll needs for reading savefiles or startup newgame
	if( !EngFuncs::CheckGameDll( ))
	{
		loadGame.SetGrayed( true );
		saveGame.SetGrayed( true );
		hazardCourse.SetGrayed( true );
		newGame.SetGrayed( true );
	}

	if( FBitSet( gMenu.m_gameinfo.flags, GFL_ANIMATED_TITLE ))
	{
		if( animatedBanner.TryLoad())
			AddItem( animatedBanner );
	}
	else
	{
		AddItem( movieBanner );
	}

	dialog.Link( this );

	AddItem( banner );
#if !XASH_XBOX
	AddItem( console );
	AddItem( disconnect );
#endif // !XASH_XBOX
	AddItem( resumeGame );
	AddItem( newGame );

	if ( bTrainMap )
		AddItem( hazardCourse );

	// XM.1 (Codex review): registration order IS focus order
	// (ItemsHolder.cpp), independent of on-screen position - it has to
	// match each platform's own VidInit() stacking, not just this
	// platform's, or Up/Down jumps around the retail PC layout that
	// keeps Configuration above Load Game. saveGame stays registered
	// (never visible) on non-Xbox too - an invisible item is skipped by
	// navigation, so its exact slot there doesn't matter.
#if XASH_XBOX
	AddItem( loadGame );
	AddItem( saveGame );
	AddItem( configuration );
#else
	AddItem( configuration );
	AddItem( loadGame );
	AddItem( saveGame );
#endif // XASH_XBOX

#if !XASH_XBOX
	AddItem( multiPlayer );

	if ( bCustomGame )
		AddItem( customGame );

	AddItem( previews );
	AddItem( quit );
#endif // !XASH_XBOX

	AddItem( minimizeBtn );
	AddItem( quitButton );
}

/*
=================
UI_Main_Init
=================
*/
void CMenuMain::VidInit( bool connected )
{
	int hoffset = ( 70 / 640.0 ) * 1024.0;

#if XASH_XBOX
	// XM.1 (fork-plan.md): Options anchors the bottom of the flattened
	// menu at the same fixed point retail's Previews used to - same
	// on-screen real estate as the WON layout, top-to-bottom now Resume /
	// New Game / Training Room / Load Game / Save Game / Options.
	int configuration_voffset = ( 404 / 480.0 ) * 768.0;

	// no visible console button gap
	int ygap = (( 404 - 373 ) / 480.0 ) * 768.0;

	// statically positioned items
	minimizeBtn.SetRect( uiStatic.width - 72, 13, 32, 32 );
	quitButton.SetRect( uiStatic.width - 36, 13, 32, 32 );

	configuration.SetCoord( hoffset, configuration_voffset );

	int yoffset = configuration_voffset - ygap;

	bool single = gpGlobals->maxClients < 2;

	// Save Game's row only exists in-game (single-player, same as the
	// SetVisibility below) - skip its ygap slot entirely when hidden, the
	// same way resumeGame/hazardCourse do, or Load Game stacks one row
	// too high and leaves a gap above Configuration.
	if( connected && single )
	{
		saveGame.SetCoord( hoffset, yoffset );
		yoffset -= ygap;
	}

	loadGame.SetCoord( hoffset, yoffset );
	yoffset -= ygap;

	if( bTrainMap )
	{
		hazardCourse.SetCoord( hoffset, yoffset );
		yoffset -= ygap;
	}

	newGame.SetCoord( hoffset, yoffset );
	yoffset -= ygap;

	if( connected )
	{
		resumeGame.SetCoord( hoffset, yoffset );
		yoffset -= ygap;
	}

	// now figure out what's visible
	resumeGame.SetVisibility( connected );
	saveGame.SetVisibility( connected && single );
#else
	// in original menu Previews is located at specific point
	int previews_voffset = ( 404 / 480.0 ) * 768.0;

	// no visible console button gap
	int ygap = (( 404 - 373 ) / 480.0 ) * 768.0;

	// statically positioned items
	minimizeBtn.SetRect( uiStatic.width - 72, 13, 32, 32 );
	quitButton.SetRect( uiStatic.width - 36, 13, 32, 32 );

	previews.SetCoord( hoffset, previews_voffset );
	quit.SetCoord( hoffset, previews_voffset + ygap );

	// let's start calculating positions
	int yoffset = previews_voffset - ygap;

	if( bCustomGame )
	{
		customGame.SetCoord( hoffset, yoffset );
		yoffset -= ygap;
	}

	multiPlayer.SetCoord( hoffset, yoffset );
	yoffset -= ygap;

	bool single = gpGlobals->maxClients < 2;

	loadGame.SetCoord( hoffset, yoffset );
	yoffset -= ygap;

	configuration.SetCoord( hoffset, yoffset );
	yoffset -= ygap;

	if( bTrainMap )
	{
		hazardCourse.SetCoord( hoffset, yoffset );
		yoffset -= ygap;
	}

	newGame.SetCoord( hoffset, yoffset );
	yoffset -= ygap;

	if( connected )
	{
		resumeGame.SetCoord( hoffset, yoffset );
		yoffset -= ygap;

		if( !single )
		{
			disconnect.SetCoord( hoffset, yoffset );
			yoffset -= ygap;
		}
	}

	console.SetCoord( hoffset, yoffset );
	yoffset -= ygap;

	// now figure out what's visible
	resumeGame.SetVisibility( connected );
	disconnect.SetVisibility( connected && !single );
	saveGame.SetVisibility( false );

	if( connected && single )
	{
		loadGame.SetNameAndStatus( L( "Save\\Load Game" ), L( "StringsList_192" ) );
		loadGame.SetPicture( PC_SAVE_LOAD_GAME );
		loadGame.onReleased = UI_SaveLoad_Menu;
	}
	else
	{
		loadGame.SetNameAndStatus( L( "GameUI_LoadGame" ), L( "StringsList_191" ) );
		loadGame.SetPicture( PC_LOAD_GAME );
		loadGame.onReleased = UI_LoadGame_Menu;
	}
#endif // XASH_XBOX
}

void CMenuMain::_VidInit()
{
	VidInit( CL_IsActive() );
}

void CMenuMain::Think()
{
#if !XASH_XBOX
	if( gpGlobals->developer )
	{
		if( !console.IsVisible( ))
			console.Show();
	}
	else
	{
		if( console.IsVisible( ))
			console.Hide();
	}
#endif // !XASH_XBOX

	CMenuFramework::Think();
}

ADD_MENU( menu_main, CMenuMain, UI_Main_Menu );
