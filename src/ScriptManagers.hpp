#ifndef _hpp_ScriptManagers
#define _hpp_ScriptManagers

#include <string>
#include <map>

#include "ScriptInterfaces.hpp"

class GenericScriptManager {
	public:
		virtual void scan(const std::string filename) =0;
		virtual void scandir(const std::string path);
};

class CDriveScriptManager : public GenericScriptManager {
	private:
		/**
		 * Map between drive types and script filenames.
		 *
		 * This map is used to find out which script must be loaded to gain access
		 * to a given drive type.
		 */
		std::map<std::string, std::string> mDrivetypes;

	public:
		CDriveScript load(const std::string drivetype);
		void scan(const std::string filename);
};

#endif // _hpp_ScriptManagers

