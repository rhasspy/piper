#include "piper.hpp"
#include "piperlib.hpp"
#include <cstring>

extern "C"
{
    void loadVoice(PiperConfig* config, const char* modelPath, const char* modelConfigPath, Voice* voice, int64_t* speakerId) {
        std::optional<piper::SpeakerId> optSpeakerId;
        if (speakerId) {
            optSpeakerId = *speakerId;
        }
        piper::loadVoice(*config, modelPath, modelConfigPath, *voice, optSpeakerId);
    }

    void textToAudio(PiperConfig* config, Voice* voice, const char* text, SynthesisResult* result, AudioCallback audioCallback) {
        std::vector<int16_t> audioBuf;
        piper::textToAudio(*config, *voice, text, audioBuf, *result, [&audioBuf, audioCallback] { audioCallback(audioBuf.data(), audioBuf.size()); });
    }

    void textToWavFile(PiperConfig* config, Voice* voice, const char* text, const char* audioFile, SynthesisResult* result) {
        std::string audioFilePath = audioFile;
        std::ofstream audioFileStream(audioFilePath, std::ios::binary);
        piper::textToWavFile(*config, *voice, text, audioFileStream, *result);
        audioFileStream << "opened";
        audioFileStream.flush();
        audioFileStream.close();
    }
}
