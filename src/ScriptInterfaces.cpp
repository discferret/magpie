// STL headers
#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <algorithm>
#include <dirent.h>

// DiscFerret
#include <discferret/discferret.h>

// Lua 5.1 (PUC-Rio)
#include "lua.hpp"

// Local headers
#include "CDriveInfo.hpp"
#include "ScriptInterfaces.hpp"
#include "EDriveSpecParse.hpp"

using namespace std;

// Pull in the LuaBit library
extern "C" { 
	LUALIB_API int luaopen_bit(lua_State *L);
}

CScriptInterface::CScriptInterface(const std::string filename)
{
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
			cerr << "Error loading Lua script: " << lua_tostring(L, -1) << endl;
			lua_close(L);	// close down lua
			throw -1;		// TODO: throw something better
		}
	}
}

CScriptInterface::~CScriptInterface()
{
	// close down Lua
	lua_close(L);
}

/////////////////////////////////////////////////////////////////////////////

CDriveScript::CDriveScript(const std::string filename) : CScriptInterface(filename)
{
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
			if (!lua_istable(L, -1)) {
				// This isn't a table... what does the user think they're playing at?
				throw EDriveSpecParse("drivespecs table contains a non-table entity.", lua_tointeger(L, -2));
				lua_close(L);
				//TODO! throw something proper
				throw -1;
			}

			// Make sure this is a table, not an array
			if (lua_isnumber(L, -2)) {
				// Key isn't a string identifier.
				throw EDriveSpecParse("drivespecs is an array, not a table.", lua_tointeger(L, -3));
				lua_close(L);
				//TODO! throw something proper
				throw -1;
			}

			// Get the drivetype and store it
			string key = lua_tostring(L, -2);
			svDrivetypes.push_back(key);

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

CDriveInfo CDriveScript::GetDriveInfo(const std::string drivetype)
{
	// get the drivespecs table
	lua_getfield(L, LUA_GLOBALSINDEX, "drivespecs");

	// make sure it's a table
	if (!lua_istable(L, -1)) {
		// TODO: this should be an Internal Error
		throw EDriveSpecParse("DriveSpec script does not contain a 'drivespecs' table.");
	}
	// push the table key and retrieve the entry
	lua_pushstring(L, drivetype.c_str());
	lua_gettable(L, -2);	// get drivespecs[drivetype]

	// make sure the drivespec entry is a table (drivespecs is a table-of-tables)
	if (!lua_istable(L, -1)) {
		throw EDriveSpecParse("DriveSpec entry '" + drivetype + "' is not a table.");
	}

	// Temporary storage for drivespec fields (CDriveInfo's mandatory parameters are immutable)
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
		if (key.compare("friendlyname") == 0) {
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
	if (friendlyname.compare("$$unspecified$$") == 0)
		throw EDriveSpecParse("Friendlyname string not specified.", lua_tointeger(L, -2));
	CDriveInfo driveinfo(drivetype, friendlyname, steprate, spinup, tracks, trackstep, heads);

	return driveinfo;
}

