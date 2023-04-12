#ifndef PHONEMIZE_H_
#define PHONEMIZE_H_

#include <filesystem>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include <espeak-ng/speak_lib.h>

#include "config.hpp"
#include "utf8.h"

using namespace std;

namespace piper {

// Text to phonemes using eSpeak-ng
void phonemize(string text, PhonemizeConfig &phonemizeConfig,
               vector<vector<Phoneme>> &phonemes) {
  if (!phonemizeConfig.eSpeak) {
    throw runtime_error("Missing eSpeak config");
  }

  auto voice = phonemizeConfig.eSpeak->voice;
  int result = espeak_SetVoiceByName(voice.c_str());
  if (result != 0) {
    throw runtime_error("Failed to set eSpeak-ng voice");
  }

  // Modified by eSpeak
  string textCopy(text);

  utf8::iterator textIter(textCopy.begin(), textCopy.begin(), textCopy.end());
  utf8::iterator textIterEnd(textCopy.end(), textCopy.begin(), textCopy.end());
  vector<char32_t> textClauseBreakers;

  // Identify clause breakers in the sentence, since eSpeak removes them during
  // phonemization.
  //
  // This will unfortunately do the wrong thing with abbreviations, etc.
  while (textIter != textIterEnd) {
    auto codepoint = *textIter;
    if (phonemizeConfig.eSpeak->clauseBreakers.contains(codepoint)) {
      textClauseBreakers.push_back(codepoint);
    }

    textIter++;
  }

  vector<Phoneme> *sentencePhonemes = nullptr;
  const char *inputTextPointer = textCopy.c_str();
  size_t clauseBreakerIndex = 0;

  while (inputTextPointer != NULL) {
    string clausePhonemes(
        espeak_TextToPhonemes((const void **)&inputTextPointer,
                              /*textmode*/ espeakCHARS_AUTO,
                              /*phonememode = IPA*/ 0x02));

    utf8::iterator phonemeIter(clausePhonemes.begin(), clausePhonemes.begin(),
                               clausePhonemes.end());
    utf8::iterator phonemeEnd(clausePhonemes.end(), clausePhonemes.begin(),
                              clausePhonemes.end());

    if (!sentencePhonemes) {
      // Start new sentence
      phonemes.emplace_back();
      sentencePhonemes = &phonemes[phonemes.size() - 1];
    }

    sentencePhonemes->insert(sentencePhonemes->end(), phonemeIter, phonemeEnd);
    if (clauseBreakerIndex < textClauseBreakers.size()) {
      auto clauseBreaker = textClauseBreakers[clauseBreakerIndex];
      sentencePhonemes->push_back(clauseBreaker);
      if (phonemizeConfig.eSpeak->sentenceBreakers.contains(clauseBreaker)) {
        // End of sentence
        sentencePhonemes = nullptr;
      }

      clauseBreakerIndex++;
    }
  }

} /* phonemize */

// Phonemes to ids using JSON map
void phonemes2ids(vector<Phoneme> &phonemes, PhonemizeConfig &phonemizeConfig,
                  vector<PhonemeId> &phonemeIds) {
  if (phonemes.empty()) {
    throw runtime_error("No phonemes");
  }

  phonemeIds.push_back(phonemizeConfig.idBos);
  if (phonemizeConfig.interspersePad) {
    phonemeIds.push_back(phonemizeConfig.idPad);
  }

  for (auto phoneme = phonemes.begin(); phoneme != phonemes.end(); phoneme++) {
    if (phonemizeConfig.phonemeIdMap.contains(*phoneme)) {
      for (auto id : phonemizeConfig.phonemeIdMap[*phoneme]) {
        phonemeIds.push_back(id);

        if (phonemizeConfig.interspersePad) {
          phonemeIds.push_back(phonemizeConfig.idPad);
        }
      }
    }
  }

  phonemeIds.push_back(phonemizeConfig.idEos);

} /* phonemes2ids */

} // namespace piper

#endif // PHONEMIZE_H_
