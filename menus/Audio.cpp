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
#include "Bitmap.h"
#include "PicButton.h"
#include "CheckBox.h"
#include "SpinControl.h"
#include "StringArrayModel.h"
#include "Action.h"

#define ART_BANNER			"gfx/shell/head_audio"

class CMenuAudio : public CMenuFramework
{
public:
	typedef CMenuFramework BaseClass;

	CMenuAudio() : CMenuFramework("CMenuAudio") { }

private:
	void _Init() override;
	void _VidInit() override;
	void GetConfig();
#if !XASH_XBOX
	void VibrateChanged();
#endif // !XASH_XBOX
	void SaveAndPopMenu() override;

	CMenuSlider	soundVolume;
	CMenuSlider	musicVolume;
	CMenuSlider	suitVolume;
#if XASH_XBOX
	// XM.1 (fork-plan.md): DSP has no player-facing meaning on a fixed
	// console (the earlier draft's "keep DSP" is reversed) and
	// snd_mute_losefocus doesn't apply either - Xbox has no window to
	// lose focus to. Vibration moves to Controller (item 5) once it
	// exists; hiding it here now, before that screen is built, is the
	// plan's own explicit call (unlike Joystick in Configuration.cpp,
	// which stays reachable because nothing replaces it yet) - vibration
	// still defaults on (vibration_enable "1", cl_mobile.c) and just
	// cannot be turned off until Controller ships, a real but narrow gap
	// next to losing gamepad-axis configuration entirely.
	//
	// The design also calls for a read-only "Speaker output" row from
	// the Dashboard - deliberately NOT built this pass: nxdk exposes the
	// setting only as a raw EEPROM value (ExQueryNonVolatileSetting,
	// XC_AUDIO, xboxkrnl.h:4229), with no documented bit layout anywhere
	// in this vendored copy or its samples, unlike Display's video
	// output row (XVideoGetEncoderSettings's named fields, verified
	// against divergence #64's own PR round 2). Decoding XC_AUDIO
	// without that would be exactly the "never guess a constant" this
	// project's own rules forbid.
	CMenuAction heading;
	CMenuPicButton done;
#else
	CMenuSlider	vibration;
	CMenuCheckBox noDSP;
	CMenuCheckBox useAlphaDSP;
	CMenuCheckBox muteFocusLost;
	CMenuCheckBox vibrationEnable;

	float oldVibrate;
#endif // XASH_XBOX
};

/*
=================
CMenuAudio::GetConfig
=================
*/
void CMenuAudio::GetConfig( void )
{
	soundVolume.LinkCvar( "volume" );
	musicVolume.LinkCvar( "MP3Volume" );
	suitVolume.LinkCvar( "suitvolume" );

#if !XASH_XBOX
	vibration.LinkCvar( "vibration_length" );

	noDSP.LinkCvar( "room_off" );
	useAlphaDSP.LinkCvar( "dsp_coeff_table" );
	muteFocusLost.LinkCvar( "snd_mute_losefocus" );
	vibrationEnable.LinkCvar( "vibration_enable" );

	if( !vibrationEnable.bChecked )
		vibration.SetGrayed( true );
	oldVibrate = vibration.GetCurrentValue();
#endif // !XASH_XBOX
}

#if !XASH_XBOX
void CMenuAudio::VibrateChanged()
{
	float newVibrate = vibration.GetCurrentValue();
	if( oldVibrate != newVibrate )
	{
		char cmd[64];
		snprintf( cmd, 64, "vibrate %f", newVibrate );
		EngFuncs::ClientCmd( false, cmd );
		vibration.WriteCvar();
		oldVibrate = newVibrate;
	}
}
#endif // !XASH_XBOX

/*
=================
CMenuAudio::SetConfig
=================
*/
void CMenuAudio::SaveAndPopMenu()
{
	soundVolume.WriteCvar();
	musicVolume.WriteCvar();
	suitVolume.WriteCvar();
#if !XASH_XBOX
	vibration.WriteCvar();
	noDSP.WriteCvar();
	useAlphaDSP.WriteCvar();
	muteFocusLost.WriteCvar();
	vibrationEnable.WriteCvar();
#endif // !XASH_XBOX

	CMenuFramework::SaveAndPopMenu();
}

