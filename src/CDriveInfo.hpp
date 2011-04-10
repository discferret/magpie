#ifndef _hpp_CDriveInfo
#define _hpp_CDriveInfo

#include <string>

/**
 * @brief	Drive information class
 *
 * Used to store information about a drive.
 */
class CDriveInfo {
	private:
		std::string		_drive_type;		///< Drive type string (immutable)
		std::string		_friendly_name;		///< Friendly name (displayed to user)
		unsigned long	_steprate_us;		///< Step rate in microseconds
		unsigned long	_spinup_ms;			///< Spin-up wait time in milliseconds
		unsigned long	_tracks;			///< Number of tracks
		float			_tpi;				///< Tracks per inch
		unsigned long	_heads;				///< Number of heads
	public:
		const std::string drive_type()			{ return _drive_type;		};
		void drive_type(const std::string x)	{ _drive_type = x;			};
		const std::string friendly_name()		{ return _friendly_name;	};
		void friendly_name(const std::string x)	{ _friendly_name = x;		};
		const unsigned long steprate_us()		{ return _steprate_us;		};
		void steprate_us(const unsigned long x)	{ _steprate_us = x;			};
		const unsigned long spinup_ms()			{ return _spinup_ms;		};
		void spinup_ms(const unsigned long x)	{ _spinup_ms = x;			};
		const unsigned long tracks()			{ return _tracks;			};
		void tracks(const unsigned long x)		{ _tracks = x;				};
		const float tpi()						{ return _tpi;				};
		void tpi(const float x)					{ _tpi = x;					};
		const unsigned long heads()				{ return _heads;			};
		void heads(const unsigned long x)		{ _heads = x;				};

		/// No-args ctor for CDriveInfo
		CDriveInfo()
		{
		}

		/**
		 * ctor for CDriveInfo.
		 *
		 * @param	drive_type		Drive type string.
		 * @param	friendly_name	Friendly name
		 * @param	step_rate_us	Step rate in microseconds
		 * @param	spinup_ms		Spin-up time in milliseconds
		 * @param	tracks			Number of tracks on drive
		 * @param	tpi				Number of tracks per inch
		 * @param	heads			Number of heads on drive
		 */
		CDriveInfo(
				std::string drive_type, std::string friendly_name,
				unsigned long step_rate_us,
				unsigned long spinup_ms,
				unsigned long tracks,
				float tpi,
				unsigned long heads)
		{
			_drive_type		= drive_type;
			_friendly_name	= friendly_name;
			_steprate_us	= step_rate_us;
			_spinup_ms		= spinup_ms;
			_tracks			= tracks;
			_tpi			= tpi;
			_heads			= heads;
		}
};

#endif // _hpp_CDriveInfo

