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

// XM.1 item 12: the canvas's own exact values for the four face buttons
// (fork-plan.md, item 13's font-size audit) - these are fixed hardware-
// button colours, not a re-themeable UI role like uiColorLegend above,
// so they are literals here rather than another colors.lst key.
static const unsigned int s_legendFaceColor[4] =
{
	0xFF6DBE45, // A - green
	0xFFD9322D, // B - red
	0xFF2F79C8, // X - blue
	0xFFF0A91C, // Y - yellow
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
	m_iCount++;
}

void CMenuLegend::SetVerb( int index, const char *verb )
{
	if( index < 0 || index >= m_iCount )
		return;

	m_entries[index].verb = verb;
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
centered on top in white for contrast; the D-pad draws as an outlined
diamond with no letter; START draws as an outlined box containing the
word itself, since it has no natural single-letter abbreviation the
way the face buttons do.
=================
*/
void CMenuLegend::Draw( void )
{
	int i;
	int glyphSize = GLYPH_SIZE * uiStatic.scaleX;
	int slotWidth = SLOT_WIDTH * uiStatic.scaleX;

	for( i = 0; i < m_iCount; i++ )
	{
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
			// An outlined diamond, not a bitmap - four short strokes
			// meeting at the glyph's own midpoints, in the legend's own
			// text colour (the D-pad has no single "Duke colour" the way
			// the face buttons do).
			int mid = glyphSize / 2;
			int arm = glyphSize / 3;
			int stroke = Q_max( 1, (int)( 2 * uiStatic.scaleX ) );

			UI_FillRect( x + mid - stroke / 2, y, stroke, arm, colorBase );
			UI_FillRect( x + mid - stroke / 2, y + glyphSize - arm, stroke, arm, colorBase );
			UI_FillRect( x, y + mid - stroke / 2, arm, stroke, colorBase );
			UI_FillRect( x + glyphSize - arm, y + mid - stroke / 2, arm, stroke, colorBase );
			break;
		}
		case LEGEND_START:
		{
			glyphWidth = glyphSize * 2;

			UI_DrawRectangleExt( x, y, glyphWidth, glyphSize, colorBase, Q_max( 1, (int)( 2 * uiStatic.scaleX ) ), QM_TOP | QM_BOTTOM | QM_LEFT | QM_RIGHT );
			UI_DrawString( font, x, y, glyphWidth, glyphSize, "START", colorBase, m_scChSize, QM_CENTER );
			break;
		}
		}

		textX = x + glyphWidth + ( 8 * uiStatic.scaleX );
		UI_DrawString( font, textX, y, slotWidth - glyphWidth, glyphSize, m_entries[i].verb, colorBase, m_scChSize, QM_LEFT | QM_TOP );
	}
}
