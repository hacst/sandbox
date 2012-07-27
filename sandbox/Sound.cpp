#include "Sound.h"

#ifdef _WIN32
#include <windows.h>
#include <string>


bool doPlaySound(const std::string &soundfile)
{	
	return PlaySoundA(soundfile.c_str(), NULL, SND_FILENAME | SND_ASYNC) == TRUE ? true : false;
}

#else

bool doPlaySound(const std::string &soundfile) {
	// Not implemented
	return false;
}
#endif