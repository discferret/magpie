// C++ STL headers
#include <string>
#include <vector>
#include <map>

// Lua 5.1 (PUC-Rio)
#include "lua.hpp"

// Local headers
#include "CDriveInfo.hpp"

/**
 * Interface and common code for script loading.
 */
class CScriptInterface {
	protected:
		lua_State *L;

	public:
		CScriptInterface(const std::string filename);
		~CScriptInterface();
};

class CDriveScript : public CScriptInterface {
	private:
		std::vector<std::string> svDrivetypes;

	public:
		CDriveScript(const std::string filename);

		CDriveInfo GetDriveInfo(const std::string drivetype);

		/**
		 * @brief	Lua wrapper function for IsDriveReady() DriveSpec function
		 */
		bool isDriveReady(const std::string drivetype, const unsigned long status)
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
//				cerr << "Error calling isDriveReady: " << lua_tostring(L, -1) << endl;
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
		bool getDriveOutputs(const std::string drivetype, const unsigned long track, const unsigned long head, const unsigned long sector)
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
//				cerr << "Error calling getDriveOutputs: " << lua_tostring(L, -1) << endl;
				lua_pop(L, 1);	// pop error message off of stack
				return false;
			} else {
				result = lua_toboolean(L, -1);
				lua_pop(L, 1);	// pop result off of stack
				return result;
			}
		}

		const std::vector<std::string> getDrivetypes(void) {
			return svDrivetypes;
		}
};

