#include <Cocoa/Cocoa.h> 
 
void removeDockIcon() 
{ 
	#ifdef __MAC_OS_X_VERSION_MIN_REQUIRED 
		#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070 
			// Don't display this application in the MAC OS X dock 
			// kProcessTransformToBackgroundApplication only works on Mac OS X 10.7 and later 
			ProcessSerialNumber psn; 
			if (GetCurrentProcess(&psn) == noErr) 
				TransformProcessType(&psn, kProcessTransformToBackgroundApplication); 
		#endif 
	#endif 
}
