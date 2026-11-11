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
#include "Slider.h"
#include "CheckBox.h"
#include "PicButton.h"
#include "Action.h"
#include "Legend.h"

// XM.1 item 5, Branch 4 of its own implementation plan (fork-plan.md) - the
// Controller screen's "Settings" sub-screen, opened from the not-yet-built
// Controller screen (Branch 3). A new screen, not a reshape of a stock one:
// stock Gamepad.cpp had six raw per-axis-slot spins plus four full
// sensitivity/invert slider pairs, all replaced by the scheme model
// (divergence #82) and this one merged "Look sensitivity" control. Xbox-only
// end to end, the same reasoning Pause.cpp's own header comment already
// gives - the guard has to come after these includes (XASH_XBOX is reached
// transitively through Framework.h, not available before any #include runs).
#if XASH_XBOX

// joy_pitch/joy_yaw (engine/client/input/in_joy.c:48-49) both default to
// "100.0" and have no documented practical range - stock Gamepad.cpp let a
// player type anything from 0 to 200 in two separate raw sliders. This
// screen offers one friendlier 1-10 control instead (fork-plan.md's own
// wording) and scales it back up to that same real cvar range, so a slider
// at its default middle position (5) reproduces the engine's own "100.0"
// default exactly.
#define SENSITIVITY_SCALE 20.0f

// The four stick dead zones, one slider each: the engine's own joy_*_deadzone
// cvars (engine/client/input/in_joy.c), shown as the raw number the config
// files and the packager use. An axis whose reading is inside its dead zone is
// taken as not moved. Original Xbox pads rest well off centre (a stick left
// alone can read -7424), so a dead zone below that makes the view turn, or the
// player walk, by itself; the Xbox default is 8689 (common/defaults.h,
// divergences.md #104). A worn stick can need more, hence 0..16000 - the same
// choices the packager's Controller group offers.
#define DEADZONE_MAX 16000.0f
#define DEADZONE_STEP 500.0f

static const struct
{
	const char *cvar;
	const char *title;
} s_deadZones[] =
{
	{ "joy_yaw_deadzone",     "Look stick, left-right" },
	{ "joy_pitch_deadzone",   "Look stick, up-down" },
	{ "joy_side_deadzone",    "Move stick, sideways" },
	{ "joy_forward_deadzone", "Move stick, forward-back" },
};
#define NUM_DEADZONES ( sizeof( s_deadZones ) / sizeof( s_deadZones[0] ))

class CMenuControllerSettings : public CMenuFramework
{
public:
	CMenuControllerSettings() : CMenuFramework( "CMenuControllerSettings" ),
		m_bYawWasInverted( false ), m_flOriginalPitchMagnitude( 0.0f ),
		m_flOriginalYawMagnitude( 0.0f ), m_flInitialSliderValue( 0.0f ) { }

	bool KeyDown( int key ) override;

private:
	void _Init( void ) override;
	void _VidInit( void ) override;
	void GetConfig( void );
	void SaveAndPopMenu( void ) override;
	void UpdateDeadZoneLabels( void );

	CMenuAction heading;

	CMenuSlider lookSensitivity;
	CMenuCheckBox invertLook;
	CMenuCheckBox vibrationEnable;
	CMenuCheckBox enableOsk;

	// Right-hand column: the four dead zones. The slider control draws only a
	// bar and its name, so the value is written into the name (deadZoneLabel).
	CMenuAction deadZoneHeading;
	CMenuSlider deadZone[NUM_DEADZONES];
	char deadZoneLabel[NUM_DEADZONES][64];

	// What GetConfig() found: the cvar's real value, and where the slider put
	// it (it clamps to 0..16000 and steps by 500). Done writes a dead zone only
	// when its slider moved, so opening this screen and pressing Done leaves an
	// untouched value exactly as it was - 8689 stays 8689, not 8750 - the same
	// care the look sensitivity slider takes above.
	float m_flDeadZoneRaw[NUM_DEADZONES];
	float m_flDeadZoneInitial[NUM_DEADZONES];

	CMenuPicButton done;

	// XM.1 item 12: A Select / B Back / D-pad Move - Adjust - the same
	// legend VideoOptions.cpp already uses for the same mix of one
	// D-pad-adjustable slider plus toggle-with-A checkboxes.
	CMenuLegend legend;

	// Codex review round 1: stock Gamepad.cpp exposed invYaw as its own
	// checkbox (Gamepad.cpp:230-232), so an existing player can already
	// have joy_yaw < 0 saved. This screen drops the horizontal-invert
	// CONTROL (this item's own design only offers one "Invert look" row,
	// vertical), but must not destroy that already-saved preference the
	// first time someone opens this screen and presses Done with nothing
	// else changed - not a UI checkbox, just remembered sign state,
	// re-applied on save exactly as it was read.
	bool m_bYawWasInverted;

