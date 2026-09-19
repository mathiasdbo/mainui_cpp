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
#include "Bitmap.h"
#include "PicButton.h"
#include "Action.h"
#include "YesNoMessageBox.h"
#include "MessageBox.h"
#include "Table.h"
#include "utlvector.h"
#include "KbActListModel.h"
#if XASH_XBOX
#include "Legend.h"
#include "ControllerSchemes.h"
#endif

#define ART_BANNER		"gfx/shell/head_controls"

class CMenuControls;

class CMenuKeysModel : public CMenuKbActListModel
{
public:
	CMenuKeysModel( CMenuControls *parent ) : CMenuKbActListModel( VIEW_BINDINGS ), parent( parent ) { }

	void Update() override;
	void OnActivateEntry( int line ) override;
	void OnDeleteEntry( int line ) override;

private:
	CMenuControls *parent;
};

class CMenuControls : public CMenuFramework
{
public:
	CMenuControls() : CMenuFramework("CMenuControls"), keysListModel( this ) { }

	void _Init();
	void _VidInit();
	void EnterGrabMode( void );
	void UnbindEntry( void );
#if XASH_XBOX
	bool KeyDown( int key ) override;
#endif // XASH_XBOX

	// state toggle by
	CMenuTable keysList;
	CMenuKeysModel keysListModel;

private:
	void UnbindCommand( const char *command );
#if !XASH_XBOX
	void ResetKeysList( void );
#endif // !XASH_XBOX
#if XASH_XBOX
	void ApplySchemeCb( void );
	void Hide( void ) override;
#else
	void Cancel( void )
	{
		EngFuncs::ClientCmd( true, "exec keyboard\n" );
		Hide();
	}
#endif // XASH_XBOX

	// redefine key wait dialog
	class CGrabKeyMessageBox : public CMenuMessageBox
	{
	public:
		bool KeyUp( int key ) override;
		bool KeyDown( int key ) override;
	} msgBox1; // small msgbox

#if !XASH_XBOX
	CMenuYesNoMessageBox msgBox2; // large msgbox
#endif // !XASH_XBOX

#if XASH_XBOX
	// XM.1 item 12: A Reassign / X Clear / Y Reset / B Back.
	CMenuLegend legend;
#endif // XASH_XBOX
};

void CMenuControls::UnbindCommand( const char *command )
{
	for( int i = 0; ; i++ )
	{
		const char *str = EngFuncs::KeynumToString( i );

		if( !strcmp( str, "<OUT OF RANGE>" ))
			break;

		const char *b = EngFuncs::KEY_GetBinding( i );
		if( !b )
			continue;

#if XASH_XBOX
		// Codex review round 1: the stock strncmp() below is a PREFIX
		// match, not an exact one - clearing "+attack" also matches any
		// key bound to "+attack2" (its own bound string starts with the
		// same seven characters), a real, if latent, bug carried over
		// verbatim from stock Controls.cpp. It was rarely reachable on PC
		// (nothing there normally binds both to related keys a player
		// would think to compare), but every scheme here binds BOTH
		// together by design (ControllerSchemes.cpp) and this screen's
		// own X (Clear) makes wiping one keystroke away - so clearing Fire
		// routinely wiped Secondary Fire too. Fixed with an exact,
		// case-insensitive match, the same comparison LookupBoundKeys
		// (KbActListModel.h) already uses. A genuine upstream-quality fix,
		// guarded here per this ledger's own policy since it has not been
		// submitted upstream yet.
		if( stricmp( b, command ))
			continue;
#else
		if( strncmp( b, command, strlen( command )))
			continue;
#endif // XASH_XBOX

		EngFuncs::KEY_SetBinding( i, "" );
	}
}

