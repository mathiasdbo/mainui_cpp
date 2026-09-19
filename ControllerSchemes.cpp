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
#include "extdll_menu.h"
#include "BaseMenu.h"
#include "build.h"
#include "ControllerSchemes.h"
#include "miniutl.h"

// XM.1 item 5 (fork-plan.md) is Xbox-only content - "scheme" has no PC
// meaning, and this whole file compiles to an empty translation unit
// everywhere else; full account in this fork's own divergences.md #82.
// Guard placed AFTER every include, matching this fork's own established
// convention (divergences.md #72 already found the opposite order
// compiles silently to an empty TU and fails only at link time, not at
// compile time).
#if XASH_XBOX

struct SchemeBind_t
{
	int keynum;
	const char *command;
};

// Every scheme's own bind table deliberately excludes Start - it is
// locked to `cancelselect` in every scheme (fork-plan.md item 5;
// deps/hlsdk/cl_dll/ammo.cpp:753-763's own CHudAmmo::UserCmd_Close is
// the only pad route from gameplay back to the pause menu) and
// UI_ApplyControllerScheme() re-pins it unconditionally, separately from
// these tables, so it is never a useful detection signal either.

// Standard (fork-plan.md item 5): the scheme this fork wants new players
// to have by default, fixing two real stock collisions
// (engine/client/input/in_keys.c:107-127): R1_BUTTON stock-bound to
// +attack is Black, a large face button, not a shoulder bumper the
// original "Duke" controller has at all - confirmed against the real
// hardware's own USB HID driver, not guessed:
// deps/nxdk/lib/sdl/SDL2/src/joystick/xbox/SDL_xboxjoystick.c:532-533
// maps Black to XINPUT_GAMEPAD_RIGHT_SHOULDER (R1_BUTTON) and White to
// XINPUT_GAMEPAD_LEFT_SHOULDER (L1_BUTTON). +speed and +attack2 were
// also each stock-bound from three different physical inputs.
static const SchemeBind_t s_standardBinds[] =
{
	{ K_JOY2,        "+attack"      }, // RTRIGGER (analog) - Fire
	{ K_R2_BUTTON,   "+attack"      }, // digital-threshold twin of RTRIGGER
	{ K_JOY1,        "+attack2"     }, // LTRIGGER (analog) - Secondary fire
	{ K_L2_BUTTON,   "+attack2"     }, // digital-threshold twin of LTRIGGER
	{ K_A_BUTTON,    "+jump"        },
	{ K_LSTICK,      "+duck"        }, // left stick click - Crouch
	{ K_B_BUTTON,    "+use"         },
	{ K_X_BUTTON,    "+reload"      },
	{ K_Y_BUTTON,    "invnext"      }, // Next weapon
	{ K_L1_BUTTON,   "invprev"      }, // White - Previous weapon
	{ K_R1_BUTTON,   "impulse 100"  }, // Black - Flashlight
	{ K_RSTICK,      "+speed"       }, // right stick click - Walk
	// Weapon slots on the D-pad: fork-plan.md item 5 names "D-pad" for
	// this row but not which slot goes where - this clockwise-from-up
	// order (1/2/3/4) is this branch's own judgment call, easy to
	// revisit in review; deps/hlsdk/cl_dll/ammo.cpp:276-285 confirms
	// slot1..slot10 are the real client commands.
	{ K_DPAD_UP,     "slot1"        },
	{ K_DPAD_RIGHT,  "slot2"        },
	{ K_DPAD_DOWN,   "slot3"        },
	{ K_DPAD_LEFT,   "slot4"        },
	{ K_BACK_BUTTON, "save quick"   }, // Quick save - kb_def.lst's own F6 bind
};

// Southpaw is Standard with only the stick assignment swapped
// (fork-plan.md: "Standard with the sticks swapped in joy_axis_binding") -
// identical bind table, different axis string only (see s_schemes below).
#define s_southpawBinds s_standardBinds

// Legacy: engine/client/input/in_keys.c:107-127's own stock defaults,
// verbatim, so a player who already has them keeps them by name -
// including their own real quirks preserved on purpose: Black/R1_BUTTON
// already meant Fire here, and White/L1_BUTTON duplicates Crouch
// alongside the right stick click.
static const SchemeBind_t s_legacyBinds[] =
{
	{ K_A_BUTTON,    "+jump"        },
	{ K_B_BUTTON,    "+use"         },
	{ K_X_BUTTON,    "+reload"      },
	{ K_Y_BUTTON,    "impulse 100"  },
	{ K_BACK_BUTTON, "pause"        },
	{ K_LSTICK,      "+speed"       },
	{ K_RSTICK,      "+duck"        },
	{ K_L1_BUTTON,   "+duck"        },
	{ K_R1_BUTTON,   "+attack"      },
	{ K_DPAD_UP,     "impulse 201"  },
	{ K_DPAD_DOWN,   "lastinv"      },
	{ K_DPAD_LEFT,   "invprev"      },
	{ K_DPAD_RIGHT,  "invnext"      },
	{ K_L2_BUTTON,   "+speed"       },
	{ K_R2_BUTTON,   "+attack2"     },
	{ K_JOY1,        "+speed"       },
	{ K_JOY2,        "+attack2"     },
};

