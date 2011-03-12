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
#include <dirent.h>

// DiscFerret
#include <discferret/discferret.h>

// Lua 5.1 (PUC-Rio)
#include "lua.hpp"

// Local headers
#include "CDriveInfo.hpp"
#include "EDriveSpecParse.hpp"

using namespace std;

// Default script directories
#ifndef DRIVESCRIPTDIR
#define DRIVESCRIPTDIR "./scripts/drive"
#endif

/// Verbosity flag; true if verbose mode enabled.
int bVerbose = false;

/////////////////////////////////////////////////////////////////////////////

// Pull in the LuaBit library
extern "C" { 
	LUALIB_API int luaopen_bit(lua_State *L);
}

/**
 * Interface and common code for script loading.
 */
class CScriptInterface {
	protected:
		lua_State *L;

	public:
		CScriptInterface(const std::string filename) {
//			SetupLua(filename);
			int err;

			// Set up Lua
			// TODO: error checking! throw exception if something goes wrong!
			L = luaL_newstate();
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

			if (filename.length() > 0) {
				// Load the script into Lua
				err = luaL_dofile(L, filename.c_str());
				if (err) {
					lua_pop(L, 1); // pop error message off of stack
					lua_close(L);	// close down lua
					cerr << "Error loading Lua script: " << lua_tostring(L, -1) << endl;
				}
			}
		}

		~CScriptInterface() {
			// close down Lua
			lua_close(L);
		}
};

class CDriveScript : public CScriptInterface {
	private:
		/// List of drive types understood by this script
		map<string, CDriveInfo> mDrives;

	public:
		CDriveScript(const std::string filename) : CScriptInterface(filename) {
			// Scan through all the Drive Specs in this file -- TODO: error check
			try {
				lua_getfield(L, LUA_GLOBALSINDEX, "drivespecs");
				if (!lua_istable(L, -1)) {
					throw EDriveSpecParse("DriveSpec script does not contain a 'drivespecs' table.");
				}
				lua_pushnil(L);		// first key
				while (lua_next(L, -2) != 0) {
					// uses 'key' at index -2, and 'value' at index -1

					// Check that 'value' is a table
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
						//TODO! throw something proper
						throw -1;
					}
						// remove 'value' from stack, keep 'key' for next iteration
					lua_pop(L, 1);
				}

				// pop the table off of the stack
				lua_pop(L, 1);
			} catch (EDriveSpecParse &e) {
				// Parse error / format violation. Let the user know what (we think) they did wrong...
				if (e.spec() == -1)
					cerr << "[" << filename << "]: DriveSpec format violation: " << e.what() << endl;
				else
					cerr << "[" << filename << ", block " << e.spec() << "]: DriveSpec format violation: " << e.what() << endl;
				//TODO! throw something proper
				throw -1;
			}

		}

		/**
		 * @brief	Lua wrapper function for IsDriveReady() DriveSpec function
		 */
		bool isDriveReady(const string drivetype, const unsigned long status)
		{
			bool result;
			int err;

			// TODO: check drive type is in mDrives?

			lua_getfield(L, LUA_GLOBALSINDEX, "isDriveReady");
			lua_pushstring(L, drivetype.c_str());	// drive type string
			lua_pushnumber(L, status);				// status value from discferret_get_status()
			err = lua_pcall(L, 2, 1, 0);	// 2 parameters, 1 return value
			if (err) {
				// TODO: throw exception on error --> remove cerr!
				cerr << "Error calling isDriveReady: " << lua_tostring(L, -1) << endl;
				lua_pop(L, 1);	// pop error message off of stack
				return false;
			} else {
				result = lua_toboolean(L, -1);
				lua_pop(L, 1);	// pop result off of stack
				return result;
			}
		}

		/**
		 * @brief	Lua wrapper function for IsDriveReady() DriveSpec function
		 */
		bool getDriveOutputs(const string drivetype, const unsigned long track, const unsigned long head, const unsigned long sector)
		{
			bool result;
			int err;

			// TODO: check drive type is in mDrives?

			lua_getfield(L, LUA_GLOBALSINDEX, "getDriveOutputs");
			lua_pushstring(L, drivetype.c_str());	// drive type string
			lua_pushnumber(L, track);				// physical track
			lua_pushnumber(L, head);				// physical head
			lua_pushnumber(L, sector);				// physical sector
			err = lua_pcall(L, 4, 1, 0);			// 4 parameters, 1 return value
			if (err) {
				// TODO: throw exception on error
				cerr << "Error calling getDriveOutputs: " << lua_tostring(L, -1) << endl;
				lua_pop(L, 1);	// pop error message off of stack
				return false;
			} else {
				result = lua_toboolean(L, -1);
				lua_pop(L, 1);	// pop result off of stack
				return result;
			}
		}
};

/***
 * TODO: turn this into an interface:
 *   CScriptManager::Load --> virtual =0
 *   CScriptManager::LoadDir --> code from TraverseScriptDir(), calls CSM::Load
 *
 *   CDriveScriptManager --> extend CGenericScriptManager
 *     --> implement Load() to create a CDriveScript and merge drivescript list from that with the root list
 *   CFormatScriptManager --> s.a.a. but create CFormatScript objects instead
 */
class CScriptManager {
	private:
		/**
		 * Drive information storage map.
		 *
		 * Stores a list of all available drive types, mapped by drive type string.
		 * The type string is also stored in CDriveInfo for
		 */
		map<string, CDriveInfo> mDrives;

	public:
		int LoadDriveScript(const string filename)
		{
			if (bVerbose) cout << "loading drivescript: " << filename << endl;
			CDriveScript script(filename);	// TODO: CAN_THROW --> catch exception

			// TODO: merge script's drivetype list with CDS's list

			return 0;
		}


};

#if 0
void TraverseScriptDir(const string path)
{
	DIR *dp;
	struct dirent *dt;
	dp = opendir(path.c_str());
	while ((dt = readdir(dp)) != NULL) {
		string filename = path + "/";
		// skip hidden files
		if (dt->d_name[0] == '.') continue;
		// add filename to get fully qualified path
		filename += dt->d_name;

		// What is this directory entry?
		if (dt->d_type == DT_DIR) {
			// it's a subdirectory...
			cout << "traversing directory: " << filename <<endl;
			TraverseScriptDir(filename);
		} else if (dt->d_type == DT_REG) {
			// skip files which aren't lua scripts
			if (filename.substr(filename.length()-4, 4).compare(".lua") != 0) continue;

			// it's a regular file... load it as a script.
			LoadDriveScript(filename);
		}
	}
	closedir(dp);

}



//////////////////////////////////////////////////////////////////////////////
// "I am main(), king of kings. Look upon my works, ye mighty, and despair!"
//////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
	while (1) {
		// Getopt option table
		static const struct option opts_long[] = {
			// name			has_arg				flag			val
			{"verbose",		no_argument,		&bVerbose,		true},
			{"drive",		required_argument,	0,				'd'},
			{"format",		required_argument,	0,				'f'},
			{"serial",		required_argument,	0,				's'},
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

			case 's':
				// discferret unit serial number TODO
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
	TraverseScriptDir(DRIVESCRIPTDIR);

	return 0;
}
#else

int main(void)
{
	return 0;
}

#endif
