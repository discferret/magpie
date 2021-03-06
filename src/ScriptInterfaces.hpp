#ifndef _hpp_ScriptInterfaces
#define _hpp_ScriptInterfaces

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
		std::string filename;

	public:
		CScriptInterface(const std::string _filename);
		~CScriptInterface();
};

class CDriveScript : public CScriptInterface {
	private:
		std::vector<std::string> svDrivetypes;

	public:
		CDriveScript(const std::string _filename);

		CDriveInfo GetDriveInfo(const std::string drivetype);

		/**
		 * @brief	Lua wrapper function for IsDriveReady() DriveSpec function
		 */
		bool isDriveReady(const std::string drivetype, const unsigned long status);

		/**
		 * @brief	Lua wrapper function for GetDriveOutputs() DriveSpec function
		 */
		int getDriveOutputs(const std::string drivetype, const unsigned long track, const unsigned long head, const unsigned long sector);

		const std::vector<std::string> getDrivetypes(void);
};

#endif // _hpp_ScriptInterfaces

