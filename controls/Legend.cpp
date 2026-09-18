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
// another colors.lst key.
//
// Y corrected a second time, 2026-09-18, back to #f0a91c - and the way
// it was wrong is the point. The earlier value (#f0c21b) came from
// rendering the canvas headless and reading a pixel; the artboards'
// own source says `background: #f0a91c` in plain text, in both of the
// two that draw a Y at all (Controller's legend chip, the keyboard's
// keycap badge), and no source anywhere in the canvas or this tree
// contains #f0c21b. A rendered pixel is the design after a compositor
// has touched it; the declaration is the design. Corroborated
// independently by the master controller illustration this fork now
// bakes its Duke diagram from (assets/resource/XboxDuke.png): its Y
// button's own most common texel is #f7a318, six levels from #f0a91c
// and thirty-one from #f0c21b in green alone. A, B and X were read the
// same way this time and are unchanged, matching the artboards exactly.
static const unsigned int s_legendFaceColor[4] =
{
	0xFF6DBE45, // A - green
	0xFFD9322D, // B - red
	0xFF2F79C8, // X - blue
	0xFFF0A91C, // Y - yellow
};

// START's pill is chrome rather than a button face: the canvas draws it
// as a dark fill inside a grey border (`.pb.st`: background #222,
// border 2px solid #777), not in the legend's own text colour the way
// the D-pad cross is. Literals for the same reason as the face colours.
#define LEGEND_START_FILL   0xFF222222
#define LEGEND_START_BORDER 0xFF777777

static const char *const s_legendFaceLetter[4] = { "A", "B", "X", "Y" };

CMenuLegend::CMenuLegend() : BaseClass()
{
	m_iCount = 0;
	m_iRealX = m_iRealY = 0;

	iFlags |= QMF_INACTIVE;
	SetCharSize( QM_SMALLFONT );
	// Nominal only - Draw() below positions every entry from its own
	// measured text width, not from size.w/m_scSize, so this has nothing
	// real to be exact about.
	SetSize( 700, GLYPH_SIZE );
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
no focus state, no click handling. A/B/X/Y draw as a filled disc in the
button's own colour (the one new asset this control needs,
gfx/shell/legend_button.tga - a plain tintable circle, everything else
here is UI_FillRect/UI_DrawRectangleExt, no bitmap) with the letter
centered on top in white for contrast; the D-pad draws as a filled
plus/cross (the canvas's own icon has slightly notched outer corners
this rect-only approximation does not attempt to reproduce); START
draws as an empty outlined pill with no letter or word inside it at
all - checked directly against the canvas, which draws it exactly this
way and puts the verb ("Resume") outside it, same as every other
glyph, rather than labelling the shape itself.

