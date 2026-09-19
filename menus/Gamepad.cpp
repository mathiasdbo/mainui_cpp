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

#include "build.h"
#include "Framework.h"
#include "Bitmap.h"
#include "PicButton.h"
#include "Slider.h"
#include "CheckBox.h"
#include "SpinControl.h"
#include "StringArrayModel.h"
#include "Switch.h"
#include "Action.h"
#include "keydefs.h"
#if XASH_XBOX
#include "Legend.h"
#include "ControllerSchemes.h"
#endif

#define ART_BANNER			"gfx/shell/head_gamepad"

enum engineAxis_t
{
	JOY_AXIS_SIDE = 0,
	JOY_AXIS_FWD,
	JOY_AXIS_PITCH,
	JOY_AXIS_YAW,
	JOY_AXIS_RT,
	JOY_AXIS_LT,
	JOY_AXIS_NULL
};

static const char *axisNames[7] =
{
	"Side",
	"Forward",
	"Yaw",
	"Pitch",
	"Right Trigger",
	"Left Trigger",
	"NOT BOUND"
};

#if XASH_XBOX
// XM.1 item 5 (fork-plan.md): "the scheme's mapping list (Left trigger,
// Left stick, Stick click, D-pad, Back) beside the Duke diagram with the
// scheme's labels drawn by code". Read live from the engine's own bind/
// axis state rather than from whichever scheme is selected - the same
// "real state, never a literal" idiom this fork already uses for every
// other read-only annotation (Pause.cpp's chapter title, VideoOptions.cpp's
// video output row). This is also the only way to get Custom right: there
// is no stored table for it to read from, but the live binds are always
// real regardless of which named scheme (if any) currently matches them.
struct CommandLabel_t
{
	const char *command;
	const char *label;
};

// One entry per command any scheme table (ControllerSchemes.cpp) can
// actually bind, mapped to kb_act.lst's own "#Valve_*" dictionary key so
// this reuses the exact same translated strings the Customize screen's
// own action list already shows for these commands - not a second set of
// labels to keep in sync by hand. "cancelselect" is this fork's own
// binding (not a kb_act.lst action), so it gets the same literal
// "Pause Menu" label the Customize screen's own locked row already uses.
static const CommandLabel_t s_commandLabels[] =
{
	{ "+attack",      "#Valve_Primary_Attack" },
	{ "+attack2",     "#Valve_Secondary_Attack" },
	{ "+jump",        "#Valve_Jump" },
	{ "+duck",        "#Valve_Duck" },
	{ "+use",         "#Valve_Use_Items" },
	{ "+reload",      "#Valve_Reload_Weapon" },
	{ "invnext",      "#Valve_Next_Weapon" },
	{ "invprev",      "#Valve_Previous_Weapon" },
	{ "impulse 100",  "#Valve_Flashlight" },
	{ "+speed",       "#Valve_Walk" },
	{ "slot1",        "#Valve_Weapon_Category_1" },
	{ "slot2",        "#Valve_Weapon_Category_2" },
	{ "slot3",        "#Valve_Weapon_Category_3" },
	{ "slot4",        "#Valve_Weapon_Category_4" },
	{ "save quick",   "#Valve_Quick_Save" },
	{ "pause",        "#Valve_Pause_Game" },
	{ "impulse 201",  "#Valve_Spray_Logo" },
	{ "lastinv",      "#Valve_Last_Weapon_Used" },
	{ "cancelselect", "Pause Menu" },
};

static const char *FriendlyCommandName( const char *command )
{
	if( !command || !command[0] )
		return L( "NOT BOUND" );

	for( size_t i = 0; i < V_ARRAYSIZE( s_commandLabels ); i++ )
	{
		if( !stricmp( command, s_commandLabels[i].command ))
			return L( s_commandLabels[i].label );
	}

	// An unrecognised raw command (a config hand-edited outside every
	// scheme this fork ships) - show it verbatim rather than hide it.
	return command;
}

