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

// Local headers
#include "ScriptInterfaces.hpp"
#include "ScriptManagers.hpp"

using namespace std;

// Default script directories
#ifndef DRIVESCRIPTDIR
#define DRIVESCRIPTDIR "./scripts/drive"
#endif

/// Verbosity flag; true if verbose mode enabled.
int bVerbose = false;

/////////////////////////////////////////////////////////////////////////////



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
