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
#include "keydefs.h"
#include "PicButton.h"
#include "Action.h"
#include "YesNoMessageBox.h"

// XM.1 item 10 (fork-plan.md): a new screen, not a reshape of an existing
// one - there is no stock "in-game pause menu" file in mainui to diverge
// from (GoldSrc/Xash open Main itself in-game, PC_RESUME_GAME/PC_SAVE_GAME
// rows and all). The whole rest of the file is Xbox-only rather than
// carrying an `#else` half, since there is no PC behaviour this fork is
// changing - but the guard has to come AFTER these includes, not before
// them: XASH_XBOX is defined via build.h, reached transitively through
// Framework.h, not a compiler -D flag available before any #include runs.
// Checking it first left the whole file seeing an undefined macro (0 in
// an #if) and compiling to an empty 544-byte object - caught by the
// link, not the compile, since an empty translation unit is not an error.
#if XASH_XBOX

class CMenuPause : public CMenuFramework
{
private:
	void _Init( void ) override;
	void _VidInit( void ) override;
	void Hide( void ) override;
	bool KeyDown( int key ) override;

	void QuitConfirmedCb( void );
	void QuitDialogCb( void );

	// Codex review (round 1, finding 1): BACK is independently bound to
	// raw "pause" (engine/client/input/in_keys.c) outside this menu, and
	// SV_Pause_f (engine/server/sv_client.c) is a toggle, not a settable
	// flag - so a player can already be paused (or not) by the time this
	// screen opens or closes. Tracks whether THIS screen's own _VidInit
	// was the one that flipped pause on, so Hide() only flips it back off
	// when this screen caused it - never blindly toggling on either edge.
	bool m_bDidPause;

public:
	CMenuPause() : CMenuFramework( "CMenuPause" ), m_bDidPause( false ) { }

	// Codex review (round 2, finding 1): CMenuLoadGame::SaveGame()/
	// ::LoadGame() (LoadGame.cpp) both finish by calling UI_CloseMenu(),
	// which clears the ENTIRE window stack (CWindowStack::Clean(),
	// WindowSystem.h) without calling Hide() on anything still in it -
	// harmless for every other screen, since no other Hide() override
	// does real work, but this screen's own unpause/ui_renderworld reset
	// live there. Public so UI_Pause_CheckClosed() (below, polled once
	// per frame from BaseMenu.cpp's own per-frame hook) can finish this
	// screen's cleanup when it disappears that way instead of through
	// its own Hide(). Idempotent - harmless to call again after a normal
	// Hide() already ran (m_bDidPause already false by then).
	void ClosedExternally( void )
	{
		if( m_bDidPause )
		{
			EngFuncs::ClientCmd( false, "pause\n" );
			m_bDidPause = false;
		}

		EngFuncs::CvarSetValue( "ui_renderworld", 0.0f );
	}

	CMenuAction heading;
	// A read-only annotation, same idiom as Display's own "Video output"
	// row (divergence #71) - real state, never a literal. Difficulty is
	// real (the "skill" cvar, the same one NewGame.cpp's own Easy/Medium/
	// Difficult rows write - GameUI_Easy/Medium/Difficult are real,
	// already-resolved dictionary keys).
	//
	// Chapter title, corrected in Codex review: an earlier version of
	// this claimed classic Half-Life has no chapter-title concept at all
	// because only deps/hlsdk/cl_dll/ was grepped - the client DLL, which
	// never sees it. It is real: worldspawn's own "chaptertitle" key
	// (deps/hlsdk/dlls/world.cpp:663-674) fires a one-time client message
	// at map start, which is not something to read back later - but the
	// engine ALSO carries a complete mapname-to-title-token table for
	// every retail HL/OpFor/BShift map (engine/server/sv_save.c's own
	// gTitleComments[], already used to build save-file comments,
	// LoadGame.cpp:286's own `L(s)` already resolving its tokens) that
	// does not depend on a given map re-setting chaptertitle - most
	// mid-chapter maps do not. `SV_ResolveLevelTitle()`, pulled out of
	// that same file's SaveBuildComment (a pure refactor, save comments
	// unchanged), populates a new read-only `sv_leveltitle` cvar once a
	// map activates (SV_ActivateServer, sv_init.c) - the same zero-new-
	// ABI-surface shape as Display's own vid_refresh/vid_aspect, Xbox-
	// only end to end since this cvar has exactly one reader.
	CMenuAction chapterTitle;
	char chapterTitleText[64];