// One line per scheme (index by EControllerScheme, Custom included at
// CONTROLLER_SCHEME_COUNT) - not this branch's spec to write verbatim,
// so kept short and honest about what each one actually does.
static const char *s_schemeDescriptions[CONTROLLER_SCHEME_COUNT + 1] =
{
	"Recommended - modern trigger controls, matching most shooters.",
	"Standard with the sticks swapped - look with the left stick, move with the right.",
	"Classic Half-Life pad controls, unchanged.",
	"Your own layout - open Customize to change it, or press Y there to reset.",
};

// Codex review round 1: CMenuSwitch itself only changes its own state on
// a mouse click or Enter (Switch.cpp's own KeyDown/KeyUp) - D-pad left/
// right would otherwise be swallowed as plain focus navigation by the
// base ItemsHolder (CMenuItemsHolder::Key, controls/ItemsHolder.cpp:83's
// own "the focused item's KeyDown gets first refusal" contract), leaving
// this screen's own scheme row completely unusable on a real pad. A
// small Xbox-only subclass, not a change to the shared control -
// ServerBrowser.cpp's own tabSwitch (PC-only, mouse-driven) is untouched.
// CONTROLLER_SCHEME_COUNT+1 (Custom) is this screen's own known, fixed
// segment count, not something CMenuSwitch itself exposes generically.
class CMenuSchemeSwitch : public CMenuSwitch
{
public:
	bool KeyDown( int key ) override
	{
		if( UI::Key::IsLeftArrow( key ) || UI::Key::IsRightArrow( key ))
		{
			int count = CONTROLLER_SCHEME_COUNT + 1;
			int state = GetState() + ( UI::Key::IsRightArrow( key ) ? 1 : -1 );

			SetState( ( state + count ) % count );
			PlayLocalSound( uiStatic.sounds[SND_MOVE] );
			return true;
		}

		return CMenuSwitch::KeyDown( key );
	}
};
#endif // XASH_XBOX

class CMenuGamePad : public CMenuFramework
{
public:
	CMenuGamePad() : CMenuFramework("CMenuGamePad") { }

private:
	void _Init() override;
	void _VidInit() override;
	void GetConfig();
#if !XASH_XBOX
	void SaveAndPopMenu() override;
#endif // !XASH_XBOX

#if XASH_XBOX
	void Hide( void ) override;
	void OnSchemeChanged( void );
	void UpdateSchemeDisplay( void );

	CMenuAction heading;

	// XM.1 item 5: Standard / Southpaw / Legacy / Custom as one horizontal
	// row - the exact setup shape ServerBrowser.cpp's own tabSwitch
	// already proves (`:1198-1208`), the only difference being what
	// onChanged does (apply a real scheme here, switch a tab list there).
	CMenuSchemeSwitch schemeSwitch;

	CMenuAction description;
	char descriptionText[128];

	// QMF_INACTIVE, non-interactive - the same pattern VideoOptions.cpp's
	// own gamma test image already proves for a static diagram picture.
	CMenuBitmap diagram;

	CMenuAction mapLabels[5];
	char mapLabelText[5][64];

	// XM.1's own "do not use AddButton(EDefaultBtns)" call
	// (Configuration.cpp:96-135's own precedent, this ledger's own
	// EDefaultBtns-collision lesson): neither button has a real WON strip
	// picture to match its new label, so both are heap-allocated,
	// registered into m_apBtns manually, and never given a picture at
	// all - PicButton.cpp's own text-only branch then always fires,
	// independent of any strip's size, and ~CMenuFramework() still finds
	// them in m_apBtns to clean up correctly on shutdown.
	CMenuPicButton *settingsBtn;
	CMenuPicButton *customizeBtn;
	CMenuPicButton done;

	CMenuLegend legend;
#else
	CMenuSlider side, forward, pitch, yaw;
	CMenuCheckBox invSide, invFwd, invPitch, invYaw;

	CMenuSpinControl axisBind[6];

	CMenuAction axisBind_label;

	CMenuCheckBox enableOsk;
#endif // XASH_XBOX
};

