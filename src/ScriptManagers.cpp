// TODO: eliminate cout / cerr and need for iostream
#include <iostream>
#include <dirent.h>

#include "ScriptManagers.hpp"

using namespace std;

void GenericScriptManager::scandir(const std::string path)
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
			// TODO: recursion limit
			cout << "traversing directory: " << filename <<endl;
			this->scandir(filename);
		} else if (dt->d_type == DT_REG) {
			// skip files which aren't lua scripts
			if (filename.substr(filename.length()-4, 4).compare(".lua") != 0) continue;

			// it's a regular file... load it as a script.
			this->scan(filename);
		}
	}
	closedir(dp);
}

CDriveScript CDriveScriptManager::load(const std::string drivetype)
{
	// Look up the drive type
	map<string,string>::const_iterator it = mDrivetypes.find(drivetype);
	if (it == mDrivetypes.end()) {
		cerr << "Drivetype not found" << endl;
		throw -1;
	}

	// Drive type is valid, and it->second contains the script file name
	return CDriveScript(it->second);
}

void CDriveScriptManager::scan(const std::string filename)
{
//	if (bVerbose) cout << "loading drivescript: " << filename << endl;
	CDriveScript script(filename);	// TODO: CAN_THROW --> catch exception

	// merge script's drivetype list with CDS's list
	vector<string> drivelist = script.getDrivetypes();
	for (vector<string>::const_iterator it = drivelist.begin(); it != drivelist.end(); it++) {
		// TODO: make sure this dt hasn't already been defined
		mDrivetypes[*it] = filename;
	}
}

