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
#include <sstream>
#include <algorithm>
#include <getopt.h>

// DiscFerret
#include <discferret/discferret.h>

// Local headers
#include "ScriptInterfaces.hpp"
#include "ScriptManagers.hpp"
#include "Exceptions.hpp"

using namespace std;

// Default script directories
#ifndef DRIVESCRIPTDIR
#define DRIVESCRIPTDIR "./scripts/drive"
#endif

/// Verbosity flag; true if verbose mode enabled.
int bVerbose = false;

/////////////////////////////////////////////////////////////////////////////

void usage(char *appname)
{
	cout
		<< "Usage:" << endl
		<< "   " << appname << " [--verbose] --drive drivetype --format formattype" <<endl
		<< "      [--serial serialnum]" << endl
		<< endl
		<< "Where:" << endl
		<< "   drivetype   Type of disc drive attached to the DiscFerret" << endl
		<< "   formattype  Type of the disc inserted in the drive" << endl
		<< "   serialnum   Serial number of the DiscFerret to connect to. If this is" << endl
		<< "               not specified, then the first DiscFerret will be used." << endl;
}

//////////////////////////////////////////////////////////////////////////////
// "I am main(), king of kings. Look upon my works, ye mighty, and despair!"
//////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
	string drivetype, formattype, serialnum;

	while (1) {
		// Getopt option table
		static const struct option opts_long[] = {
			// name			has_arg				flag			val
			{"help",		no_argument,		0,				'h'},
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

			case 'h':
				// show help and quit
				usage(argv[0]);
				return EXIT_SUCCESS;

			case 'd':
				// set drive type
				drivetype = optarg;
				break;

			case 'f':
				// set format type
				formattype = optarg;
				break;

			case 's':
				// discferret unit serial number
				serialnum = optarg;
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

	// Scan for drive scripts
	CDriveScriptManager dsmgr;
	dsmgr.scandir(DRIVESCRIPTDIR);

	// Make sure the user specified a valid drive type
	CDriveScript *drivescript;
	try {
		if (drivetype.length() != 0) {
			drivescript = new CDriveScript(dsmgr.load(drivetype));
		} else {
			cerr << "Error: drive type not specified." << endl;
			return EXIT_FAILURE;
		}
	} catch (...) {
		cerr << "Error: drive type '" << drivetype << "' was not defined by a drive script." << endl;
		return EXIT_FAILURE;
	}

	// TODO: implement format scripts to allow for weird stuff like Amiga mfmsync and MultiCycle Sampling

	int errcode = EXIT_SUCCESS;
	DISCFERRET_DEVICE_HANDLE *dh = NULL;
	try {
		// Try and initialise the DiscFerret API
		DISCFERRET_ERROR e;
		
		e = discferret_init();
		if (e != DISCFERRET_E_OK) {
			stringstream s("Error initialising libdiscferret. Error code: ");
			s << e;
			throw EApplicationError(s.str());
		}

		// Did the user spec a DiscFerret serial number to look for?
		if (serialnum.length() > 0) {
			// Yep -- open the specific DiscFerret requested
			e = discferret_open(serialnum.c_str(), &dh);
		} else {
			// No serial number specified, open the first DiscFerret
			e = discferret_open_first(&dh);
		}

		if (e != DISCFERRET_E_OK) {
			stringstream s("Error opening DiscFerret device. Is it connected and powered on? (error code ");
			s << e << ")";
			throw EApplicationError(s.str());
		}

		// Upload the DiscFerret microcode
		cout << "Loading microcode..." << endl;
		e = discferret_fpga_load_rbf(dh, discferret_microcode, discferret_microcode_length);
		if (e != DISCFERRET_E_OK) throw EApplicationError("Error loading DiscFerret microcode.");
		cout << "Microcode loaded successfully." << endl;

		// Show information about the DiscFerret in use
		DISCFERRET_DEVICE_INFO devinfo;
		e = discferret_get_info(dh, &devinfo);
		if (e != DISCFERRET_E_OK) throw ECommunicationError();
		cout << "Connected to DiscFerret with serial number " << devinfo.serialnumber << endl;
		cout << "Revision info: hardware " << devinfo.hardware_rev << ", firmware " << devinfo.firmware_ver << endl;
		cout << "Microcode type " << devinfo.microcode_type << ", revision " << devinfo.microcode_ver << endl;
		cout << endl;

		// Get some information about the disc type
		CDriveInfo driveinfo = drivescript->GetDriveInfo(drivetype);
		cout << "Drive type: '" << drivetype << "' (" << driveinfo.friendly_name() << ")" << endl;
		cout << "??? tpi, " << driveinfo.tracks() << " tracks, " << driveinfo.heads() << " heads." << endl;

		// Set up the step rate
		e = discferret_seek_set_rate(dh, driveinfo.steprate_us());
		if (e != DISCFERRET_E_OK) {
			if (e == DISCFERRET_E_BAD_PARAMETER) {
				throw EApplicationError("Seek rate out of range.");
			} else {
				throw EApplicationError("Error setting seek rate.");
			}
		}
	} catch (EApplicationError &e) {
		cerr << "Application error: " << e.what() << endl;
		errcode = EXIT_FAILURE;
	} catch (ECommunicationError &e) {
		cerr << e.what() << endl;
		errcode = EXIT_FAILURE;
	}

	// When it's all over, we still have to clean up...
	// Shut down libdiscferret
	discferret_close(dh);
	discferret_done();

	// Final cleanup
	delete drivescript;

	return errcode;
}
