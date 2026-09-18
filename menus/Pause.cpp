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

	void QuitConfirmedCb( void );
	void QuitDialogCb( void );

public:
	CMenuPause() : CMenuFramework( "CMenuPause" ) { }

	CMenuAction heading;
	// A read-only annotation, same idiom as Display's own "Video output"
	// row (divergence #71) - real state, never a literal. Difficulty is
	// real (the "skill" cvar, the same one NewGame.cpp's own Easy/Medium/
	// Difficult rows write - GameUI_Easy/Medium/Difficult are real,
	// already-resolved dictionary keys). "Chapter" from the canvas is
	// NOT built: classic Half-Life's SDK has no chapter-title concept at
	// all (grepped deps/hlsdk/cl_dll/ for "chapter" - nothing; that is an
	// HL2-era idea), so the console's own map name (`c1a0`, an internal
	// codename with no display-name database in this SDK) is all there
	// really is to show, and showing a codename in its place would be
	// cryptic rather than the polish the canvas asked for - "never guess
	// a constant" extends to never inventing a display database this
	// project has no source for either. A tracked follow-up if this SDK
	// ever grows one; not guessed here.
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
discard-on-Escape relies on elsewhere in this fork), and Quit's own
QuitConfirmedCb above. Toggling "pause" again here rather than in
Resume's own onReleased means every path unpauses exactly once, with no
risk of a button-specific handler being added later that forgets to.

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
	EngFuncs::ClientCmd( false, "pause\n" );
	EngFuncs::CvarSetValue( "ui_renderworld", 0.0f );

	CMenuFramework::Hide();
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

	difficulty.iFlags = QMF_INACTIVE|QMF_DROPSHADOW;
	difficulty.szName = difficultyText;
	difficulty.colorBase = uiColorHelp;
	difficulty.SetCharSize( QM_SMALLFONT );
	difficulty.SetRect( 72, 240, 400, 26 );
	difficultyText[0] = '\0';

	resumeGame.SetNameAndStatus( L( "GameUI_GameMenu_ResumeGame" ), nullptr );
	resumeGame.onReleased = VoidCb( &CMenuPause::Hide );
	resumeGame.iFlags |= QMF_NOTIFY;
	resumeGame.SetCoord( 72, 280 );

	saveGame.SetNameAndStatus( L( "GameUI_SaveGame" ), nullptr );
	saveGame.onReleased = UI_SaveGame_Menu;
	saveGame.iFlags |= QMF_NOTIFY;
	saveGame.SetCoord( 72, 330 );

	loadGame.SetNameAndStatus( L( "GameUI_LoadGame" ), nullptr );
	loadGame.onReleased = UI_LoadGame_Menu;
	loadGame.iFlags |= QMF_NOTIFY;
	loadGame.SetCoord( 72, 380 );

	options.SetNameAndStatus( L( "GameUI_Options" ), nullptr );
	options.onReleased = UI_Options_Menu;
	options.iFlags |= QMF_NOTIFY;
	options.SetCoord( 72, 430 );

	quitConfirm.SetMessage( L( "GameUI_QuitConfirmationText" ) );
	quitConfirm.Link( this );

	quit.SetNameAndStatus( L( "GameUI_GameMenu_Quit" ), nullptr );
	quit.onReleased = VoidCb( &CMenuPause::QuitDialogCb );
	quit.iFlags |= QMF_NOTIFY;
	quit.SetCoord( 72, 480 );

	AddItem( heading );
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

	// The pause itself and the dimmed live scene, both real hardware/
	// engine state this screen has to switch on when it opens - "pause"
	// is a real SV_TogglePause (engine/server/sv_client.c), reachable
	// via ClientCmd exactly like Audio.cpp's own "vibrate" elsewhere in
	// this fork; ui_renderworld is the single flag V_RenderView checks
	// (engine/client/cl_view.c) to skip the 3D draw while a menu is up -
	// AnimatedBanner.cpp/MovieBanner.cpp already flip the same cvar for
	// their own reasons, this is not new engine territory. Both verified
	// live before writing this screen, not assumed: memstats-adjacent
	// verification for a pause screen makes no sense, so this was
	// checked by reading SV_Pause_f/V_RenderView directly instead - see
	// the plan's own two "measure before writing" prerequisites for item
	// 10, both closed by that reading, neither by a boot test.
	EngFuncs::ClientCmd( false, "pause\n" );
	EngFuncs::CvarSetValue( "ui_renderworld", 1.0f );
}

ADD_MENU( menu_pause, CMenuPause, UI_Pause_Menu );

#endif // XASH_XBOX
