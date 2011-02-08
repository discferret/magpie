/****************************************************************************
 * DiscFerret Image Acquisition Tool -- Xacqt! (eXtensible ACQuisition Tool)
 *
 * (C) 2011 Philip Pemberton. All rights reserved.
 *
 * Distributed under the GNU General Public Licence Version 2, see the file
 * 'COPYING' for distribution restrictions.
 ****************************************************************************/

// C++ stdlib
#include <cstdio>
#include <vector>
#include <string>
#include <map>
#include <iostream>
#include <getopt.h>

// DiscFerret
#include <discferret/discferret.h>

// Lua 5.1 (PUC-Rio)
#include "lua.hpp"

using namespace std;

// Default script directory
#ifndef SCRIPTDIR
#define SCRIPTDIR "./scripts"
#endif

/**
 * @brief	Drive information class
 *
 * Used to store information about a drive.
 */
class CDriveInfo {
	private:
		string			_drive_type;		///< Drive type string (immutable)
		string			_friendly_name;		///< Friendly name (displayed to user)
		unsigned long	_steprate_us;		///< Step rate in milliseconds
		unsigned long	_spinup_ms;			///< Spin-up wait time in milliseconds
		unsigned long	_tracks;			///< Number of tracks
		unsigned long	_track_step;		///< Steps per track (1=single-stepped, 2=double-stepped)
		unsigned long	_heads;				///< Number of heads
	public:
		const string drive_type()			{ return _drive_type;		};
		const string friendly_name()		{ return _friendly_name;	};
		const unsigned long steprate_us()	{ return _steprate_us;		};
		const unsigned long spinup_ms()		{ return _spinup_ms;		};
		const unsigned long tracks()		{ return _tracks;			};
		const unsigned long track_step()	{ return _track_step;		};
		const unsigned long heads()			{ return _heads;			};

		/**
		 * ctor for CDriveInfo.
		 *
		 * @param	drive_type		Drive type string.
		 * @param	friendly_name	Friendly name
		 * @param	step_rate_us	Step rate in microseconds
		 * @param	spinup_ms		Spin-up time in milliseconds
		 * @param	tracks			Number of tracks on drive
		 * @param	track_step		Track stepping (1=single-stepped, 2=double-stepped)
		 * @param	heads			Number of heads on drive
		 */
		CDriveInfo(
				string drive_type, string friendly_name,
				unsigned long step_rate_us,
				unsigned long spinup_ms,
				unsigned long tracks,
				unsigned long track_step,
				unsigned long heads)
		{
			_drive_type		= drive_type;
			_friendly_name	= friendly_name;
			_steprate_us	= step_rate_us;
			_spinup_ms		= spinup_ms;
			_tracks			= tracks;
			_track_step		= track_step;
			_heads			= heads;
		}
};

/**
 * Drive information storage map.
 *
 * Stores a list of all available drive types, mapped by drive type string.
 * The type string is also stored in CDriveInfo for
 */
map<string, CDriveInfo> drives;

/// Verbosity flag; true if verbose mode enabled.
int bVerbose = false;

/////////////////////////////////////////////////////////////////////////////

// Pull in the LuaBit library
extern "C" { 
	LUALIB_API int luaopen_bit(lua_State *L);
}

