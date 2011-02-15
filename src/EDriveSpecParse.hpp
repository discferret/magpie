#ifndef _hpp_EDriveSpecParse
#define _hpp_EDriveSpecParse

#include <exception>

/**
 * Exception class for DriveSpec parse errors
 */
class EDriveSpecParse : public std::exception {
	private:
		std::string x;
		long _spec;
	public:
		EDriveSpecParse(const std::string error, const long specnum) : exception(), x(error), _spec(specnum) { };
		EDriveSpecParse(const std::string error) : exception(), x(error), _spec(-1) { };
		virtual ~EDriveSpecParse() throw() {};

		virtual const char* what() throw() { return x.c_str(); }
		virtual const long spec() { return _spec; }
};

#endif // _hpp_EDriveSpecParse