	// Codex review round 2: the round-1 fix above preserved yaw's SIGN but
	// not its MAGNITUDE - merging two independently-adjustable sensitivities
	// into one slider is the intended simplification (fork-plan.md's own
	// wording), but it must only actually happen when the player moves that
	// slider, not as a side effect of opening this screen and pressing Done
	// with nothing touched (joy_pitch=100/joy_yaw=40 is a real reachable
	// state - e.g. a save from before this fork, or the stock four-slider
	// Gamepad.cpp). These three remember what GetConfig() found, so
	// SaveAndPopMenu() can tell "slider actually moved" apart from "still at
	// what it was initialised to" and only unify pitch/yaw in the former
	// case.
	float m_flOriginalPitchMagnitude;
	float m_flOriginalYawMagnitude;
	float m_flInitialSliderValue;
};

/*
=================
CMenuControllerSettings::GetConfig

joy_pitch's own sign is "Invert look" (vertical look only, matching stock
Gamepad.cpp's own invPitch checkbox and every FPS convention this fork has
seen elsewhere). joy_yaw's own sign has no control on this screen, but is
still read and remembered (m_bYawWasInverted) so Done cannot silently
flip it back to positive for a player who already had it inverted. Both
magnitudes are remembered too (m_flOriginal{Pitch,Yaw}Magnitude), along
with the slider's own initial position (m_flInitialSliderValue), so
SaveAndPopMenu() can detect whether the player actually touched the merged
control before deciding whether to unify the two cvars at all.
=================
*/
void CMenuControllerSettings::GetConfig( void )
{
	float pitch = EngFuncs::GetCvarFloat( "joy_pitch" );
	float yaw = EngFuncs::GetCvarFloat( "joy_yaw" );

	m_flOriginalPitchMagnitude = fabs( pitch );
	m_flOriginalYawMagnitude = fabs( yaw );
	m_bYawWasInverted = yaw < 0.0f;

	lookSensitivity.SetCurrentValue( m_flOriginalPitchMagnitude / SENSITIVITY_SCALE );
	m_flInitialSliderValue = lookSensitivity.GetCurrentValue();
	invertLook.bChecked = pitch < 0.0f;

	vibrationEnable.LinkCvar( "vibration_enable" );
	enableOsk.LinkCvar( "osk_enable" );

	for( size_t i = 0; i < NUM_DEADZONES; i++ )
	{
		m_flDeadZoneRaw[i] = EngFuncs::GetCvarFloat( s_deadZones[i].cvar );
		deadZone[i].LinkCvar( s_deadZones[i].cvar );
		m_flDeadZoneInitial[i] = deadZone[i].GetCurrentValue();
	}
	UpdateDeadZoneLabels();
}

/*
=================
CMenuControllerSettings::UpdateDeadZoneLabels

The slider's name carries its value. An untouched slider shows the cvar's real
value (which may be past the slider's 16000 end, or between its steps); once
it moves, it shows where it is.
=================
*/
void CMenuControllerSettings::UpdateDeadZoneLabels( void )
{
	for( size_t i = 0; i < NUM_DEADZONES; i++ )
	{
		float cur = deadZone[i].GetCurrentValue();
		float shown = ( cur == m_flDeadZoneInitial[i] ) ? m_flDeadZoneRaw[i] : cur;

		snprintf( deadZoneLabel[i], sizeof( deadZoneLabel[i] ), "%s: %d", L( s_deadZones[i].title ), (int)( shown + 0.5f ));
		deadZone[i].szName = deadZoneLabel[i];
	}
}

/*
=================
CMenuControllerSettings::KeyDown

B/Escape saves, exactly as Done does. CMenuBaseWindow's own KeyDown calls
Hide() for B/Escape (IsEscape, Utils.h, matches both) and never reaches
SaveAndPopMenu(), so every change made here - the look sensitivity, invert,
vibration, the keyboard row and the four dead zones - was dropped when the
player left with B, which is how a console pad leaves a screen. Controls.cpp
and Gamepad.cpp found the same bug in review and made every exit persist; this
screen does the same by sending B through the one path Done uses (it writes
the cvars, then host_writeconfig, then closes), so there is a single way to
save, not two.
=================
*/
bool CMenuControllerSettings::KeyDown( int key )
{
	if( UI::Key::IsEscape( key ))
	{
		SaveAndPopMenu();
		return true;
	}

	return CMenuFramework::KeyDown( key );
}

