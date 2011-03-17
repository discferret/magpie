#ifndef _hpp_EDriveSpecParse
#define _hpp_EDriveSpecParse

#include <exception>

/**
 * Exception class for DriveSpec parse errors
 */
class EDriveSpecParse : public std::exception {
	private:
		std::string x, _spec, _filename;
	public:
		EDriveSpecParse(const std::string error, const std::string filename, const std::string specname) : exception() {
			x = error;
			_filename = filename;
			_spec = specname;
		}
		EDriveSpecParse(const std::string error, const std::string filename) : exception() {
			x = error;
			_filename = filename;
			_spec = "";
		}
		virtual ~EDriveSpecParse() throw() {};

		virtual const char* what() const throw() {
			if (_spec.length() > 0) {
				return ("[" + _filename + ", drivespec '" + _spec + "']: " + x).c_str();
			} else {
				return ("[" + _filename + "]: " + x).c_str();
			}
		}
		virtual const char* spec() const throw() { return _spec.c_str(); }
		virtual const char* filename() const throw() { return _filename.c_str(); }
		virtual const char* error() const throw() { return x.c_str(); }
};

/// A generic exception
#define XCPT(name, errorstr) \
	class name : public std::exception {								\
		public:															\
			virtual const char* what() throw() { return errorstr; }		\
	}

/// An exception which accepts a string parameter on throwing
#define XCPTS(name, errorstr) \
	class name : public std::exception {											\
		private:																	\
			std::string x;															\
		public:																		\
			name(const std::string error) : exception(), x(errorstr + error) {};	\
			virtual ~name() throw() {};												\
			virtual const char* what() const throw() { return x.c_str(); }			\
	}


/// Lua parse or runtime error
XCPTS(ELuaError, "Lua error: ");
/// Internal error in the scripting engine
XCPTS(EInternalScriptingError, "Internal script engine error: ");
/// Drivetype not known
XCPTS(EInvalidDrivetype, "Invalid drive type: ");


#undef XCPT
#undef XCPTS

#endif // _hpp_EDriveSpecParse