/*
=================
CMenuAudio::Init
=================
*/
#if XASH_XBOX
void CMenuAudio::_Init( void )
{
	// XM.1: text heading in place of a head_*.bmp banner.
	heading.iFlags = QMF_INACTIVE|QMF_DROPSHADOW;
	heading.szName = L( "GameUI_Audio" );
	// XM.1 item 13 step 2: a heading's own colour, not the shared
	// uiColorHelp every label/status line on this screen also draws with
	// (the sliders' own names below, the status lines beneath them) -
	// the two are one colour on stock WON, the design wants two.
	heading.colorBase = uiColorHeading;
	heading.SetCharSize( QM_BIGFONT );
	heading.SetRect( 72, 200, 400, 32 );

	// Codex review round 1: CMenuSlider draws its own label ABOVE its
	// SetCoord y (Slider.cpp's textHeight = y - charHeight*1.5, ~39px
	// for the medium font this uses) - a slider at y=260 puts its label
	// at ~221, inside the heading's own 200-232 span. Stock Audio.cpp's
	// own first slider sits at y=280 with nothing above it; reusing that
	// exact value gives the label ~241, clear of the heading below it,
	// rather than inventing a new offset.
	soundVolume.szName = L( "GameUI_SoundEffectVolume" );
	soundVolume.Setup( 0.0, 1.0, 0.05f );
	soundVolume.onChanged = CMenuEditable::WriteCvarCb;
	soundVolume.SetCoord( 72, 280 );
	soundVolume.size.w = 300;

	musicVolume.szName = L( "GameUI_MP3Volume" );
	musicVolume.Setup( 0.0, 1.0, 0.05f );
	musicVolume.onChanged = CMenuEditable::WriteCvarCb;
	musicVolume.SetCoord( 72, 340 );
	musicVolume.size.w = 300;

	suitVolume.szName = L( "GameUI_HEVSuitVolume" );
	suitVolume.Setup( 0.0, 1.0, 0.05f );
	suitVolume.onChanged = CMenuEditable::WriteCvarCb;
	suitVolume.SetCoord( 72, 400 );
	suitVolume.size.w = 300;

	done.SetNameAndStatus( L( "Done" ), nullptr );
	done.SetPicture( PC_DONE );
	done.onReleased = VoidCb( &CMenuAudio::SaveAndPopMenu );
	done.iFlags |= QMF_NOTIFY;
	done.SetCoord( 72, 480 );

	AddItem( heading );
	AddItem( soundVolume );
	AddItem( musicVolume );
	AddItem( suitVolume );
	AddItem( done );
}
#else
void CMenuAudio::_Init( void )
{
	banner.SetPicture(ART_BANNER);

	soundVolume.szName = L( "GameUI_SoundEffectVolume" );
	soundVolume.Setup( 0.0, 1.0, 0.05f );
	soundVolume.onChanged = CMenuEditable::WriteCvarCb;
	soundVolume.SetCoord( 320, 280 );
	soundVolume.size.w = 300;

	musicVolume.szName = L( "GameUI_MP3Volume" );
	musicVolume.Setup( 0.0, 1.0, 0.05f );
	musicVolume.onChanged = CMenuEditable::WriteCvarCb;
	musicVolume.SetCoord( 320, 340 );
	musicVolume.size.w = 300;

	suitVolume.szName = L( "GameUI_HEVSuitVolume" );
	suitVolume.Setup( 0.0, 1.0, 0.05f );
	suitVolume.onChanged = CMenuEditable::WriteCvarCb;
	suitVolume.SetCoord( 320, 400 );
	suitVolume.size.w = 300;

	noDSP.szName = L( "Disable DSP effects" );
	noDSP.onChanged = CMenuEditable::WriteCvarCb;
	noDSP.SetCoord( 320, 460 );

	useAlphaDSP.szName = L( "Use Alpha DSP effects" );
	useAlphaDSP.onChanged = CMenuEditable::WriteCvarCb;
	useAlphaDSP.SetCoord( 320, 510 );

	muteFocusLost.szName = L( "Mute when inactive" );
	muteFocusLost.onChanged = CMenuEditable::WriteCvarCb;
	muteFocusLost.SetCoord( 320, 560 );

	vibrationEnable.szName = L( "Enable vibration" );
	vibrationEnable.iMask = (QMF_GRAYED|QMF_INACTIVE);
	vibrationEnable.bInvertMask = true;
	vibrationEnable.onChanged = CMenuCheckBox::BitMaskCb;
	vibrationEnable.onChanged.pExtra = &vibration.iFlags;
	vibrationEnable.SetCoord( 700, 460 );

	vibration.szName = L( "Vibration" );
	vibration.Setup( 0.0f, 5.0f, 0.05f );
	vibration.onChanged = VoidCb( &CMenuAudio::VibrateChanged );
	vibration.SetCoord( 700, 560 );

	AddItem( banner );
	AddButton( L( "Done" ), nullptr, PC_DONE, VoidCb( &CMenuAudio::SaveAndPopMenu ));
	AddItem( soundVolume );
	AddItem( musicVolume );
	AddItem( suitVolume );
	AddItem( noDSP );
	AddItem( useAlphaDSP );
	AddItem( muteFocusLost );
	AddItem( vibrationEnable );
	AddItem( vibration );
}
#endif // XASH_XBOX

void CMenuAudio::_VidInit( )
{
	GetConfig();
}

ADD_MENU( menu_audio, CMenuAudio, UI_Audio_Menu );