#if !XASH_XBOX
/*
=================
CMenuGamePad::GetConfig
=================
*/
void CMenuGamePad::GetConfig( void )
{
	float _side, _forward, _pitch, _yaw;
	char binding[7] = { 0 };

	enableOsk.LinkCvar( "osk_enable" );

	Q_strncpy( binding, EngFuncs::GetCvarString( "joy_axis_binding"), sizeof( binding ));

	_side = EngFuncs::GetCvarFloat( "joy_side" );
	_forward = EngFuncs::GetCvarFloat( "joy_forward" );
	_pitch = EngFuncs::GetCvarFloat( "joy_pitch" );
	_yaw = EngFuncs::GetCvarFloat( "joy_yaw" );

	side.SetCurrentValue( fabs( _side ) );
	forward.SetCurrentValue( fabs( _forward ));
	pitch.SetCurrentValue( fabs( _pitch ));
	yaw.SetCurrentValue( fabs( _yaw ));

	invSide.bChecked = _side < 0.0f;
	invFwd.bChecked = _forward < 0.0f;
	invPitch.bChecked = _pitch < 0.0f;
	invYaw.bChecked = _yaw < 0.0f;

	// I made a monster...
	for( unsigned int i = 0; i < sizeof( binding ) - 1; i++ )
	{
		switch( binding[i] )
		{
		case 's':
			axisBind[i].ForceDisplayString( L( axisNames[JOY_AXIS_SIDE] ) );
			axisBind[i].SetCurrentValue( JOY_AXIS_SIDE );
			break;
		case 'f':
			axisBind[i].ForceDisplayString( L( axisNames[JOY_AXIS_FWD] ) );
			axisBind[i].SetCurrentValue( JOY_AXIS_FWD );
			break;
		case 'p':
			axisBind[i].ForceDisplayString( L( axisNames[JOY_AXIS_PITCH] ) );
			axisBind[i].SetCurrentValue( JOY_AXIS_PITCH );
			break;
		case 'y':
			axisBind[i].ForceDisplayString( L( axisNames[JOY_AXIS_YAW] ) );
			axisBind[i].SetCurrentValue( JOY_AXIS_YAW );
			break;
		case 'r':
			axisBind[i].ForceDisplayString( L( axisNames[JOY_AXIS_RT] ) );
			axisBind[i].SetCurrentValue( JOY_AXIS_RT );
			break;
		case 'l':
			axisBind[i].ForceDisplayString( L( axisNames[JOY_AXIS_LT] ) );
			axisBind[i].SetCurrentValue( JOY_AXIS_LT );
			break;
		default:
			axisBind[i].ForceDisplayString( L( axisNames[JOY_AXIS_NULL] ) );
			axisBind[i].SetCurrentValue( JOY_AXIS_NULL );
		}
	}
}
#else
/*
=================
CMenuGamePad::GetConfig

Runs on every visit (_VidInit(), below), not just construction - binds
can change via the Customize sub-screen between visits, so the switch's
own position has to be re-detected every time, not cached once.
=================
*/
void CMenuGamePad::GetConfig( void )
{
	schemeSwitch.SetState( UI_DetectControllerScheme() );
	UpdateSchemeDisplay();
}

/*
=================
CMenuGamePad::UpdateSchemeDisplay
=================
*/
void CMenuGamePad::UpdateSchemeDisplay( void )
{
	int scheme = schemeSwitch.GetState();

	Q_strncpy( descriptionText, L( s_schemeDescriptions[scheme] ), sizeof( descriptionText ));
	description.szName = descriptionText;

	snprintf( mapLabelText[0], sizeof( mapLabelText[0] ), "Left trigger: %s", FriendlyCommandName( EngFuncs::KEY_GetBinding( K_JOY1 )));

	const char *axisBinding = EngFuncs::GetCvarString( "joy_axis_binding" );
	bool leftStickLooks = axisBinding[0] == 'y' || axisBinding[0] == 'p';
	snprintf( mapLabelText[1], sizeof( mapLabelText[1] ), "Left stick: %s", leftStickLooks ? "Look" : "Move" );

	snprintf( mapLabelText[2], sizeof( mapLabelText[2] ), "Stick click: %s", FriendlyCommandName( EngFuncs::KEY_GetBinding( K_LSTICK )));
	snprintf( mapLabelText[3], sizeof( mapLabelText[3] ), "D-pad: %s", FriendlyCommandName( EngFuncs::KEY_GetBinding( K_DPAD_UP )));
	snprintf( mapLabelText[4], sizeof( mapLabelText[4] ), "Back: %s", FriendlyCommandName( EngFuncs::KEY_GetBinding( K_BACK_BUTTON )));

	for( int i = 0; i < 5; i++ )
		mapLabels[i].szName = mapLabelText[i];
}

