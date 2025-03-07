#ifndef PIPER_H_
#define PIPER_H_

#include <fstream>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include <onnxruntime_cxx_api.h>
#include <piper-phonemize/phonemize.hpp>
#include <piper-phonemize/tashkeel.hpp>

#include "json.hpp"
#include "phonemes.hpp"

using json = nlohmann::json;

namespace piper {

  typedef int64_t SpeakerId;

  struct eSpeakConfig {
    std::string voice = "en-us";
  };

  struct PiperConfig {
    std::string eSpeakDataPath;
    bool useESpeak = true;

    bool useTashkeel = false;
    bool phonemeInput = false;
    std::optional<std::string> tashkeelModelPath;
    std::unique_ptr<tashkeel::State> tashkeelState;
  };

  enum PhonemeType { eSpeakPhonemes, TextPhonemes, RawPhonemes };

  struct PhonemizeConfig {
    PhonemeType phonemeType = eSpeakPhonemes;
    std::optional<std::map<Phoneme, std::vector<Phoneme>>> phonemeMap;
    std::map<Phoneme, std::vector<PhonemeId>> phonemeIdMap;

    PhonemeId idPad = 0; // padding (optionally interspersed)
    PhonemeId idBos = 1; // beginning of sentence
    PhonemeId idEos = 2; // end of sentence
    bool interspersePad = true;

    eSpeakConfig eSpeak;
  };

  struct SynthesisConfig {
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

  struct ModelConfig {
    int numSpeakers;

    // speaker name -> id
    std::optional<std::map<std::string, SpeakerId>> speakerIdMap;
  };

  struct ModelSession {
    Ort::Session onnx;
    Ort::AllocatorWithDefaultOptions allocator;
    Ort::SessionOptions options;
    Ort::Env env;

    ModelSession() : onnx(nullptr) {};
  };

  struct SynthesisResult {
    double inferSeconds;
    double audioSeconds;
    double realTimeFactor;
  };

  struct Voice {
    json configRoot;
    PhonemizeConfig phonemizeConfig;
    SynthesisConfig synthesisConfig;
    ModelConfig modelConfig;
    ModelSession session;
  };

  // Get the first UTF-8 codepoint of a string or 1-3 ASCII chars 
  Phoneme getCodepoint(std::string s);

  // Get version of Piper
  std::string getVersion();

  // Must be called before using textTo* functions
  void initialize(PiperConfig& config);

  // Clean up
  void terminate(PiperConfig& config);

  // Load Onnx model and JSON config file
  void loadVoice(PiperConfig& config, std::string modelPath,
    std::string modelConfigPath, Voice& voice,
    std::optional<SpeakerId>& speakerId, bool useCuda);

  // Phonemize text and synthesize audio
  void textToAudio(PiperConfig& config, Voice& voice, std::string text,
    std::vector<int16_t>& audioBuffer, SynthesisResult& result,
    const std::function<void()>& audioCallback);

  // Phonemize text and synthesize audio to WAV file
  void textToWavFile(PiperConfig& config, Voice& voice, std::string text,
    std::ostream& audioFile, SynthesisResult& result);

  // Parses phonemes from text into phonemes vector
  void parse_phonemes(std::string text, std::vector<std::vector<Phoneme>>& phonemes);

  // Converts phoneme vector to phoneme ids
  void phonemes_to_ids(const std::vector<Phoneme>& phonemes, PhonemeIdConfig& config,
    std::vector<PhonemeId>& phonemeIds,
    std::map<Phoneme, std::size_t>& missingPhonemes);

} // namespace piper

#endif // PIPER_H_
