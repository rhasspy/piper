#include <stdint.h>
#include <stdbool.h>
#include "piper.hpp"

using namespace piper;

#if defined(_WIN32) && !defined(__MINGW32__)
#    define PIPER_API __declspec(dllimport)
#else
#    define PIPER_API __attribute__ ((visibility ("default")))
#endif

extern "C" {
	typedef void (*AudioCallback)(int16_t* audioBuffer, int length);

	PIPER_API eSpeakConfig* create_eSpeakConfig();
	PIPER_API void destroy_eSpeakConfig(eSpeakConfig* config);
	PIPER_API PiperConfig* create_PiperConfig(char* eSpeakDataPath);
	PIPER_API void destroy_PiperConfig(PiperConfig* config);
	PIPER_API PhonemizeConfig* create_PhonemizeConfig();
	PIPER_API void destroy_PhonemizeConfig(PhonemizeConfig* config);
	PIPER_API SynthesisConfig* create_SynthesisConfig();
	PIPER_API void destroy_SynthesisConfig(SynthesisConfig* config);
	PIPER_API ModelConfig* create_ModelConfig();
	PIPER_API void destroy_ModelConfig(ModelConfig* config);
	PIPER_API ModelSession* create_ModelSession();
	PIPER_API void destroy_ModelSession(ModelSession* config);
	PIPER_API SynthesisResult* create_SynthesisResult();
	PIPER_API void destroy_SynthesisResult(SynthesisResult* config);
	PIPER_API Voice* create_Voice();
	PIPER_API void destroy_Voice(Voice* voice);

	PIPER_API bool isSingleCodepoint(const char* s);
	PIPER_API char32_t getCodepoint(const char* s);
	PIPER_API char* getVersion();
	PIPER_API void initializePiper(PiperConfig* config);
	PIPER_API void terminatePiper(PiperConfig* config);
	PIPER_API SynthesisConfig getSynthesisConfig(Voice* voice);
	PIPER_API void loadVoice(PiperConfig* config, const char* modelPath, const char* modelConfigPath, Voice* voice, SpeakerId* speakerId);
	PIPER_API void textToAudio(PiperConfig* config, Voice* voice, const char* text, SynthesisResult* result, AudioCallback audioCallback);
	PIPER_API void textToWavFile(PiperConfig* config, Voice* voice, const char* text, const char* audioFile, SynthesisResult* result);

}
