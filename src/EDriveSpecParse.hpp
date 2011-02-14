#ifndef _hpp_EDriveSpecParse
#define _hpp_EDriveSpecParse

#include <exception>

/**
 * Exception class for DriveSpec parse errors
 */
class EDriveSpecParse : public std::exception {
	private:
		std::string x;
		unsigned long _spec;
	public:
		EDriveSpecParse(const std::string error, const unsigned long specnum) : exception(), x(error), _spec(specnum) { };
		virtual ~EDriveSpecParse() throw() {};

		virtual const char* what() throw() { return x.c_str(); }
		virtual const unsigned long spec() { return _spec; }
};

#endif // _hpp_EDriveSpecParse

