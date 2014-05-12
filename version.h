#ifndef ins_VERSION_H
#define ins_VERSION_H

namespace ins_AutoVersion{
	
	//Date Version Types
	static const char ins_DATE[] = "12";
	static const char ins_MONTH[] = "05";
	static const char ins_YEAR[] = "2014";
	static const char ins_UBUNTU_VERSION_STYLE[] =  "14.05";
	
	//Software Status
	static const char ins_STATUS[] =  "Alpha";
	static const char ins_STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long ins_MAJOR  = 1;
	static const long ins_MINOR  = 5;
	static const long ins_BUILD  = 55;
	static const long ins_REVISION  = 79;
	
	//Miscellaneous Version Types
	static const long ins_BUILDS_COUNT  = 105;
	#define ins_RC_FILEVERSION 1,5,55,79
	#define ins_RC_FILEVERSION_STRING "1, 5, 55, 79\0"
	static const char ins_FULLVERSION_STRING [] = "1.5.55.79";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long ins_BUILD_HISTORY  = 7;
	

}
#endif //ins_VERSION_H
