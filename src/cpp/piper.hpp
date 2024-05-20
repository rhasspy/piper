#ifndef PIPER_H_
#define PIPER_H_

#include <fstream>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include <onnxruntime_cxx_api.h>

#include "json.hpp"

using json = nlohmann::json;

namespace piper
{
  typedef char32_t Phoneme;

  typedef int64_t PhonemeId;

  typedef int64_t SpeakerId;

  struct PhonemizeConfig
  {
    std::optional<std::map<Phoneme, std::vector<Phoneme>>> phonemeMap;
    std::map<Phoneme, std::vector<PhonemeId>> phonemeIdMap;

    PhonemeId idPad = 0; // padding (optionally interspersed)
    PhonemeId idBos = 1; // beginning of sentence
    PhonemeId idEos = 2; // end of sentence
    bool interspersePad = true;
  };

  struct SynthesisConfig
  {
    std::string modelPath = "";

    // VITS inference settings
    float noiseScale = 0.667f;
    float lengthScale = 1.0f;
    float noiseW = 0.8f;

    std::string outputPath;
    bool writeFile = false;
    bool useCuda = false;

    // Audio settings
    int sampleRate = 22050;
    int sampleWidth = 2; // 16-bit
    int channels = 1;    // mono

    // Speaker id from 0 to numSpeakers - 1
    SpeakerId speakerId = 0;

    // Extra silence
    float sentenceSilenceSeconds = 0.2f;
    std::map<piper::Phoneme, float> phonemeSilenceSeconds;
  };

  struct ModelSession
  {
    Ort::Session onnx;
    Ort::AllocatorWithDefaultOptions allocator;
    Ort::SessionOptions options;
    Ort::Env env;

    ModelSession() : onnx(nullptr){};
  };

  struct SynthesisResult
  {
    double inferSeconds;
    double audioSeconds;
    double realTimeFactor;
  };

  struct Voice
  {
    json configRoot;
    SynthesisConfig synthesisConfig;
    ModelSession session;
  };

  // Get version of Piper
  std::string getVersion();

  // Must be called before using textTo* functions
  void LoadIPAData(std::string ipaPath);

  SynthesisConfig *LoadSynthesisConfig(const char *configPath);

  // Load Onnx model and JSON config file
  Voice *LoadVoice(SynthesisConfig &synthConfig);

  // Phonemize text and synthesize audio
  void textToAudio(Voice &voice, std::string text,
                   std::vector<int16_t> &audioBuffer, SynthesisResult &result,
                   const std::function<void()> &audioCallback);

  // Phonemize text and synthesize audio to WAV file
  char *textToVoice(Voice &voice, std::string text, uint32_t &dataSize);

} // namespace piper

#endif // PIPER_H_