#if XASH_XBOX
// XM.1 item 5 (fork-plan.md): kb_act.lst is the full keyboard action list -
// most of it (movement, strafe, mouselook, voice chat, screenshots...) has
// no real gamepad-button equivalent a Customize row could usefully show.
// Filtered down to exactly the commands ControllerSchemes.cpp's own scheme
// tables actually bind - the REAL Standard/Southpaw/Legacy union (Codex
// review round 1: an earlier version of this list only covered Standard's
// own 15 commands, so a Legacy player - "pause"/"impulse 201"/"lastinv",
// Legacy's own Back/D-pad-up/D-pad-down - saw those rows hidden entirely
// while Standard-only rows like "save quick"/slot1-4 showed as unbound).
static const char *s_padActions[] =
{
	"+attack", "+attack2", "+jump", "+duck", "+use", "+reload",
	"invnext", "invprev", "impulse 100", "+speed",
	"slot1", "slot2", "slot3", "slot4", "save quick",
	"pause", "impulse 201", "lastinv",
};

static void FormatBoundKeyForRow( int key, char *out, size_t size )
{
	out[0] = '\0';
	if( key == -1 )
		return;

	const char *s = EngFuncs::KeynumToString( key );
	if( s )
		snprintf( out, size, "^3%s^7", s );
}
#endif // XASH_XBOX

void CMenuKeysModel::Update( void )
{
	CMenuKbActListModel::Update();

#if XASH_XBOX
	for( int i = entries.Count() - 1; i >= 0; i-- )
	{
		if( entries[i].bind[0] == '\0' )
		{
			entries.Remove( i ); // a separator/heading line
			continue;
		}

		bool keep = false;
		for( size_t j = 0; j < V_ARRAYSIZE( s_padActions ); j++ )
		{
			if( !strcmp( entries[i].bind, s_padActions[j] ))
			{
				keep = true;
				break;
			}
		}

		if( !keep )
			entries.Remove( i );
	}

	// Start's own row - not a kb_act.lst action at all ("cancelselect" is
	// this fork's own binding, engine/client/input/in_keys.c:115), added
	// the same way KbActListModel.h's own AddVirtualCommand() injects
	// touch-only rows for VIEW_PICKER. Shown so the player can see it is
	// bound, but EnterGrabMode()/UnbindEntry() below both refuse to touch
	// it - fork-plan.md's own explicit call, since Start is the only pad
	// route back to the pause menu from gameplay.
	entry_t startEntry = { 0 };
	Q_strncpy( startEntry.bind, "cancelselect", sizeof( startEntry.bind ));
	snprintf( startEntry.display, sizeof( startEntry.display ), "^6%s^7", L( "Pause Menu" ));

	int keys[2];
	LookupBoundKeys( startEntry.bind, keys );
	FormatBoundKeyForRow( keys[0], startEntry.first, sizeof( startEntry.first ));
	FormatBoundKeyForRow( keys[1], startEntry.second, sizeof( startEntry.second ));

	entries.AddToTail( startEntry );
#endif // XASH_XBOX
}

void CMenuKeysModel::OnActivateEntry(int line)
{
	parent->EnterGrabMode();
}

void CMenuKeysModel::OnDeleteEntry(int line)
{
	parent->UnbindEntry();
}

#if !XASH_XBOX
void CMenuControls::ResetKeysList( void )
{
	char *afile = (char *)EngFuncs::COM_LoadFile( "gfx/shell/kb_def.lst", NULL );
	char *pfile = afile;
	char token[1024];

	if( !afile )
	{
		UI_ShowMessageBox( "UI_Parse_KeysList: kb_act.lst not found\n" );
		return;
	}

	EngFuncs::ClientCmd( true, "unbindall" );

	while(( pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token ))) != NULL )
	{
		char	key[32];

		Q_strncpy( key, token, sizeof( key ));

		pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token ));
		if( !pfile ) break;	// technically an error

		char	cmd[4096];

		if( key[0] == '\\' && key[1] == '\\' )
		{
			key[0] = '\\';
			key[1] = '\0';
		}

		snprintf( cmd, sizeof( cmd ), "bind \"%s\" \"%s\"\n", key, token );
		EngFuncs::ClientCmd( true, cmd );
	}

	EngFuncs::COM_FreeFile( afile );
	keysListModel.Update();
}
#endif // !XASH_XBOX

