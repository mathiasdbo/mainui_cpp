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

class CMenuControllerSettings : public CMenuFramework
{
public:
	CMenuControllerSettings() : CMenuFramework( "CMenuControllerSettings" ) { }

private:
	void _Init( void ) override;
	void _VidInit( void ) override;
	void GetConfig( void );
	void SaveAndPopMenu( void ) override;

	CMenuAction heading;

	CMenuSlider lookSensitivity;
	CMenuCheckBox invertLook;
	CMenuCheckBox vibrationEnable;
	CMenuCheckBox enableOsk;

	CMenuPicButton done;

	// XM.1 item 12: A Select / B Back / D-pad Move - Adjust - the same
	// legend VideoOptions.cpp already uses for the same mix of one
	// D-pad-adjustable slider plus toggle-with-A checkboxes.
	CMenuLegend legend;
};

/*
=================
CMenuControllerSettings::GetConfig

joy_pitch's own sign is "Invert look" (vertical look only, matching stock
Gamepad.cpp's own invPitch checkbox and every FPS convention this fork has
seen elsewhere) - joy_yaw (horizontal turning) never gets an invert option,
same as before. Both cvars are written together on Done, so only joy_pitch's
magnitude needs reading back here; if a future path ever wrote them to
different magnitudes, this screen would just resync them to whatever
joy_pitch says the next time it opens, not corrupt anything.
=================
*/
void CMenuControllerSettings::GetConfig( void )
{
	float pitch = EngFuncs::GetCvarFloat( "joy_pitch" );

	lookSensitivity.SetCurrentValue( fabs( pitch ) / SENSITIVITY_SCALE );
	invertLook.bChecked = pitch < 0.0f;

	vibrationEnable.LinkCvar( "vibration_enable" );
	enableOsk.LinkCvar( "osk_enable" );
}

/*
=================
CMenuControllerSettings::SaveAndPopMenu
=================
*/
void CMenuControllerSettings::SaveAndPopMenu( void )
{
	float magnitude = lookSensitivity.GetCurrentValue() * SENSITIVITY_SCALE;

	EngFuncs::CvarSetValue( "joy_pitch", invertLook.bChecked ? -magnitude : magnitude );
	EngFuncs::CvarSetValue( "joy_yaw", magnitude );

	vibrationEnable.WriteCvar();
	enableOsk.WriteCvar();

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
	AddItem( done );
	AddItem( legend );
}

void CMenuControllerSettings::_VidInit( void )
{
	GetConfig();
}

ADD_MENU( menu_controllersettings, CMenuControllerSettings, UI_ControllerSettings_Menu );

#endif // XASH_XBOX
