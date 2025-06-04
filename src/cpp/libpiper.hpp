#include <stdint.h>
#include <stdbool.h>
#include "piper.hpp"

using namespace piper;

#if defined(_WIN32) && !defined(__MINGW32__)
#    define PIPER_API __declspec(dllexport)
#else
#    define PIPER_API __attribute__ ((visibility ("default")))
#endif

extern "C" {
	typedef void (*AudioCallback)(int16_t* audioBuffer, int length);
	typedef void (*ProgressCallback)(uint16_t progress, size_t total);

#define LIBPIPER_LEVEL_TRACE 0
#define LIBPIPER_LEVEL_DEBUG 1
#define LIBPIPER_LEVEL_INFO 2
#define LIBPIPER_LEVEL_WARN 3
#define LIBPIPER_LEVEL_ERROR 4
#define LIBPIPER_LEVEL_CRITICAL 5
#define LIBPIPER_LEVEL_OFF 6

	PIPER_API void setLogLevel(int logLevel);

	PIPER_API void initializePiper(PiperConfig* config);
	PIPER_API void terminatePiper(PiperConfig* config);

	PIPER_API void loadVoice(PiperConfig* config, const char* modelPath, const char* modelConfigPath, Voice* voice, SpeakerId* speakerId);
	PIPER_API void textToAudio(PiperConfig* config, Voice* voice, const char* text, SynthesisResult* result, AudioCallback audioCallback, ProgressCallback progressCallback);
	PIPER_API void textToWavFile(PiperConfig* config, Voice* voice, const char* text, const char* audioFile, SynthesisResult* result, ProgressCallback progressCallback);
}
