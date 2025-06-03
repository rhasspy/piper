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

	PIPER_API void initializePiper(PiperConfig* config);
	PIPER_API void terminatePiper(PiperConfig* config);

	PIPER_API void loadVoice(PiperConfig* config, const char* modelPath, const char* modelConfigPath, Voice* voice, SpeakerId* speakerId);
	PIPER_API void textToAudio(PiperConfig* config, Voice* voice, const char* text, SynthesisResult* result, AudioCallback audioCallback, ProgressCallback progressCallback);
	PIPER_API void textToWavFile(PiperConfig* config, Voice* voice, const char* text, const char* audioFile, SynthesisResult* result, ProgressCallback progressCallback);
}
