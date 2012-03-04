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
#include "Exceptions.hpp"
#include "ScriptInterfaces.hpp"

using namespace std;

// Pull in the LuaBit library
extern "C" { 
	LUALIB_API int luaopen_bit(lua_State *L);
}

CScriptInterface::CScriptInterface(const std::string _filename)
{
	int err;

	filename = _filename;

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
			const char *errstr = lua_tostring(L, -1);
			lua_pop(L, 1);	// pop error message from stack
			lua_close(L);	// close down lua
			throw ELuaError(errstr);
		}
	}
}

CScriptInterface::~CScriptInterface()
{
	// close down Lua
	lua_close(L);
}

/////////////////////////////////////////////////////////////////////////////

CDriveScript::CDriveScript(const std::string _filename) : CScriptInterface(_filename)
{
	// Scan through all the Drive Specs in this file -- TODO: error check
	try {
		lua_getfield(L, LUA_GLOBALSINDEX, "drivespecs");
		if (!lua_istable(L, -1)) {
			throw EDriveSpecParse("DriveSpec script does not contain a 'drivespecs' table.", filename);
		}
		lua_pushnil(L);		// first key
		while (lua_next(L, -2) != 0) {
			// uses 'key' at index -2, and 'value' at index -1

			// Make sure this is a table, not an array
			if (lua_isnumber(L, -2)) {
				// Key isn't a string identifier. This isn't a table.
				lua_close(L);
				throw EDriveSpecParse("drivespecs must be a table, not a numerically-indexed array.", filename);
			}

			// Check that 'value' is a table
			if (!lua_istable(L, -1)) {
				// This isn't a table... what does the user think they're playing at?
				const char *specname = lua_tostring(L, -2);
				lua_close(L);
				throw EDriveSpecParse("drivespecs table contains a non-table entity.", filename, specname);
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
		// TODO: shouldn't use cerr here; we should let the host handle this
/*		if (string(e.spec()).length() == 0)
			cerr << "[" << filename << "]: DriveSpec format violation: " << e.what() << endl;
		else
			cerr << "[" << filename << ", block " << e.spec() << "]: DriveSpec format violation: " << e.what() << endl;
*/
		throw;
	}
}

CDriveInfo CDriveScript::GetDriveInfo(const std::string drivetype)
{
	// get the drivespecs table
	lua_getfield(L, LUA_GLOBALSINDEX, "drivespecs");

	// make sure it's a table
	if (!lua_istable(L, -1)) {
		// This is an Internal Error because the ctor checks this...!
		throw EInternalScriptingError("DriveSpec script does not contain a 'drivespecs' table, but it has already been loaded.", filename);
	}

	// push the table key and retrieve the entry
	lua_pushstring(L, drivetype.c_str());
	lua_gettable(L, -2);	// get drivespecs[drivetype]

	// make sure the drivespec entry is a table (drivespecs is a table-of-tables)
	if (!lua_istable(L, -1)) {
		// This is an Internal Error because the ctor checks this...!
		throw EInternalScriptingError("DriveSpec entry '" + drivetype + "' is not a table.", filename);
	}

	// Temporary storage for drivespec fields (CDriveInfo's mandatory parameters are immutable)
	string friendlyname = "$$unspecified$$";
	unsigned int heads = 1,
				 tracks = 40,
				 spinup = 1000,
				 steprate = 6000;
	float tpi = 0;

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
				throw EDriveSpecParse("friendlyname not valid.", filename, lua_tostring(L, -4));
		} else if (key.compare("heads") == 0) {
			// [integer] Number of heads
			heads = lua_tointeger(L, -1);
			if (heads < 1)
				throw EDriveSpecParse("Value of 'heads' parameter must be an integer greater than zero.", filename, lua_tostring(L, -4));
		} else if (key.compare("spinup") == 0) {
			// [integer] Spinup time, milliseconds
			spinup = lua_tointeger(L, -1);
		} else if (key.compare("steprate") == 0) {
			// [float] Step rate, milliseconds
			double x = lua_tonumber(L, -1);
			steprate = x * 1000;	// convert from milliseconds to microseconds
			if (steprate < 0)
				throw EDriveSpecParse("Value of 'steprate' parameter must be greater than zero.", filename, lua_tostring(L, -4));
		} else if (key.compare("tracks") == 0) {
			// [integer] Number of tracks
			tracks = lua_tointeger(L, -1);
			if (tracks < 1)
				throw EDriveSpecParse("Value of 'tracks' parameter must be an integer greater than zero.", filename, lua_tostring(L, -4));
		} else if (key.compare("tpi") == 0) {
			// [float] Number of physical tracks per inch
			tpi = lua_tonumber(L, -1);
			if (tpi < 0)
				throw EDriveSpecParse("Value of 'tpi' parameter must be greater than or equal to zero.", filename, lua_tostring(L, -4));
		} else {
			throw EDriveSpecParse("Unrecognised key \"" + key + "\"", filename, lua_tostring(L, -4));
		}

		// pop value off of stack, leave key for next iteration
		lua_pop(L, 1);
	}

	// Now we have all our keys, try to make a CDriveInfo
	if (friendlyname.compare("$$unspecified$$") == 0)
		throw EDriveSpecParse("Friendlyname string not specified.", filename, lua_tostring(L, -2));
	CDriveInfo driveinfo(drivetype, friendlyname, steprate, spinup, tracks, tpi, heads);

	return driveinfo;
}


bool CDriveScript::isDriveReady(const std::string drivetype, const unsigned long status)
{
	bool result;
	int err;

	// TODO: check drive type is in mDrives?

	lua_getfield(L, LUA_GLOBALSINDEX, "isDriveReady");
	lua_pushstring(L, drivetype.c_str());	// drive type string
	lua_pushnumber(L, status);				// status value from discferret_get_status()
	err = lua_pcall(L, 2, 1, 0);	// 2 parameters, 1 return value
	if (err) {
		// error -- throw an exception
		const char *errmsg = lua_tostring(L, -1);
		lua_pop(L, 1);	// pop error message off of stack
		throw ELuaError(errmsg);
	} else {
		// success -- return drive ready/not ready state
		result = lua_toboolean(L, -1);
		lua_pop(L, 1);	// pop result off of stack
		return result;
	}
}

int CDriveScript::getDriveOutputs(const std::string drivetype, const unsigned long track, const unsigned long head, const unsigned long sector)
{
	int result;
	int err;

	// TODO: check drive type is in mDrives?

	lua_getfield(L, LUA_GLOBALSINDEX, "getDriveOutputs");
	lua_pushstring(L, drivetype.c_str());	// drive type string
	lua_pushnumber(L, track);				// physical track
	lua_pushnumber(L, head);				// physical head
	lua_pushnumber(L, sector);				// physical sector
	err = lua_pcall(L, 4, 1, 0);			// 4 parameters, 1 return value
	if (err) {
		// error -- throw an exception
		const char *errmsg = lua_tostring(L, -1);
		lua_pop(L, 1);	// pop error message off of stack
		throw ELuaError(errmsg);
	} else {
		// success -- return drive ready/not ready state
		result = lua_tointeger(L, -1);
		lua_pop(L, 1);	// pop result off of stack
		return result;
	}
}

const std::vector<std::string> CDriveScript::getDrivetypes(void)
{
	return svDrivetypes;
}