bool CMenuControls::CGrabKeyMessageBox::KeyUp( int key )
{
	EUISounds sound;
	CMenuControls *parent = ((CMenuControls*)m_pParent);

#if XASH_XBOX
	// XM.1 item 5: Start must stay cancelselect forever, even if some
	// OTHER row's grab dialog tries to capture it - a real way the lock
	// in EnterGrabMode()/UnbindEntry() below could otherwise be bypassed,
	// since binding K_START_BUTTON to a different command here would
	// silently move Start away from cancelselect without ever touching
	// that row's own entry.
	if( key == K_START_BUTTON )
	{
		sound = SND_BUZZ;
	}
	else
#endif // XASH_XBOX
	// defining a key
	// escape is special, should allow rebind all keys on gamepad
	if( UI::Key::IsConsole( key ) || key == K_ESCAPE
		|| !parent->keysListModel.entries.IsValidIndex( parent->keysList.GetCurrentIndex( )))
	{
		sound = SND_BUZZ;
	}
	else
	{
		const char *bindName = parent->keysListModel.entries[parent->keysList.GetCurrentIndex( )].bind;

		EngFuncs::ClientCmdF( true, "bind \"%s\" \"%s\"\n", EngFuncs::KeynumToString( key ), bindName );

		sound = SND_LAUNCH;
	}

	parent->keysListModel.Update();
	Hide();
	PlayLocalSound( uiStatic.sounds[sound] );

	return true;
}

bool CMenuControls::CGrabKeyMessageBox::KeyDown( int key )
{
	return true;
}

void CMenuControls::UnbindEntry()
{
	if( !keysListModel.IsLineUsable( keysList.GetCurrentIndex( )))
	{
		PlayLocalSound( uiStatic.sounds[SND_BUZZ] );
		return; // not a key
	}

	const char *bindName = keysListModel.entries[keysList.GetCurrentIndex( )].bind;

#if XASH_XBOX
	if( !strcmp( bindName, "cancelselect" ))
	{
		PlayLocalSound( uiStatic.sounds[SND_BUZZ] );
		return;
	}
#endif // XASH_XBOX

	UnbindCommand( bindName );
	PlayLocalSound( uiStatic.sounds[SND_REMOVEKEY] );
	keysListModel.Update();

	// disabled: left command just unbinded
	// msgBox1.Show();
}

void CMenuControls::EnterGrabMode()
{
	if( !keysListModel.IsLineUsable( keysList.GetCurrentIndex( )))
	{
		PlayLocalSound( uiStatic.sounds[SND_REMOVEKEY] );
		return;
	}

	// entering to grab-mode
	const char *bindName = keysListModel.entries[keysList.GetCurrentIndex( )].bind;

#if XASH_XBOX
	// XM.1 item 5: Start's row is shown so the player can see it is bound,
	// but stays cancelselect forever - it is the only pad route back to
	// the pause menu from gameplay (fork-plan.md's own explicit call).
	if( !strcmp( bindName, "cancelselect" ))
	{
		PlayLocalSound( uiStatic.sounds[SND_BUZZ] );
		return;
	}
#endif // XASH_XBOX

	int keys[2];

	CMenuKbActListModel::LookupBoundKeys( bindName, keys );
	if( keys[1] != -1 )
		UnbindCommand( bindName );

	msgBox1.Show();

	PlayLocalSound( uiStatic.sounds[SND_KEY] );
}

#if XASH_XBOX
/*
=================
CMenuControls::KeyDown

Y is a screen-level shortcut, not tied to any one row - CMenuTable's own
KeyDown already routes A/X to OnActivateEntry()/OnDeleteEntry() per the
focused row, but nothing else in this list claims Y, so it reaches here.
=================
*/
bool CMenuControls::KeyDown( int key )
{
	if( key == K_Y_BUTTON )
	{
		ApplySchemeCb();
		return true;
	}

	return CMenuFramework::KeyDown( key );
}

/*
=================
CMenuControls::ApplySchemeCb

fork-plan.md item 5's own explicit fallback: a Custom config (no exact
scheme match) resolves to Standard rather than leaving Y a no-op for
whoever needs it most - a player whose binds are already a mess.
=================
*/
void CMenuControls::ApplySchemeCb( void )
{
	EControllerScheme scheme = UI_DetectControllerScheme();

	if( scheme == SCHEME_CUSTOM )
		scheme = SCHEME_STANDARD;

	UI_ApplyControllerScheme( scheme );
	keysListModel.Update();

	PlayLocalSound( uiStatic.sounds[SND_LAUNCH] );
}

