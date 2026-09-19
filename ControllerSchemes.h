/*
Copyright (C) 2026 Resonance3D contributors

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

*/
#pragma once
#ifndef CONTROLLERSCHEMES_H
#define CONTROLLERSCHEMES_H

#include "build.h"

// XM.1 item 5 (fork-plan.md) - controller schemes; full account in this
// fork's own divergences.md #82. Xbox-only: a "scheme" is a complete
// controller profile (a bind table PLUS joy_axis_binding), applied
// through the existing cvar/bind systems, never a second store - see
// docs/r3d/divergences.md #53 for why applying one must never call
// unbindall. Guarded here too, not just in the .cpp - a non-Xbox caller
// that somehow reached one of these symbols would rather fail to compile
// (undeclared identifier) than link against nothing.
#if XASH_XBOX

// SCHEME_CUSTOM is a detection-only state - there is no stored "Custom"
// table, it just means the live config matched none of the other three
// exactly (a rebind departed from every preset). Never pass it to
// UI_ApplyControllerScheme() - there is nothing to apply; a caller
// wanting "apply the active scheme, or Standard if it's Custom" (the
// Customize screen's own Y-button behaviour, fork-plan.md item 5) must
// resolve SCHEME_CUSTOM to SCHEME_STANDARD itself before calling.
enum EControllerScheme
{
	SCHEME_STANDARD = 0,
	SCHEME_SOUTHPAW,
	SCHEME_LEGACY,
	SCHEME_CUSTOM
};

// SCHEME_CUSTOM has no table of its own - it is not one of the schemes
// this count covers.
#define CONTROLLER_SCHEME_COUNT 3

// Issues one `bind` per the scheme's table entry, sets joy_axis_binding,
// and re-pins Start to cancelselect - never unbindall. A garbage or
// SCHEME_CUSTOM argument is a no-op, by design (see above).
void UI_ApplyControllerScheme( EControllerScheme scheme );

// Reads every scheme-relevant bind plus joy_axis_binding back out of the
// live config and returns whichever of Standard/Southpaw/Legacy matches
// EXACTLY (checked in that order, first match wins), or SCHEME_CUSTOM if
// none does. Read-only - safe to call on every screen entry.
EControllerScheme UI_DetectControllerScheme( void );

// One display name per real scheme, in enum order - plain literal text,
// not yet an L() localization key: none of "Standard"/"Southpaw"/
// "Legacy" exist in MenuStrings.cpp's compiled table today, and whether/
// how to add them is the consuming screen's own call to make, not this
// shared model's (avoiding divergences.md #72's own already-documented
// mistake of guessing at an unverified key).
extern const char *g_pszControllerSchemeNames[CONTROLLER_SCHEME_COUNT];

#endif // XASH_XBOX
#endif // CONTROLLERSCHEMES_H
