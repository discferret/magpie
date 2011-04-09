#ifndef _hpp_Exceptions
#define _hpp_Exceptions

#include <exception>

/// An exception with a filename and a specname
#define XCPTFSN(name, errorstr)	\
	class name : public std::exception {																				\
		private:																										\
			std::string x, _spec, _filename;																			\
		public:																											\
			name(const std::string error, const std::string filename, const std::string specname = "") : exception() {	\
				x = error;																								\
				_filename = filename;																					\
				_spec = specname;																						\
			}																											\
			virtual ~name() throw() {};																					\
																														\
			virtual const char* what() const throw() {																	\
				if (_spec.length() > 0) {																				\
					return ("[" + _filename + ", drivespec '" + _spec + "']: " + errorstr + x).c_str();					\
				} else {																								\
					return ("[" + _filename + "]: " + errorstr + x).c_str();											\
				}																										\
			}																											\
			virtual const char* spec() const throw() { return _spec.c_str(); }											\
			virtual const char* filename() const throw() { return _filename.c_str(); }									\
			virtual const char* error() const throw() { return x.c_str(); }												\
	}

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

/// Exception class for DriveSpec parse errors
XCPTFSN(EDriveSpecParse, "DriveSpec script parse error: ");
/// Internal error in the scripting engine
XCPTFSN(EInternalScriptingError, "Internal script engine error: ");

/// Lua parse or runtime error
XCPTS(ELuaError, "Lua error: ");
/// Drivetype not known
XCPTS(EInvalidDrivetype, "Invalid drive type: ");

/// Application error
XCPT(EApplicationError, "Application error");
/// DiscFerret communications error
XCPT(ECommunicationError, "DiscFerret communication error");

#undef XCPTFSN
#undef XCPT
#undef XCPTS

#endif // _hpp_Exceptions

