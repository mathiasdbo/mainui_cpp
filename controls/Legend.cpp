/*
Legend.cpp - shared control-hint row for the Xbox port
Copyright (C) 2026 Resonance3D contributors

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "extdll_menu.h"
#include "BaseMenu.h"
#include "Legend.h"
#include "Utils.h"

// Guard placed after the includes, matching Legend.h's own reasoning
// and Pause.cpp's - XASH_XBOX only resolves once build.h has been
// reached transitively. Legend.h's own body is entirely inside the same
// guard, so this file has nothing to compile without it anyway.
#if XASH_XBOX

// XM.1 item 12: the canvas's own exact values for the four face buttons -
// these are fixed hardware-button colours, not a re-themeable UI role
// like uiColorLegend above, so they are literals here rather than
// another colors.lst key. Sampled directly from the design canvas
// itself (docs/r3d/design/xbox menu/Half-Life Xbox Menu.html, rendered
// headless and read pixel-by-pixel) rather than trusted from an earlier
// note's own transcription - Y's own value there (#f0a91c) turned out
// to be wrong, real value #f0c21b confirmed by sampling the same pixel
// repeatedly across two different legend rows.
static const unsigned int s_legendFaceColor[4] =
{
	0xFF6DBE45, // A - green
	0xFFD9322D, // B - red
	0xFF2F79C8, // X - blue
	0xFFF0C21B, // Y - yellow (corrected from an earlier #f0a91c)
};

static const char *const s_legendFaceLetter[4] = { "A", "B", "X", "Y" };

CMenuLegend::CMenuLegend() : BaseClass()
{
	m_iCount = 0;
	m_iRealX = m_iRealY = 0;

	iFlags |= QMF_INACTIVE;
	SetCharSize( QM_SMALLFONT );
	SetSize( MAX_ENTRIES * SLOT_WIDTH, GLYPH_SIZE );
}

void CMenuLegend::SetRealCoord( int x, int y )
{
	m_iRealX = x;
	m_iRealY = y;
}

void CMenuLegend::VidInit( void )
{
	// Real pixel target -> this codebase's own virtual 1024x768 layout
	// space, the same conversion Main.cpp's own Xbox layout already uses
	// (`(404/480.0)*768.0`) - CalcPosition()/CalcSizes() (BaseClass::VidInit,
	// called below) then scale pos.x/pos.y back down by uiStatic.scaleX/Y
	// (ScreenWidth/1024, ScreenHeight/768) to produce m_scPos, landing
	// back on the exact real pixel target regardless of the actual scale
	// factor - not accumulated: recomputed from m_iRealX/Y fresh every
	// call, since VidInit() can run again on a later Show().
	pos.x = ( m_iRealX / ScreenWidth ) * 1024.0f;
	pos.y = ( m_iRealY / ScreenHeight ) * 768.0f;

	BaseClass::VidInit();

	colorBase.SetDefault( uiColorLegend );
}

void CMenuLegend::Add( ELegendGlyph glyph, const char *verb )
{
	if( m_iCount >= MAX_ENTRIES )
		return;

	m_entries[m_iCount].glyph = glyph;
	m_entries[m_iCount].verb = verb;
	m_entries[m_iCount].visible = true;
	m_iCount++;
}

void CMenuLegend::SetVisible( int index, bool visible )
{
	if( index < 0 || index >= m_iCount )
		return;

	m_entries[index].visible = visible;
}

/*
=================
CMenuLegend::Draw

Purely descriptive (item 12's own "it describes, it does not dispatch") -
no focus state, no click handling, drawn at a fixed slot per entry
rather than measuring each verb string: this row carries no input
geometry for that measurement to serve. A/B/X/Y draw as a filled disc
in the button's own colour (the one new asset this control needs,
gfx/shell/legend_button.tga - a plain tintable circle, everything else
here is UI_FillRect/UI_DrawRectangleExt, no bitmap) with the letter
centered on top in white for contrast; the D-pad draws as a filled
plus/cross (the canvas's own icon has slightly notched outer corners
this rect-only approximation does not attempt to reproduce); START
draws as an empty outlined pill with no letter or word inside it at
all - checked directly against the canvas, which draws it exactly this
way and puts the verb ("Resume") outside it, same as every other
glyph, rather than labelling the shape itself.
=================
*/
void CMenuLegend::Draw( void )
{
	int i;
	int glyphSize = GLYPH_SIZE * uiStatic.scaleX;
	int slotWidth = SLOT_WIDTH * uiStatic.scaleX;

	for( i = 0; i < m_iCount; i++ )
	{
		if( !m_entries[i].visible )
			continue;

		int x = m_scPos.x + i * slotWidth;
		int y = m_scPos.y;
		int glyphWidth = glyphSize;
		int textX;

		switch( m_entries[i].glyph )
		{
		case LEGEND_A:
		case LEGEND_B:
		case LEGEND_X:
		case LEGEND_Y:
		{
			int idx = m_entries[i].glyph - LEGEND_A;

			UI_DrawPic( x, y, glyphSize, glyphSize, s_legendFaceColor[idx], "gfx/shell/legend_button" );
			UI_DrawString( font, x, y, glyphSize, glyphSize, s_legendFaceLetter[idx], uiColorWhite, m_scChSize, QM_CENTER );
			break;
		}
		case LEGEND_DPAD:
		{
			// A filled plus/cross - two overlapping bars, in the legend's
			// own text colour (the D-pad has no single "Duke colour" the
			// way the face buttons do).
			int armWidth = Q_max( 2, (int)( glyphSize * 0.34f ) );
			int barOffset = ( glyphSize - armWidth ) / 2;

			UI_FillRect( x + barOffset, y, armWidth, glyphSize, colorBase );
			UI_FillRect( x, y + barOffset, glyphSize, armWidth, colorBase );
			break;
		}
		case LEGEND_START:
		{
			// Empty on purpose - no letter, no word. Narrower than the
			// face-button slot (the canvas's own pill reads as roughly
			// glyph-height tall by 1.6x wide, not the 2x a boxed "START"
			// label needed).
			glyphWidth = (int)( glyphSize * 1.6f );

			UI_DrawRectangleExt( x, y, glyphWidth, glyphSize, colorBase, Q_max( 1, (int)( 2 * uiStatic.scaleX ) ), QM_TOP | QM_BOTTOM | QM_LEFT | QM_RIGHT );
			break;
		}
		}

		textX = x + glyphWidth + ( 8 * uiStatic.scaleX );
		UI_DrawString( font, textX, y, slotWidth - glyphWidth, glyphSize, m_entries[i].verb, colorBase, m_scChSize, QM_LEFT | QM_TOP );
	}
}

#endif // XASH_XBOX