	CMenuAction difficulty;
	char difficultyText[32];

	CMenuPicButton resumeGame;
	CMenuPicButton saveGame;
	CMenuPicButton loadGame;
	CMenuPicButton options;
	CMenuPicButton quit;
	CMenuYesNoMessageBox quitConfirm;
};

/*
=================
CMenuPause::QuitConfirmedCb

"Quit to Main Menu" leaves the map - EngFuncs::ClientCmd's own
"disconnect" (the same command Main.cpp's own, non-Xbox DisconnectCb
already uses). Unlike that PC path, this screen also has to navigate
itself away afterward: disconnecting does not pop any menu or open a
different one on its own (UI-level navigation is always the caller's
job in this menu system, the same as every "onReleased = UI_X_Menu"
call elsewhere), and leaving Pause showing over a now-disconnected game
would be stale. Hide() (below) already handles unpausing and turning
ui_renderworld back off for every exit path including this one, so this
only needs to disconnect and then hand off to Main.
=================
*/
void CMenuPause::QuitConfirmedCb( void )
{
	EngFuncs::ClientCmd( false, "disconnect\n" );
	Hide();
	UI_Main_Menu();
}

/*
=================
CMenuPause::QuitDialogCb

Same idiom Main.cpp's own (non-Xbox) DisconnectDialogCb/HazardCourseCb
already use for a confirm-before-acting button: wire the dialog's own
onPositive to the real action, set its message, show it. Needed because
a member function pointer can only be taken through the class that owns
it, not through a member of that class (`&CMenuPause::quitConfirm.Show`
does not compile) - this one extra method is that indirection.
=================
*/
void CMenuPause::QuitDialogCb( void )
{
	quitConfirm.onPositive = VoidCb( &CMenuPause::QuitConfirmedCb );
	quitConfirm.Show();
}

/*
=================
CMenuPause::Hide

The single place every exit path goes through - the Resume button, B/
Escape (CMenuBaseWindow::KeyDown's own Escape branch calls Hide()
directly, never a separate handler, the same idiom Editable.cpp's own
discard-on-Escape relies on elsewhere in this fork), START (this
screen's own KeyDown override, below), and Quit's own QuitConfirmedCb
above.

"pause" is only re-issued here if _VidInit() (below) recorded that THIS
screen was the one that paused - Codex review (round 1, finding 1)
found that blindly toggling on every exit path corrupts state whenever
the player was already paused by some other means (BACK is bound to
raw "pause" independently of this menu, engine/client/input/in_keys.c)
before this screen ever opened.

ui_renderworld back to 0 restores the flat, opaque background the rest
of the in-game menu tree still expects (Options/Audio/Game reached from
here fall back to it - see the plan's own note on this row) - matching
its value everywhere else in this menu system rather than leaving Pause
as the one screen that changed a global rendering switch and never put
it back.
=================
*/
void CMenuPause::Hide( void )
{
	ClosedExternally();

	CMenuFramework::Hide();
}

/*
=================
CMenuPause::KeyDown

Codex review (round 1, finding 2): once a menu is active, raw input
goes straight to UI and never reaches gameplay bindings
(engine/client/input/in_keys.c), and the inherited escape predicate
(UI::Key::IsEscape, Utils.h) only recognizes Escape/B - Xbox's START is
bound to "cancelselect" (in_keys.c), not pause, so it would otherwise
do nothing here. Routes START through the same Hide() every other close
path already uses; anything else falls through to the base class
unchanged.
=================
*/
bool CMenuPause::KeyDown( int key )
{
	if( key == K_START_BUTTON )
	{
		Hide();
		return true;
	}

	return CMenuFramework::KeyDown( key );
}