Codex review (round 1): a fixed slot per entry clipped "Move - Adjust"'s
own German translation ("Bewegen - Anpassen", noticeably longer) against
UI_DrawString's own ellipsis-on-overflow. Each entry now advances a
running x cursor by its own measured verb width (GetTextWideScaled(),
the same call Action.cpp's own VidInit uses to size a label from its
text) plus a fixed gap, so a longer translation simply pushes the next
entry over rather than clipping - correct for every language this ever
ships, not just the two checked here. ETF_NOSIZELIMIT is still passed
belt-and-suspenders, since the box width now already equals the
measured text exactly and has no margin of its own to absorb a
rounding difference between this measurement and UI_DrawString's own.
=================
*/
void CMenuLegend::Draw( void )
{
	int i;
	int glyphSize = GLYPH_SIZE * uiStatic.scaleX;
	int glyphGap = (int)( 8 * uiStatic.scaleX );    // glyph -> its own verb
	int entryGap = (int)( 24 * uiStatic.scaleX );   // one entry's verb -> the next glyph
	int x = m_scPos.x;
	int y = m_scPos.y;

	for( i = 0; i < m_iCount; i++ )
	{
		if( !m_entries[i].visible )
			continue;

		int glyphWidth = glyphSize;

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
			// A HOLLOW cross outline, in the legend's own text colour (the
			// D-pad has no single "Duke colour" the way the face buttons
			// do). Was a filled plus until 2026-09-18 - two overlapping
			// bars - which this file's own divergence row already admitted
			// was an approximation of the canvas's outlined icon, and which
			// reads on screen as two crossed strokes rather than the icon
			// the design draws.
			//
			// Drawn as the twelve edges of the plus polygon, because
			// nothing here strokes a path: the canvas's own icon is a 24
			// unit box whose corners sit at 4, 9, 15 and 20, and filling
			// the plus and punching a hole in it is not an option either -
			// what would have to show through is the menu background, an
			// image this control knows nothing about. Each edge is
			// lengthened by the line thickness so the corners meet without
			// a notch.
			const float u = glyphSize / 24.0f;
			const int t = Q_max( 1, (int)( 2.0f * u + 0.5f ));
			const int half = t / 2;
			const int p4  = (int)(  4.0f * u + 0.5f );
			const int p9  = (int)(  9.0f * u + 0.5f );
			const int p15 = (int)( 15.0f * u + 0.5f );
			const int p20 = (int)( 20.0f * u + 0.5f );

			// Six horizontal edges: top of the top arm, top and bottom of
			// the right arm, bottom of the bottom arm, bottom and top of
			// the left arm.
			UI_FillRect( x + p9  - half, y + p4  - half, p15 - p9  + t, t, colorBase );
			UI_FillRect( x + p15 - half, y + p9  - half, p20 - p15 + t, t, colorBase );
			UI_FillRect( x + p15 - half, y + p15 - half, p20 - p15 + t, t, colorBase );
			UI_FillRect( x + p9  - half, y + p20 - half, p15 - p9  + t, t, colorBase );
			UI_FillRect( x + p4  - half, y + p15 - half, p9  - p4  + t, t, colorBase );
			UI_FillRect( x + p4  - half, y + p9  - half, p9  - p4  + t, t, colorBase );

			// Six vertical edges: both sides of the top arm, the right arm's
			// outer edge, both sides of the bottom arm, the left arm's outer
			// edge.
			UI_FillRect( x + p9  - half, y + p4  - half, t, p9  - p4  + t, colorBase );
			UI_FillRect( x + p15 - half, y + p4  - half, t, p9  - p4  + t, colorBase );
			UI_FillRect( x + p20 - half, y + p9  - half, t, p15 - p9  + t, colorBase );
			UI_FillRect( x + p15 - half, y + p15 - half, t, p20 - p15 + t, colorBase );
			UI_FillRect( x + p9  - half, y + p15 - half, t, p20 - p15 + t, colorBase );
			UI_FillRect( x + p4  - half, y + p9  - half, t, p15 - p9  + t, colorBase );
			break;
		}
		case LEGEND_START:
		{
			// Empty on purpose - no letter, no word. Narrower than the
			// face-button slot (the canvas's own pill reads as roughly
			// glyph-height tall by 1.6x wide, not the 2x a boxed "START"
			// label needed).
			glyphWidth = (int)( glyphSize * 1.6f );

			// Filled, then bordered - the canvas draws this pill as a dark
			// fill inside a grey border, not as a bare outline in the
			// legend's text colour (which is what this drew until
			// 2026-09-18). On a black background the fill is nearly
			// invisible; over the in-game scene the pause screens dim
			// behind it, it is what keeps the pill reading as a button.
			UI_FillRect( x, y, glyphWidth, glyphSize, LEGEND_START_FILL );
			UI_DrawRectangleExt( x, y, glyphWidth, glyphSize, LEGEND_START_BORDER, Q_max( 1, (int)( 2 * uiStatic.scaleX ) ), QM_TOP | QM_BOTTOM | QM_LEFT | QM_RIGHT );
			break;
		}
		}

		int textX = x + glyphWidth + glyphGap;
		int textWidth = g_FontMgr->GetTextWideScaled( font, m_entries[i].verb, m_scChSize );

		UI_DrawString( font, textX, y, textWidth, glyphSize, m_entries[i].verb, colorBase, m_scChSize, QM_LEFT | QM_TOP, ETF_NOSIZELIMIT );

		x = textX + textWidth + entryGap;
	}
}

#endif // XASH_XBOX
