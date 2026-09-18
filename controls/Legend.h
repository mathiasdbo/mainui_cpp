/*
Legend.h - shared control-hint row for the Xbox port
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
#ifndef MENU_LEGEND_H
#define MENU_LEGEND_H

#include "BaseItem.h"

// Codex review (round 1): every controls/*.cpp file is globbed
// unconditionally by both build systems (wscript, CMakeLists.txt), so
// without this guard the whole class would compile - inertly, but
// compile - on every platform, contradicting the plan's own "Xbox
// only" framing for this item. The guard has to come AFTER the include
// above, not before it, for the same reason Pause.cpp's own header
// comment already documents: XASH_XBOX is defined via build.h, reached
// transitively through BaseItem.h/BaseMenu.h, not a compiler -D flag
// available before any #include runs.
#if XASH_XBOX

// XM.1 item 12 (fork-plan.md): "one shared control every screen draws
// from its own list of (glyph, verb) pairs" - A/B/X/Y as filled discs
// in the pad colours, the D-pad as an outline, START as its own glyph
// on the pause screens. Purely descriptive, same as the plan's own
// wording ("it describes, it does not dispatch") - Utils.h's existing
// key predicates already answer A/B/X, and each screen already wires
// its own D-pad/START handling; this control only draws what a screen
// already does, never a second input path.
enum ELegendGlyph
{
	LEGEND_A = 0,
	LEGEND_B,
	LEGEND_X,
	LEGEND_Y,
	LEGEND_DPAD,
	LEGEND_START,
};

class CMenuLegend : public CMenuBaseItem
{
public:
	typedef CMenuBaseItem BaseClass;

	CMenuLegend();

	void VidInit( void ) override;
	void Draw( void ) override;

	// Real, native 640x480 pixel coordinates - "at 1x" in the plan's own
	// words (fork-plan.md item 12), NOT the virtual 1024x768 space every
	// other SetCoord call in this codebase expects. Main.cpp's own Xbox
	// layout converts real pixel targets into that space explicitly
	// (`(404/480.0)*768.0`, VidInit(bool) below) precisely because plain
	// literals there do not land where their face value suggests; every
	// OTHER Xbox screen this fork has shipped uses plain literals with no
	// such conversion. Rather than getting this wrong once per calling
	// screen, or leaving the legend's own real position dependent on
	// which convention happens to be visible nearby, this control does
	// the same conversion Main.cpp already proved correct, once, here -
	// callers pass the real pixel position they actually want.
	void SetRealCoord( int x, int y );

	// Appends one (glyph, verb) pair, drawn left to right in the order
	// added. MAX_ENTRIES of 5 matches item 12's own note that XM.3's
	// multiplayer screens (the widest legends this control has to carry)
	// share the same five-glyph vocabulary and no screen needs more.
	void Add( ELegendGlyph glyph, const char *verb );

	// Shows or hides an already-added entry, by the order Add() was
	// called in - a hidden entry is skipped entirely in Draw() (its own
	// measured width never enters the running x cursor other entries
	// advance along), so entries after it reflow left rather than
	// leaving a gap. Codex review (round 1): LoadGame's own X (Delete)
	// only does anything while its saves table has focus
	// (CMenuTable::KeyDown, Table.cpp) - showing it unconditionally
	// claimed a screen-wide action that depends on which control is
	// actually focused. LoadGame.cpp's own Think() override toggles this
	// per frame against ItemAtCursor().
	void SetVisible( int index, bool visible );

	static const int MAX_ENTRIES = 5;

private:
	// Codex review (round 1): a fixed per-entry slot clipped a longer
	// translation's own verb text against a shorter language's own
	// assumed width. Draw() now measures each verb's real rendered
	// width and advances a running cursor by it instead - this constant
	// is only the glyph's own fixed size, not a slot width.
	static const int GLYPH_SIZE = 20;

	struct legend_entry_t
	{
		ELegendGlyph glyph;
		const char *verb;
		bool visible;
	};

	legend_entry_t m_entries[MAX_ENTRIES];
	int m_iCount;

	// The real pixel target from SetRealCoord(), re-converted into pos.x/
	// pos.y fresh on every VidInit() - never accumulated, since VidInit()
	// can run again on a later Show() (BaseWindow.cpp's own contract) and
	// mutating pos.x/y in place would drift further off target each time.
	int m_iRealX, m_iRealY;
};

#endif // XASH_XBOX

#endif // MENU_LEGEND_H
