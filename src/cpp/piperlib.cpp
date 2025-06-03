#include "piper.hpp"
#include "piperlib.hpp"
#include <cstring>

extern "C"
{
    void loadVoice(PiperConfig* config, const char* modelPath, const char* modelConfigPath, Voice* voice, SpeakerId* speakerId) {
        std::optional<piper::SpeakerId> optSpeakerId;
        if (speakerId) {
            optSpeakerId = *speakerId;
        }
        piper::loadVoice(*config, modelPath, modelConfigPath, *voice, optSpeakerId);
    }

    void textToAudio(PiperConfig* config, Voice* voice, const char* text, SynthesisResult* result, AudioCallback audioCallback, ProgressCallback progressCallback) {
        std::vector<int16_t> audioBuf;
        piper::textToAudio(*config, *voice, text, audioBuf, *result, [&audioBuf, audioCallback] { audioCallback(audioBuf.data(), audioBuf.size()); }, progressCallback);
    }

    void textToWavFile(PiperConfig* config, Voice* voice, const char* text, const char* audioFile, SynthesisResult* result, ProgressCallback progressCallback) {
        std::string audioFilePath = audioFile;
        std::ofstream audioFileStream(audioFilePath, std::ios::binary);
        piper::textToWavFile(*config, *voice, text, audioFileStream, *result, progressCallback);
        audioFileStream.flush();
        audioFileStream.close();
    }
}
