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
#include <algorithm>
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
		unsigned long	_steprate_us;		///< Step rate in microseconds
		unsigned long	_spinup_ms;			///< Spin-up wait time in milliseconds
		unsigned long	_tracks;			///< Number of tracks
		unsigned long	_track_step;		///< Steps per track (1=single-stepped, 2=double-stepped)
		unsigned long	_heads;				///< Number of heads
	public:
		const string drive_type()				{ return _drive_type;		};
		void drive_type(const string x)			{ _drive_type = x;			};
		const string friendly_name()			{ return _friendly_name;	};
		void friendly_name(const string x)		{ _friendly_name = x;		};
		const unsigned long steprate_us()		{ return _steprate_us;		};
		void steprate_us(const unsigned long x)	{ _steprate_us = x;			};
		const unsigned long spinup_ms()			{ return _spinup_ms;		};
		void spinup_ms(const unsigned long x)	{ _spinup_ms = x;			};
		const unsigned long tracks()			{ return _tracks;			};
		void tracks(const unsigned long x)		{ _tracks = x;				};
		const unsigned long track_step()		{ return _track_step;		};
		void track_step(const unsigned long x)	{ _track_step = x;			};
		const unsigned long heads()				{ return _heads;			};
		void heads(const unsigned long x)		{ _heads = x;				};

		/// No-args ctor for CDriveInfo
		CDriveInfo()
		{
		}

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
 * Exception class for DriveSpec parse errors
 */
class EDriveSpecParse : public exception {
	private:
		string x;
		unsigned long _spec;
	public:
		EDriveSpecParse(const string error, const unsigned long specnum) : exception(), x(error), _spec(specnum) { };
		virtual ~EDriveSpecParse() throw() {};

		virtual const char* what() throw() { return x.c_str(); }
		virtual const unsigned long spec() { return _spec; }
};

/**
 * Drive information storage map.
 *
 * Stores a list of all available drive types, mapped by drive type string.
 * The type string is also stored in CDriveInfo for
 */
map<string, CDriveInfo> mDrives;

/// Verbosity flag; true if verbose mode enabled.
int bVerbose = false;

/////////////////////////////////////////////////////////////////////////////

// Pull in the LuaBit library
extern "C" { 
	LUALIB_API int luaopen_bit(lua_State *L);
}

lua_State *SetupLua(void)
{
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

	return L;
}

int LoadDriveScript(const string filename)
{
	int err;

	if (bVerbose) cout << "loading lua script: " << filename;

	// Load the script into Lua
	lua_State *L = SetupLua();
	err = luaL_dofile(L, filename.c_str());
	if (err) {
		cerr << "Error loading Lua script: " << lua_tostring(L, -1) << endl;
		lua_pop(L, 1); // pop error message off of stack
		lua_close(L);	// close down lua
		return -1;
	}

	// Scan through all the Drive Specs in this file -- TODO: error check
	lua_getfield(L, LUA_GLOBALSINDEX, "drivespecs");
	lua_pushnil(L);		// first key
	while (lua_next(L, -2) != 0) {
		// uses 'key' at index -2, and 'value' at index -1

		// Check that 'value' is a table
		try {
			if (lua_istable(L, -1)) {
				// Temporary storage for drivespec fields (CDriveInfo's mandatory parameters are immutable)
				string drivetype = "$$unspecified$$";
				string friendlyname = "$$unspecified$$";
				unsigned int heads = 1,
							 tracks = 40,
							 trackstep = 1,
							 spinup = 1000,
							 steprate = 6000;

				// Now we parse the DriveSpec -- we do this using the same table iteration method we use above
				lua_pushnil(L);		// Initial key
				while (lua_next(L, -2) != 0) {
					// Sort out where this needs to go in our 'temporary drivespec'
					//
					// Get the parameter name and convert it to lower case
					string key = lua_tostring(L, -2);
					transform(key.begin(), key.end(), key.begin(), ::tolower);

					// Convert the key->value pairs into local variables, with error checking
					if (key.compare("drivetype") == 0) {
						// [string] Drive type
						drivetype = lua_tostring(L, -1);
						if (drivetype.length() == 0)
							throw EDriveSpecParse("drivetype not valid.", lua_tointeger(L, -4));
					} else if (key.compare("friendlyname") == 0) {
						// [string] Friendly name
						friendlyname = lua_tostring(L, -1);
						if (friendlyname.length() == 0)
							throw EDriveSpecParse("friendlyname not valid.", lua_tointeger(L, -4));
					} else if (key.compare("heads") == 0) {
						// [integer] Number of heads
						heads = lua_tointeger(L, -1);
						if (heads < 1)
							throw EDriveSpecParse("Value of 'heads' parameter must be an integer greater than zero.", lua_tointeger(L, -4));
					} else if (key.compare("spinup") == 0) {
						// [integer] Spinup time, milliseconds
						spinup = lua_tointeger(L, -1);
					} else if (key.compare("steprate") == 0) {
						// [float] Step rate, milliseconds
						double x = lua_tonumber(L, -1);
						steprate = x * 1000;	// convert from milliseconds to microseconds
						if ((steprate < 250) || (steprate > (255*250)))
							throw EDriveSpecParse("Value of 'steprate' parameter must be between 0.25 and 63.75.", lua_tointeger(L, -4));
					} else if (key.compare("tracks") == 0) {
						// [integer] Number of tracks
						tracks = lua_tointeger(L, -1);
						if (tracks < 1)
							throw EDriveSpecParse("Value of 'tracks' parameter must be an integer greater than zero.", lua_tointeger(L, -4));
					} else if (key.compare("trackstep") == 0) {
						// [integer] Number of physical tracks to step for each logical track
						trackstep = lua_tointeger(L, -1);
						if (trackstep < 1)
							throw EDriveSpecParse("Value of 'trackstep' parameter must be an integer greater than zero.", lua_tointeger(L, -4));
					} else {
						throw EDriveSpecParse("Unrecognised key \"" + key + "\"", lua_tointeger(L, -4));
					}

					// pop value off of stack, leave key for next iteration
					lua_pop(L, 1);
				}

				// Now we have all our keys, try to make a CDriveInfo
				if (drivetype.compare("$$unspecified$$") == 0)
					throw EDriveSpecParse("Drivetype string not specified.", lua_tointeger(L, -2));
				if (friendlyname.compare("$$unspecified$$") == 0)
					throw EDriveSpecParse("Friendlyname string not specified.", lua_tointeger(L, -2));
				CDriveInfo driveinfo(drivetype, friendlyname, steprate, spinup, tracks, trackstep, heads);

				// put the CDriveInfo into the global map
				if (mDrives.find(drivetype) == mDrives.end()) {
					// This drive is not present in the drive map; add it
					mDrives[drivetype] = driveinfo;
				} else {
					throw EDriveSpecParse("Drive type '" + drivetype + "' has already been defined.", lua_tointeger(L, -2));
				}
			} else {
				// This isn't a table... what does the user think they're playing at?
				throw EDriveSpecParse("drivespecs table contains a non-table entity.", lua_tointeger(L, -2));
				lua_close(L);
				return -1;
			}
		} catch (EDriveSpecParse &e) {
			// Parse error / format violation. Let the user know what (we think) they did wrong...
			cerr << "DriveSpec format violation (#" << e.spec() << "): " << e.what() << endl;
			return -1;
		}

		// remove 'value' from stack, keep 'key' for next iteration
		lua_pop(L, 1);
	}

	// pop the table off of the stack
	lua_pop(L, 1);

	// TODO: REMOVEME: test calling ==> should be in a separate function we can use to get drive info
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
