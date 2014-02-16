// TODO: eliminate cout / cerr and need for iostream
#include <iostream>
#include <dirent.h>
#include <sys/stat.h>

#include "Exceptions.hpp"
#include "ScriptManagers.hpp"

using namespace std;

void GenericScriptManager::scandir(const std::string path, int recursionLimit)
{
	DIR *dp;
	struct dirent *dt;
	struct stat s;

	// Prevent infinite recursion
	if (recursionLimit == 0) return;

	dp = opendir(path.c_str());
	if (dp == NULL) return;
	while ((dt = readdir(dp)) != NULL) {
		string filename = path + "/";
		// skip hidden files
		if (dt->d_name[0] == '.') continue;
		// add filename to get fully qualified path
		filename += dt->d_name;

		// the following is copied right from an example to fix the lack of DT_DIR/DT_REG on mingw
		stat(filename.c_str(), &s);
		// What is this directory entry?
		if (s.st_mode & S_IFDIR) {
			// it's a subdirectory... scan it too.
			cout << "traversing directory: " << filename <<endl;
			this->scandir(filename, recursionLimit - 1);
		} else if (s.st_mode & S_IFREG) {
			// skip files which aren't lua scripts
			if (filename.substr(filename.length()-4, 4).compare(".lua") != 0) continue;
			// didn't skip, so it must be a lua script
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
		throw EInvalidDrivetype(drivetype);
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