/*
=================
CMenuGamePad::OnSchemeChanged

UI_ApplyControllerScheme() no-ops for SCHEME_CUSTOM by design (its own
header comment) - navigating the switch onto Custom just leaves the
live config exactly as it already was, which is what put it there.
=================
*/
void CMenuGamePad::OnSchemeChanged( void )
{
	UI_ApplyControllerScheme( (EControllerScheme)schemeSwitch.GetState() );
	UpdateSchemeDisplay();
}
#endif // XASH_XBOX

#if !XASH_XBOX
/*
=================
CMenuGamePad::SetConfig
=================
*/
void CMenuGamePad::SaveAndPopMenu()
{
	float _side, _forward, _pitch, _yaw;
	char binding[7] = { 0 };

	_side = side.GetCurrentValue();
	if( invSide.bChecked )
		_side *= -1;

	_forward = forward.GetCurrentValue();
	if( invFwd.bChecked )
		_forward *= -1;

	_pitch = pitch.GetCurrentValue();
	if( invPitch.bChecked )
		_pitch *= -1;

	_yaw = yaw.GetCurrentValue();
	if( invYaw.bChecked )
		_yaw *= -1;

	for( int i = 0; i < 6; i++ )
	{
		switch( (int)axisBind[i].GetCurrentValue() )
		{
		case JOY_AXIS_SIDE: binding[i]  = 's'; break;
		case JOY_AXIS_FWD: binding[i]   = 'f'; break;
		case JOY_AXIS_PITCH: binding[i] = 'p'; break;
		case JOY_AXIS_YAW: binding[i]   = 'y'; break;
		case JOY_AXIS_RT: binding[i]    = 'r'; break;
		case JOY_AXIS_LT: binding[i]    = 'l'; break;
		default: binding[i] = '0'; break;
		}
	}

	EngFuncs::CvarSetValue( "joy_side", _side );
	EngFuncs::CvarSetValue( "joy_forward", _forward );
	EngFuncs::CvarSetValue( "joy_pitch", _pitch );
	EngFuncs::CvarSetValue( "joy_yaw", _yaw );
	EngFuncs::CvarSetString( "joy_axis_binding", binding );

	enableOsk.WriteCvar();

	CMenuFramework::SaveAndPopMenu();
}
#else
/*
=================
CMenuGamePad::Hide

Codex review round 1: a scheme applies immediately on selection
(OnSchemeChanged, above), but B/Escape's own inherited path
(CMenuBaseWindow's own KeyDown - the same idiom Pause.cpp's own header
comment documents, and Controls.cpp's own Hide() override already
established for exactly this reason) calls Hide() directly, never
Done's own SaveAndPopMenu() - so a scheme picked here, then left via
B, was never persisted (host_writeconfig) and reverted on next boot.
Done now routes through this same override (below) instead of its own
SaveAndPopMenu(), so there is exactly one persistence path for both
exits, not two separate writes.
=================
*/
void CMenuGamePad::Hide( void )
{
	EngFuncs::ClientCmd( false, "host_writeconfig\n" );

	CMenuFramework::Hide();
}
#endif // XASH_XBOX