/*
=================
CMenuControls::Hide

B/Escape already reach this override via the base class's own KeyDown
(CMenuBaseWindow - the same idiom Pause.cpp's own header comment
documents: "Escape branch calls Hide() directly, never a separate
handler"), and B is this screen's ONLY way to leave - there is no
separate "OK" action on Xbox. Codex review round 1 caught an earlier
version of this override matching the PC "Cancel" button instead
(reverting to the last-written keyboard.cfg via "exec keyboard"): with
no other exit path, every rebind/clear/scheme-reset this session made
was silently discarded the moment the player backed out, exactly the
class of bug this ledger's own "prove it by consequence" discipline
exists to catch. Persists instead, the same "host_writeconfig" the base
class's own SaveAndPopMenu() (CMenuBaseWindow, controls/BaseWindow.cpp)
already runs before an "OK"-style close everywhere else in this
codebase - every path off this screen now saves, matching what a
player expects from the only Back a console UI gives them.
=================
*/
void CMenuControls::Hide( void )
{
	EngFuncs::ClientCmd( false, "host_writeconfig\n" );

	CMenuFramework::Hide();
}
#endif // XASH_XBOX

/*
=================
UI_Controls_Init
=================
*/
void CMenuControls::_Init( void )
{
	banner.SetPicture( ART_BANNER );

	keysList.SetRect( 360, 230, -20, 465 );
	keysList.SetModel( &keysListModel );
	keysList.SetupColumn( 0, L( "GameUI_Action" ), 0.50f );
	keysList.SetupColumn( 1, L( "GameUI_KeyButton" ), 0.25f );
	keysList.SetupColumn( 2, L( "GameUI_Alternate" ), 0.25f );

	msgBox1.SetMessage( L( "Press a key or button" ) );
	msgBox1.Link( this );

	AddItem( banner );
#if !XASH_XBOX
	msgBox2.SetMessage( L( "GameUI_KeyboardSettingsText" ) );
	msgBox2.onPositive = VoidCb( &CMenuControls::ResetKeysList );
	msgBox2.Link( this );

	AddButton( L( "GameUI_UseDefaults" ), nullptr, PC_USE_DEFAULTS, msgBox2.MakeOpenEvent( ));
	// XM.1 item 9 (fork-plan.md): AdvancedControls.cpp is hidden on
	// Xbox - its two useful rows already moved to Game (item 8), and
	// hiding it here closes its own console command and the only path
	// into InputDevices.cpp too.
	AddButton( L( "Adv. Controls" ), nullptr, PC_ADV_CONTROLS, UI_AdvControls_Menu );
	AddButton( L( "GameUI_OK" ), nullptr, PC_OK, VoidCb( &CMenuControls::SaveAndPopMenu ));
	AddButton( L( "GameUI_Cancel" ), nullptr, PC_CANCEL, VoidCb( &CMenuControls::Cancel ));
#endif // !XASH_XBOX
	AddItem( keysList );

#if XASH_XBOX
	// XM.1 item 5's own A/X/Y/B model replaces the PC button row entirely -
	// "Use Defaults" is superseded by Y (ApplySchemeCb, above), and "OK"/
	// "Cancel" both by B (Hide(), above), which persists every change
	// this screen made (host_writeconfig) since B is the only way to
	// leave - not PC Cancel()'s own revert, corrected in review.
	legend.SetRealCoord( 72, 438 );
	legend.Add( LEGEND_A, L( "Reassign" ) );
	legend.Add( LEGEND_X, L( "Clear" ) );
	legend.Add( LEGEND_Y, L( "Reset" ) );
	legend.Add( LEGEND_B, L( "Back" ) );
	AddItem( legend );
#endif // XASH_XBOX
}

void CMenuControls::_VidInit()
{
	msgBox1.SetRect( DLG_X + 192, 256, 640, 128 );
	msgBox1.pos.x += uiStatic.xOffset;
	msgBox1.pos.y += uiStatic.yOffset;

	keysListModel.Update();
}

ADD_MENU( menu_controls, CMenuControls, UI_Controls_Menu );