int LoadDriveScript(const string filename)
{
	int err;

	if (bVerbose) cout << "loading lua script: " << filename;

	// Set up Lua
	// TODO: error checking!
	lua_State *L = luaL_newstate();
	luaL_openlibs(L);
	luaopen_bit(L);

	// Set some base constants
	struct { string name; unsigned long val; } LCONSTS[] = {
		{ "PIN_DENSITY",		DISCFERRET_DRIVE_CONTROL_DENSITY	},
		{ "PIN_INUSE",			DISCFERRET_DRIVE_CONTROL_INUSE		},
		{ "PIN_DS0",			DISCFERRET_DRIVE_CONTROL_DS0		},
		{ "PIN_DS1",			DISCFERRET_DRIVE_CONTROL_DS1		},
		{ "PIN_DS2",			DISCFERRET_DRIVE_CONTROL_DS2		},
		{ "PIN_DS3",			DISCFERRET_DRIVE_CONTROL_DS3		},
		{ "PIN_MOTEN",			DISCFERRET_DRIVE_CONTROL_MOTEN		},
		{ "PIN_SIDESEL",		DISCFERRET_DRIVE_CONTROL_SIDESEL	},
		{ "STATUS_INDEX",		DISCFERRET_STATUS_INDEX				},
		{ "STATUS_TRACK0",		DISCFERRET_STATUS_TRACK0			},
		{ "STATUS_WRPROT",		DISCFERRET_STATUS_WRITE_PROTECT		},
		{ "STATUS_READY_DCHG",	DISCFERRET_STATUS_DISC_CHANGE		},
		{ "STATUS_DENSITY",		DISCFERRET_STATUS_DENSITY			}
	};
	for (size_t i=0; i<(sizeof(LCONSTS)/sizeof(LCONSTS[0])); i++) {
		lua_pushnumber(L, LCONSTS[i].val);
		lua_setglobal(L, LCONSTS[i].name.c_str());
	}

	// Load the script into Lua
	err = luaL_dofile(L, filename.c_str());
	if (err) {
		cerr << "Error loading Lua script: " << lua_tostring(L, -1) << endl;
		lua_pop(L, 1); // pop error message off of stack
		lua_close(L);	// close down lua
		return -1;
	}

	// TODO: set up the drive specs

	// TODO: REMOVEME: test calling
	lua_getfield(L, LUA_GLOBALSINDEX, "isDriveReady");
	lua_pushstring(L, "cdc-94205-51");	// TODO: push drivetype here
	lua_pushnumber(L, DISCFERRET_STATUS_DENSITY + DISCFERRET_STATUS_DISC_CHANGE);	// TODO: push status here
	err = lua_pcall(L, 2, 1, 0);
	if (err) {
		cerr << "Error calling isDriveReady: " << lua_tostring(L, -1) << endl;
		lua_pop(L, 1);	// pop error message off of stack
	} else {
		cout << "isDriveReady returns: " << lua_toboolean(L, -1) << endl;
		lua_pop(L, 1);	// pop result off of stack
	}

	// close down Lua
	lua_close(L);

	return 0;
}

int main(int argc, char **argv)
{
	while (1) {
		// Getopt option table
		static const struct option opts_long[] = {
			// name			has_arg				flag			val
			{"verbose",		no_argument,		&bVerbose,		true},
			{"drive",		required_argument,	0,				'd'},
			{"format",		required_argument,	0,				'f'},
			{0, 0, 0, 0}	// end sentinel / terminator
		};
		static const char *opts_short = "d:f:";

		// getopt stores the option index here
		int idx = 0;
		int c;
		if ((c = getopt_long(argc, argv, opts_short, opts_long, &idx)) == -1)
			break;

		// check option value
		switch (c) {
			case 0:	break;			// option set a flag (ignore this)

			case 'd':
				// set drive type TODO

			case 'f':
				// set format TODO
				printf("option %c", c);
				if (optarg) printf(" with arg %s\n", optarg);
					else printf("\n");
				break;

			case '?':
				// option unknown; getopt already printed the error, but we need to bail out here.
				exit(EXIT_FAILURE);
				break;

			default:
				// unhandled option -- an option we should have handled, but didn't!
				printf("unhandled option %c! man the lifeboats, women and children first!\n", c);
				abort();
				break;
		}
	}

	if (bVerbose) cout << "Verbose mode ON\n";

	// TODO: make sure drivetype and format have been specified

	// TODO: Scan through the available scripts (getdir etc.)
	LoadDriveScript("scripts/drive/winchester-cdc.lua");

	return 0;
}