/*
=================
CMenuGamePad::Init
=================
*/
#if !XASH_XBOX
void CMenuGamePad::_Init( void )
{
	int i, y;

	static CStringArrayModel model( axisNames, V_ARRAYSIZE( axisNames ) );

	banner.SetPicture( ART_BANNER );

	enableOsk.SetNameAndStatus( L( "Builtin on-screen keyboard" ), L( "Enable builtin on-screen keyboard in case your platform doesn't have any" ));

	axisBind_label.eTextAlignment = QM_CENTER;
	axisBind_label.iFlags = QMF_INACTIVE|QMF_DROPSHADOW;
	axisBind_label.colorBase = uiColorHelp;
	axisBind_label.szName = L( "Axis binding map" );

	for( i = 0, y = 230; i < 6; i++, y += 50 )
	{
		axisBind[i].szStatusText = L( "Set axis binding" );
		axisBind[i].Setup( &model );
	}

	side.Setup( 0.0f, 1.0f, 0.1f );
	side.SetNameAndStatus( L( "Side" ), L( "Side movement sensitivity" ) );
	invSide.SetNameAndStatus( L( "Invert" ), L( "Invert side movement axis" ) );

	forward.Setup( 0.0f, 1.0f, 0.1f );
	forward.SetNameAndStatus( L( "Forward" ), L( "Forward movement sensitivity" ) );
	invFwd.SetNameAndStatus( L( "Invert" ), L( "Invert forward movement axis" ) );

	pitch.Setup( 0.0f, 200.0f, 0.1f );
	pitch.SetNameAndStatus( L( "Look X" ), L( "Horizontal look sensitivity" ) );
	invPitch.SetNameAndStatus( L( "Invert" ), L( "Invert pitch axis" ) );

	yaw.Setup( 0.0f, 200.0f, 0.1f );
	yaw.SetNameAndStatus( L( "Look Y" ), L( "Vertical look sensitivity" ) );
	invYaw.SetNameAndStatus( L( "Invert" ), L( "Invert yaw axis" ) );

	AddItem( banner );
	AddButton( L( "Controls" ), nullptr, PC_CONTROLS, UI_Controls_Menu );
	AddButton( L( "Gyroscope" ), nullptr, PC_GYRO, UI_GamePadGyro_Menu, QMF_NOTIFY, 'g' );
	AddButton( L( "Done" ), nullptr, PC_DONE, VoidCb( &CMenuGamePad::SaveAndPopMenu ) );	// Обе строки уже встречались ранее !!
	for( i = 0; i < 6; i++ )
		AddItem( axisBind[i] );
	AddItem( enableOsk );
	AddItem( side );
	AddItem( invSide );
	AddItem( forward );
	AddItem( invFwd );
	AddItem( pitch );
	AddItem( invPitch );
	AddItem( yaw );
	AddItem( invYaw );
	AddItem( axisBind_label );
}
#else
void CMenuGamePad::_Init( void )
{
	heading.iFlags = QMF_INACTIVE|QMF_DROPSHADOW;
	heading.szName = L( "Controller" );
	heading.colorBase = uiColorHeading;
	heading.SetCharSize( QM_BIGFONT );
	heading.SetRect( 72, 200, 400, 32 );

	schemeSwitch.SetRect( 72, 270, 460, 32 );
	schemeSwitch.AddSwitch( L( "Standard" ) );
	schemeSwitch.AddSwitch( L( "Southpaw" ) );
	schemeSwitch.AddSwitch( L( "Legacy" ) );
	schemeSwitch.AddSwitch( L( "Custom" ) );
	schemeSwitch.eTextAlignment = QM_CENTER;
	schemeSwitch.bMouseToggle = false;
	schemeSwitch.bKeepToggleWidth = true;
	schemeSwitch.iSelectColor = uiInputFgColor;
	schemeSwitch.iFgTextColor = uiInputFgColor - 0x00151515; // bit darker, matching ServerBrowser.cpp's own tabSwitch
	schemeSwitch.onChanged = VoidCb( &CMenuGamePad::OnSchemeChanged );

	description.iFlags = QMF_INACTIVE|QMF_DROPSHADOW;
	description.colorBase = uiColorHelp;
	description.SetCharSize( QM_SMALLFONT );
	description.SetRect( 72, 320, 460, 48 );

	diagram.iFlags = QMF_INACTIVE;
	diagram.SetRect( 560, 220, 340, 253 );
	diagram.SetPicture( "gfx/shell/duke" );

	for( int i = 0; i < 5; i++ )
	{
		mapLabels[i].iFlags = QMF_INACTIVE|QMF_DROPSHADOW;
		mapLabels[i].colorBase = uiColorHelp;
		mapLabels[i].SetCharSize( QM_SMALLFONT );
		mapLabels[i].SetRect( 560, 480 + i * 24, 400, 22 );
	}

	settingsBtn = new CMenuPicButton();
	settingsBtn->SetNameAndStatus( L( "Settings..." ), L( "Look sensitivity, invert look, vibration" ) );
	settingsBtn->onReleased = UI_ControllerSettings_Menu;
	settingsBtn->iFlags |= QMF_NOTIFY;
	settingsBtn->SetCoord( 72, 350 + m_iBtnsNum * 50 );
	AddItem( *settingsBtn );
	m_apBtns[m_iBtnsNum++] = settingsBtn;

	customizeBtn = new CMenuPicButton();
	customizeBtn->SetNameAndStatus( L( "Customize..." ), L( "Change individual button and axis bindings" ) );
	customizeBtn->onReleased = UI_Controls_Menu;
	customizeBtn->iFlags |= QMF_NOTIFY;
	customizeBtn->SetCoord( 72, 350 + m_iBtnsNum * 50 );
	AddItem( *customizeBtn );
	m_apBtns[m_iBtnsNum++] = customizeBtn;

	done.SetNameAndStatus( L( "Done" ), nullptr );
	done.SetPicture( PC_DONE );
	done.onReleased = VoidCb( &CMenuGamePad::Hide );
	done.iFlags |= QMF_NOTIFY;
	done.SetCoord( 72, 350 + m_iBtnsNum * 50 );

	legend.SetRealCoord( 72, 438 );
	legend.Add( LEGEND_DPAD, L( "Move - Adjust" ) );
	legend.Add( LEGEND_A, L( "Select" ) );
	legend.Add( LEGEND_B, L( "Back" ) );

	// Registration order is pad focus order (ItemsHolder.cpp), independent
	// of on-screen position - divergence #66's own already-established
	// lesson, applied here rather than re-discovered.
	AddItem( heading );
	AddItem( schemeSwitch );
	AddItem( description );
	AddItem( diagram );
	for( int i = 0; i < 5; i++ )
		AddItem( mapLabels[i] );
	AddItem( done );
	AddItem( legend );
}
#endif // XASH_XBOX

