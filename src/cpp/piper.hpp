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

  struct PiperConfig
  {
  };

  enum PhonemeType
  {
    TextPhonemes
  };

  struct PhonemizeConfig
  {
    PhonemeType phonemeType = TextPhonemes;
    std::optional<std::map<Phoneme, std::vector<Phoneme>>> phonemeMap;
    std::map<Phoneme, std::vector<PhonemeId>> phonemeIdMap;

    PhonemeId idPad = 0; // padding (optionally interspersed)
    PhonemeId idBos = 1; // beginning of sentence
    PhonemeId idEos = 2; // end of sentence
    bool interspersePad = true;
  };

  struct SynthesisConfig
  {
    // VITS inference settings
    float noiseScale = 0.667f;
    float lengthScale = 1.0f;
    float noiseW = 0.8f;

    // Audio settings
    int sampleRate = 22050;
    int sampleWidth = 2; // 16-bit
    int channels = 1;    // mono

    // Speaker id from 0 to numSpeakers - 1
    std::optional<SpeakerId> speakerId;

    // Extra silence
    float sentenceSilenceSeconds = 0.2f;
    std::optional<std::map<piper::Phoneme, float>> phonemeSilenceSeconds;
  };

  struct ModelConfig
  {
    int numSpeakers;

    // speaker name -> id
    std::optional<std::map<std::string, SpeakerId>> speakerIdMap;
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
    ModelConfig modelConfig;
    ModelSession session;
  };

  // True if the string is a single UTF-8 codepoint
  bool isSingleCodepoint(std::string s);

  // Get version of Piper
  std::string getVersion();

  // Must be called before using textTo* functions
  void initialize(PiperConfig &config, std::string ipaPath);

  // Clean up
  void terminate(PiperConfig &config);

  // Load Onnx model and JSON config file
  void loadVoice(PiperConfig &config, std::string modelPath,
                 std::string modelConfigPath, Voice &voice,
                 std::optional<SpeakerId> &speakerId, bool useCuda);

  // Phonemize text and synthesize audio
  void textToAudio(PiperConfig &config, Voice &voice, std::string text,
                   std::vector<int16_t> &audioBuffer, SynthesisResult &result,
                   const std::function<void()> &audioCallback);

  // Phonemize text and synthesize audio to WAV file
  void textToWavFile(PiperConfig &config, Voice &voice, std::string text,
                     std::ostream &audioFile, SynthesisResult &result);

} // namespace piper

#endif // PIPER_H_