/*
=================
CMenuPause::Init
=================
*/
void CMenuPause::_Init( void )
{
	heading.iFlags = QMF_INACTIVE|QMF_DROPSHADOW;
	// No WON StringsList_* index holds "Paused" - checked, not guessed
	// from index proximity: 234 is HazardCourseDialogCb's own confirm
	// text, and 235 is one of the compiled-in table's own empty slots
	// (MenuStrings.cpp:53, EMPTY_STRINGS_5). L() on the literal is item
	// 13's own established idiom instead (the key IS the English string,
	// translatable via mainui_<lang>.txt, English if nobody has).
	heading.szName = L( "Paused" );
	heading.colorBase = uiColorHeading;
	heading.SetCharSize( QM_BIGFONT );
	heading.SetRect( 72, 200, 400, 32 );

	chapterTitle.iFlags = QMF_INACTIVE|QMF_DROPSHADOW;
	chapterTitle.szName = chapterTitleText;
	chapterTitle.colorBase = uiColorHelp;
	chapterTitle.SetCharSize( QM_SMALLFONT );
	chapterTitle.SetRect( 72, 240, 400, 26 );
	chapterTitleText[0] = '\0';

	difficulty.iFlags = QMF_INACTIVE|QMF_DROPSHADOW;
	difficulty.szName = difficultyText;
	difficulty.colorBase = uiColorHelp;
	difficulty.SetCharSize( QM_SMALLFONT );
	difficulty.SetRect( 72, 266, 400, 26 );
	difficultyText[0] = '\0';

	resumeGame.SetNameAndStatus( L( "GameUI_GameMenu_ResumeGame" ), nullptr );
	resumeGame.onReleased = VoidCb( &CMenuPause::Hide );
	resumeGame.iFlags |= QMF_NOTIFY;
	resumeGame.SetCoord( 72, 310 );

	saveGame.SetNameAndStatus( L( "GameUI_SaveGame" ), nullptr );
	saveGame.onReleased = UI_SaveGame_Menu;
	saveGame.iFlags |= QMF_NOTIFY;
	saveGame.SetCoord( 72, 360 );

	loadGame.SetNameAndStatus( L( "GameUI_LoadGame" ), nullptr );
	loadGame.onReleased = UI_LoadGame_Menu;
	loadGame.iFlags |= QMF_NOTIFY;
	loadGame.SetCoord( 72, 410 );

	options.SetNameAndStatus( L( "GameUI_Options" ), nullptr );
	options.onReleased = UI_Options_Menu;
	options.iFlags |= QMF_NOTIFY;
	options.SetCoord( 72, 460 );

	quitConfirm.SetMessage( L( "GameUI_QuitConfirmationText" ) );
	quitConfirm.Link( this );

	quit.SetNameAndStatus( L( "GameUI_GameMenu_Quit" ), nullptr );
	quit.onReleased = VoidCb( &CMenuPause::QuitDialogCb );
	quit.iFlags |= QMF_NOTIFY;
	quit.SetCoord( 72, 510 );

	AddItem( heading );
	AddItem( chapterTitle );
	AddItem( difficulty );
	AddItem( resumeGame );
	AddItem( saveGame );
	AddItem( loadGame );
	AddItem( options );
	AddItem( quit );
}