// engine/client/input/in_joy.c:60-61's own default and description -
// "s - side, f - forward, y - yaw, p - pitch, r - left trigger,
// l - right trigger", one character per PHYSICAL axis index (fixed by
// SDL: 0/1 = left stick X/Y, 2/3 = right stick X/Y, 4/5 = triggers).
// Standard/Legacy keep the engine's own compiled default (left stick
// moves, right stick looks); Southpaw swaps which STICK does which job
// (not the axes within a stick): axis0/1 (left stick) go from side/
// forward to yaw/pitch, axis2/3 (right stick) go from pitch/yaw to
// forward/side, axis4/5 (the triggers) are unaffected.
#define AXIS_BINDING_DEFAULT  "sfpyrl"
#define AXIS_BINDING_SOUTHPAW "ypfsrl"

struct SchemeTable_t
{
	const SchemeBind_t *binds;
	int count;
	const char *axisBinding;
};

static const SchemeTable_t s_schemes[CONTROLLER_SCHEME_COUNT] =
{
	{ s_standardBinds, V_ARRAYSIZE( s_standardBinds ), AXIS_BINDING_DEFAULT  },
	{ s_southpawBinds, V_ARRAYSIZE( s_southpawBinds ), AXIS_BINDING_SOUTHPAW },
	{ s_legacyBinds,   V_ARRAYSIZE( s_legacyBinds ),   AXIS_BINDING_DEFAULT  },
};

const char *g_pszControllerSchemeNames[CONTROLLER_SCHEME_COUNT] =
{
	"Standard",
	"Southpaw",
	"Legacy",
};

void UI_ApplyControllerScheme( EControllerScheme scheme )
{
	if( scheme < 0 || scheme >= CONTROLLER_SCHEME_COUNT )
		return; // SCHEME_CUSTOM or garbage - nothing to apply, by design

	const SchemeTable_t &table = s_schemes[scheme];

	for( int i = 0; i < table.count; i++ )
	{
		EngFuncs::ClientCmdF( true, "bind \"%s\" \"%s\"\n",
			EngFuncs::KeynumToString( table.binds[i].keynum ),
			table.binds[i].command );
	}

	// Setting this cvar alone is enough - Joy_FinalizeMove
	// (engine/client/input/in_joy.c) re-parses it into joyaxesmap[] AND
	// clears every cached joyaxis[] value the moment it sees this change,
	// so a stick still displaced from navigating to this button cannot
	// leave a stale reading latched under its new logical role (Codex
	// review found this exact race live in this branch's own history;
	// the fix lives in the engine, not here, since joyaxis[] is that
	// file's own static state - full account in divergences.md #82).
	EngFuncs::CvarSetString( "joy_axis_binding", table.axisBinding );

	// Start is never part of any scheme's own table above - it is the
	// one pad route from gameplay back to the pause menu and stays
	// locked to cancelselect regardless of which scheme (or Custom) is
	// active, re-pinned here unconditionally every time a scheme applies.
	EngFuncs::ClientCmdF( true, "bind \"%s\" \"cancelselect\"\n",
		EngFuncs::KeynumToString( K_START_BUTTON ));
}

EControllerScheme UI_DetectControllerScheme( void )
{
	const char *axisBinding = EngFuncs::GetCvarString( "joy_axis_binding" );

	for( int scheme = 0; scheme < CONTROLLER_SCHEME_COUNT; scheme++ )
	{
		const SchemeTable_t &table = s_schemes[scheme];

		if( strcmp( axisBinding, table.axisBinding ))
			continue;

		bool matches = true;
		for( int i = 0; i < table.count && matches; i++ )
		{
			const char *bound = EngFuncs::KEY_GetBinding( table.binds[i].keynum );
			if( !bound || strcmp( bound, table.binds[i].command ))
				matches = false;
		}

		if( matches )
			return (EControllerScheme)scheme;
	}

	return SCHEME_CUSTOM;
}

#endif // XASH_XBOX
