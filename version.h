#ifndef ins_VERSION_H
#define ins_VERSION_H

namespace ins_AutoVersion{
	
	//Date Version Types
	static const char ins_DATE[] = "09";
	static const char ins_MONTH[] = "03";
	static const char ins_YEAR[] = "2015";
	static const char ins_UBUNTU_VERSION_STYLE[] =  "15.03";
	
	//Software Status
	static const char ins_STATUS[] =  "Alpha";
	static const char ins_STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long ins_MAJOR  = 1;
	static const long ins_MINOR  = 7;
	static const long ins_BUILD  = 335;
	static const long ins_REVISION  = 1576;
	
	//Miscellaneous Version Types
	static const long ins_BUILDS_COUNT  = 427;
	#define ins_RC_FILEVERSION 1,7,335,1576
	#define ins_RC_FILEVERSION_STRING "1, 7, 335, 1576\0"
	static const char ins_FULLVERSION_STRING [] = "1.7.335.1576";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long ins_BUILD_HISTORY  = 87;
	

}
#endif //ins_VERSION_H