void CMenuGamePad::_VidInit()
{
#if !XASH_XBOX
	axisBind_label.SetCoord( 360, 230 );
	axisBind_label.SetCharSize( QM_SMALLFONT );

	int y = 280;
	for( int i = 0; i < 6; i++, y += 50 )
	{
		axisBind[i].SetRect( 360, y, 256, invSide.size.h );
		axisBind[i].SetCharSize( QM_SMALLFONT );
	}

	enableOsk.SetCoord( 360, y );

	int sliderAlign = invSide.size.h - side.size.h;

	side.SetCoord( 630, 280 + sliderAlign );
	side.SetCharSize( QM_SMALLFONT );
	invSide.SetCoord( 850, 280 );

	forward.SetCoord( 630, 330 + sliderAlign );
	forward.SetCharSize( QM_SMALLFONT );
	invFwd.SetCoord( 850, 330 );

	pitch.SetCoord( 630, 380 + sliderAlign );
	pitch.SetCharSize( QM_SMALLFONT );
	invPitch.SetCoord( 850, 380 );

	yaw.SetCoord( 630, 430 + sliderAlign );
	yaw.SetCharSize( QM_SMALLFONT );
	invYaw.SetCoord( 850, 430 );
#endif // !XASH_XBOX

	GetConfig();
}

ADD_MENU( menu_gamepad, CMenuGamePad, UI_GamePad_Menu );