/*
=================
CMenuControllerSettings::SaveAndPopMenu

Only unifies joy_pitch/joy_yaw into the slider's own value when the slider
itself actually moved - otherwise each cvar keeps its own original
magnitude (Codex review round 2), with only the sign changing if "Invert
look" was toggled. This is the merged control's whole point (fork-plan.md:
"one LinkCvar-style write driving both") without it silently overwriting a
magnitude nothing on this screen asked to change.
=================
*/
void CMenuControllerSettings::SaveAndPopMenu( void )
{
	bool sliderChanged = lookSensitivity.GetCurrentValue() != m_flInitialSliderValue;
	float magnitude = lookSensitivity.GetCurrentValue() * SENSITIVITY_SCALE;

	float pitchMagnitude = sliderChanged ? magnitude : m_flOriginalPitchMagnitude;
	float yawMagnitude = sliderChanged ? magnitude : m_flOriginalYawMagnitude;

	EngFuncs::CvarSetValue( "joy_pitch", invertLook.bChecked ? -pitchMagnitude : pitchMagnitude );
	EngFuncs::CvarSetValue( "joy_yaw", m_bYawWasInverted ? -yawMagnitude : yawMagnitude );

	vibrationEnable.WriteCvar();
	enableOsk.WriteCvar();

	for( size_t i = 0; i < NUM_DEADZONES; i++ )
	{
		if( deadZone[i].GetCurrentValue() != m_flDeadZoneInitial[i] )
			EngFuncs::CvarSetValue( s_deadZones[i].cvar, deadZone[i].GetCurrentValue() );
	}

	CMenuFramework::SaveAndPopMenu();
}

/*
=================
CMenuControllerSettings::_Init
=================
*/
void CMenuControllerSettings::_Init( void )
{
	// XM.1: text heading in place of a head_*.bmp banner, matching every
	// other Xbox screen this fork reshaped (Audio.cpp, GameOptions.cpp,
	// VideoOptions.cpp).
	heading.iFlags = QMF_INACTIVE|QMF_DROPSHADOW;
	heading.szName = L( "Controller Settings" );
	heading.colorBase = uiColorHeading;
	heading.SetCharSize( QM_BIGFONT );
	heading.SetRect( 72, 200, 400, 32 );

	// Codex review round 1 (item 8/6's own lesson, applied not
	// re-discovered): CMenuSlider draws its label above its SetCoord y, so
	// the first slider reuses stock Audio.cpp's own proven-safe y=280,
	// clear of the heading above it.
	lookSensitivity.szName = L( "Look sensitivity" );
	lookSensitivity.SetCoord( 72, 280 );
	lookSensitivity.Setup( 1.0f, 10.0f, 1.0f );

	invertLook.SetNameAndStatus( L( "Invert look" ), L( "Invert the vertical look direction" ) );
	invertLook.SetCoord( 72, 340 );

	vibrationEnable.SetNameAndStatus( L( "Vibration" ), L( "Enable controller vibration" ) );
	vibrationEnable.SetCoord( 72, 390 );

	// Migrated verbatim from stock Gamepad.cpp (name, status text and cvar
	// unchanged) - the one row XM.1 item 5's own design left unmentioned,
	// resolved by moving it here rather than dropping it silently.
	enableOsk.SetNameAndStatus( L( "Builtin on-screen keyboard" ), L( "Enable builtin on-screen keyboard in case your platform doesn't have any" ) );
	enableOsk.SetCoord( 72, 440 );

	// The dead zones sit in a second column, 520 across, so the screen stays
	// as short as it was: five more rows in one column would not fit under the
	// heading in 768 rows. Each slider's label is drawn above its y, so the
	// first starts a row below the column heading.
	deadZoneHeading.iFlags = QMF_INACTIVE|QMF_DROPSHADOW;
	deadZoneHeading.szName = L( "Stick dead zones" );
	deadZoneHeading.colorBase = uiColorHeading;
	deadZoneHeading.SetRect( 520, 236, 400, 24 );

	for( size_t i = 0; i < NUM_DEADZONES; i++ )
	{
		deadZone[i].Setup( 0.0f, DEADZONE_MAX, DEADZONE_STEP );
		deadZone[i].SetCoord( 520, 300 + 60 * (int)i );
		deadZone[i].onCvarChange = VoidCb( &CMenuControllerSettings::UpdateDeadZoneLabels );
	}

	done.SetNameAndStatus( L( "Done" ), nullptr );
	done.SetPicture( PC_DONE );
	done.onReleased = VoidCb( &CMenuControllerSettings::SaveAndPopMenu );
	done.iFlags |= QMF_NOTIFY;
	done.SetCoord( 72, 500 );

	legend.SetRealCoord( 72, 438 );
	legend.Add( LEGEND_DPAD, L( "Move - Adjust" ) );
	legend.Add( LEGEND_A, L( "Select" ) );
	legend.Add( LEGEND_B, L( "Back" ) );

	// Registration order is pad focus order (ItemsHolder.cpp), independent
	// of on-screen position - divergence #66's own already-established
	// lesson, applied here rather than re-discovered.
	AddItem( heading );
	AddItem( lookSensitivity );
	AddItem( invertLook );
	AddItem( vibrationEnable );
	AddItem( enableOsk );
	AddItem( deadZoneHeading );
	for( size_t i = 0; i < NUM_DEADZONES; i++ )
		AddItem( deadZone[i] );
	AddItem( done );
	AddItem( legend );
}

void CMenuControllerSettings::_VidInit( void )
{
	GetConfig();
}

ADD_MENU( menu_controllersettings, CMenuControllerSettings, UI_ControllerSettings_Menu );

#endif // XASH_XBOX