void CMenuPause::_VidInit( void )
{
	// Real state, computed here rather than cached, matching Display's
	// own reasoning: skill is a user-changeable cvar (the same values
	// NewGame.cpp's own Easy/1, Medium/2, Difficult/3 write), and
	// BaseWindow.cpp's Show() runs _Init()/_VidInit()/Reload() on every
	// visit to this screen, not just once.
	int skill = (int)EngFuncs::GetCvarFloat( "skill" );
	const char *label;

	switch( skill )
	{
	case 1: label = "GameUI_Easy"; break;
	case 2: label = "GameUI_Medium"; break;
	// "GameUI_Hard", not "GameUI_Difficult" - checked against the real
	// dictionary rather than guessed from NewGame.cpp's own row label
	// text ("Difficult"): NewGame.cpp:124 links this exact key for its
	// own third skill button, so this matches the one other place in
	// the menu that names this same skill level.
	case 3: label = "GameUI_Hard"; break;
	default: label = ""; break;
	}

	Q_strncpy( difficultyText, L( label ), sizeof( difficultyText ) );

	// sv_leveltitle (SV_UpdateLevelTitleCvar, sv_init.c/sv_save.c) - a
	// "#TOKEN"-style localization key for retail maps, or plain text for
	// anything else (a worldspawn message, or the raw mapname as a last
	// resort) - both safe to hand to L() unchanged, the same as
	// LoadGame.cpp's own already-established use of it on a save
	// comment's own title piece.
	Q_strncpy( chapterTitleText, L( EngFuncs::GetCvarString( "sv_leveltitle" ) ), sizeof( chapterTitleText ) );

	// The dimmed live scene: ui_renderworld is the single flag
	// V_RenderView checks (engine/client/cl_view.c) to skip the 3D draw
	// while a menu is up - AnimatedBanner.cpp/MovieBanner.cpp already
	// flip the same cvar for their own reasons, this is not new engine
	// territory.
	EngFuncs::CvarSetValue( "ui_renderworld", 1.0f );

	// The pause itself: "pause" (SV_Pause_f, engine/server/sv_client.c)
	// is a toggle, not a settable flag, and BACK is independently bound
	// to it outside this menu (in_keys.c) - so only issue it if the game
	// is not already paused, and remember that this screen is the one
	// that caused it (cl_ispaused mirrors cl.paused, synced once per
	// frame in Host_ClientFrame, cl_main.c - a read-only cvar rather
	// than a new ui_enginefuncs_t export, same shape as sv_leveltitle
	// above). Hide() only unpauses when m_bDidPause is true, so a
	// pre-existing BACK-pause survives this screen opening and closing
	// on top of it.
	if( EngFuncs::GetCvarFloat( "cl_ispaused" ) == 0.0f )
	{
		EngFuncs::ClientCmd( false, "pause\n" );
		m_bDidPause = true;
	}
	else
	{
		m_bDidPause = false;
	}
}

ADD_MENU( menu_pause, CMenuPause, UI_Pause_Menu );

/*
=================
UI_Pause_CheckClosed

Codex review (round 2, finding 1) - the actual fix, ClosedExternally()
above is only the half of it this file owns. Polled once per frame from
BaseMenu.cpp's own per-frame hook (the same one that opens this
screen), independently of whatever closed it: LoadGame.cpp's own
SaveGame()/LoadGame() call UI_CloseMenu() on save/load completion, which
clears the ENTIRE window stack without calling Hide() on anything still
in it - if this screen was still open underneath (Save/Load reached
from here stay layered on top of it, exactly like Options does, so
Escape/B from either without saving correctly pops back to a still-
paused Pause), that cleanup would otherwise never run. IsVisible()
(CMenuBaseWindow, already public) is true only while this screen is
still on the stack, so a true-to-false edge means it left some way
other than its own Hide() - Hide() itself pops it off the stack via
CMenuFramework::Hide() before this could see a change, so the ordinary
close paths never reach ClosedExternally() twice.
=================
*/
void UI_Pause_CheckClosed( void )
{
	static bool s_bWasVisible = false;
	bool visible = menu_pause != NULL && menu_pause->IsVisible();

	if( s_bWasVisible && !visible )
		menu_pause->ClosedExternally();

	s_bWasVisible = visible;
}

#endif // XASH_XBOX
