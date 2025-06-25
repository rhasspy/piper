#include "piper.hpp"
#include "piperlib.hpp"
#include <cstring>

extern "C"
{
    eSpeakConfig* create_eSpeakConfig() {
        return new eSpeakConfig();
    }

    void destroy_eSpeakConfig(eSpeakConfig* config) {
        delete config;
    }

    PiperConfig* create_PiperConfig(char* eSpeakDataPath) {
        auto config = new PiperConfig();
        config->eSpeakDataPath = eSpeakDataPath;
        return config;
    }

    void destroy_PiperConfig(PiperConfig* config) {
        delete config;
    }

    PhonemizeConfig* create_PhonemizeConfig() {
        return new PhonemizeConfig();
    }

    void destroy_PhonemizeConfig(PhonemizeConfig* config) {
        delete config;
    }

    SynthesisConfig* create_SynthesisConfig() {
        return new SynthesisConfig();
    }

    void destroy_SynthesisConfig(SynthesisConfig* config) {
        delete config;
    }

    ModelConfig* create_ModelConfig() {
        return new ModelConfig();
    }

    void destroy_ModelConfig(ModelConfig* config) {
        delete config;
    }

    ModelSession* create_ModelSession() {
        return new ModelSession();
    }

    void destroy_ModelSession(ModelSession* config) {
        delete config;
    }

    SynthesisResult* create_SynthesisResult() {
        return new SynthesisResult();
    }

    void destroy_SynthesisResult(SynthesisResult* config) {
        delete config;
    }

    Voice* create_Voice() {
        return new Voice();
    }

    void destroy_Voice(Voice* voice) {
        delete voice;
    }

    bool isSingleCodepoint(const char* s) {
        std::string str(s);
        return piper::isSingleCodepoint(str);
    }

    char32_t getCodepoint(const char* s) {
        return piper::getCodepoint(s);
    }

    char* getVersion() {
        auto version = piper::getVersion();
        char* cstr = new char[version.size() + 1];
        std::strcpy(cstr, version.c_str());
        return cstr;
    }

    void initializePiper(PiperConfig* config) {
        piper::initialize(*config);
    }

    void terminatePiper(PiperConfig* config) {
        piper::terminate(*config);
    }

    SynthesisConfig getSynthesisConfig(Voice* voice) {
        return voice->synthesisConfig;
    }

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
