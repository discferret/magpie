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

// Local headers
#include "ScriptInterfaces.hpp"

using namespace std;

// Default script directories
#ifndef DRIVESCRIPTDIR
#define DRIVESCRIPTDIR "./scripts/drive"
#endif

/// Verbosity flag; true if verbose mode enabled.
int bVerbose = false;

/////////////////////////////////////////////////////////////////////////////


/***
 *   CDriveScriptManager --> extend CGenericScriptManager
 *     --> implement Load() to create a CDriveScript and merge drivescript list from that with the root list
 *   CFormatScriptManager --> s.a.a. but create CFormatScript objects instead
 */

class GenericScriptManager {
	public:
		virtual void scan(const std::string filename) =0;

		virtual void scandir(const std::string path)
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
};

class CDriveScriptManager : public GenericScriptManager {
	private:
		/**
		 * Map between drive types and script filenames.
		 *
		 * This map is used to find out which script must be loaded to gain access
		 * to a given drive type.
		 */
		map<string, string> mDrivetypes;

	public:
		CDriveScript load(const std::string drivetype)
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

		void scan(const std::string filename)
		{
			if (bVerbose) cout << "loading drivescript: " << filename << endl;
			CDriveScript script(filename);	// TODO: CAN_THROW --> catch exception

			// merge script's drivetype list with CDS's list
			vector<string> drivelist = script.getDrivetypes();
			for (vector<string>::const_iterator it = drivelist.begin(); it != drivelist.end(); it++) {
				// TODO: make sure this dt hasn't already been defined
				mDrivetypes[*it] = filename;
			}
		}
};


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
//	TraverseScriptDir(DRIVESCRIPTDIR);
	CDriveScriptManager dsm;
	dsm.scandir(DRIVESCRIPTDIR);
	dsm.load("cdc_94205_51");
	dsm.load("cdc_94205_52");

	return 0;
}
