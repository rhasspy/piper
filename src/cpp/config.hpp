#ifndef CONFIG_H_
#define CONFIG_H_

#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "json.hpp"
#include "utf8.h"

using namespace std;
using json = nlohmann::json;

namespace piper {

typedef char32_t Phoneme;
typedef int64_t PhonemeId;
typedef int64_t SpeakerId;

const string DefaultVoice = "en-us";

enum eSpeakMode { Text, TextWithPhonemes, SSML };

struct eSpeakConfig {
  string voice = DefaultVoice;
  eSpeakMode mode = Text;

  // Characters that eSpeak uses to break apart paragraphs/sentences
  set<Phoneme> clauseBreakers{U'.', U'?', U'!', U',', U';', U':'};

  Phoneme fullStop = U'.';
  Phoneme comma = U',';
  Phoneme question = U'?';
  Phoneme exclamation = U'!';
};

struct PhonemizeConfig {
  optional<map<Phoneme, vector<Phoneme>>> phonemeMap;
  map<Phoneme, vector<PhonemeId>> phonemeIdMap;

  PhonemeId idPad = 0; // padding (optionally interspersed)
  PhonemeId idBos = 1; // beginning of sentence
  PhonemeId idEos = 2; // end of sentence
  bool interspersePad = true;

  optional<eSpeakConfig> eSpeak;
};

struct SynthesisConfig {
  float noiseScale = 0.667f;
  float lengthScale = 1.0f;
  float noiseW = 0.8f;
  int sampleRate = 22050;
  int sampleWidth = 2; // 16-bit
  int channels = 1;    // mono
  optional<SpeakerId> speakerId;
  float sentenceSilenceSeconds = 0.2f;
};

struct ModelConfig {
  int numSpeakers;
};

bool isSingleCodepoint(string s) {
  return utf8::distance(s.begin(), s.end()) == 1;
}

Phoneme getCodepoint(string s) {
  utf8::iterator character_iter(s.begin(), s.begin(), s.end());
  return *character_iter;
}

void parsePhonemizeConfig(json &configRoot, PhonemizeConfig &phonemizeConfig) {

  if (configRoot.contains("espeak")) {
    if (!phonemizeConfig.eSpeak) {
      phonemizeConfig.eSpeak.emplace();
    }

    auto espeakValue = configRoot["espeak"];
    if (espeakValue.contains("voice")) {
      phonemizeConfig.eSpeak->voice = espeakValue["voice"].get<string>();
    }
  }

  // phoneme to [phoneme] map
  if (configRoot.contains("phoneme_map")) {
    if (!phonemizeConfig.phonemeMap) {
      phonemizeConfig.phonemeMap.emplace();
    }

    auto phonemeMapValue = configRoot["phoneme_map"];
    for (auto &fromPhonemeItem : phonemeMapValue.items()) {
      string fromPhoneme = fromPhonemeItem.key();
      if (!isSingleCodepoint(fromPhoneme)) {
        throw runtime_error("Phonemes must be one codepoint (phoneme map)");
      }

      auto fromCodepoint = getCodepoint(fromPhoneme);
      for (auto &toPhonemeValue : fromPhonemeItem.value()) {
        string toPhoneme = toPhonemeValue.get<string>();
        if (!isSingleCodepoint(toPhoneme)) {
          throw runtime_error("Phonemes must be one codepoint (phoneme map)");
        }

        auto toCodepoint = getCodepoint(toPhoneme);
        (*phonemizeConfig.phonemeMap)[fromCodepoint].push_back(toCodepoint);
      }
    }
  }

  // phoneme to [id] map
  if (configRoot.contains("phoneme_id_map")) {
    auto phonemeIdMapValue = configRoot["phoneme_id_map"];
    for (auto &fromPhonemeItem : phonemeIdMapValue.items()) {
      string fromPhoneme = fromPhonemeItem.key();
      if (!isSingleCodepoint(fromPhoneme)) {
        throw runtime_error("Phonemes must be one codepoint (phoneme id map)");
      }

      auto fromCodepoint = getCodepoint(fromPhoneme);
      for (auto &toIdValue : fromPhonemeItem.value()) {
        PhonemeId toId = toIdValue.get<PhonemeId>();
        phonemizeConfig.phonemeIdMap[fromCodepoint].push_back(toId);
      }
    }
  }

} /* parsePhonemizeConfig */

void parseSynthesisConfig(json &configRoot, SynthesisConfig &synthesisConfig) {

  if (configRoot.contains("audio")) {
    auto audioValue = configRoot["audio"];
    if (audioValue.contains("sample_rate")) {
      // Default sample rate is 22050 Hz
      synthesisConfig.sampleRate = audioValue.value("sample_rate", 22050);
    }
  }

} /* parseSynthesisConfig */

void parseModelConfig(json &configRoot, ModelConfig &modelConfig) {

  modelConfig.numSpeakers = configRoot["num_speakers"].get<SpeakerId>();

} /* parseModelConfig */

} // namespace piper

#endif // CONFIG_H_
