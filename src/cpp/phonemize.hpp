#ifndef PHONEMIZE_H_
#define PHONEMIZE_H_

#include <filesystem>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include <espeak-ng/speak_lib.h>

#include "config.hpp"
#include "utf8.h"

using namespace std;

namespace larynx {

// Text to phonemes using eSpeak-ng
void phonemize(PhonemizeConfig &phonemizeConfig) {
  if (!phonemizeConfig.eSpeak) {
    throw runtime_error("Missing eSpeak config");
  }

  if (!phonemizeConfig.phonemes) {
    phonemizeConfig.phonemes.emplace();
  }

  auto voice = phonemizeConfig.eSpeak->voice;
  int result = espeak_SetVoiceByName(voice.c_str());
  if (result != 0) {
    throw runtime_error("Failed to set eSpeak-ng voice");
  }

  string text(phonemizeConfig.text);
  vector<char32_t> textClauseBreakers;

  utf8::iterator textIter(text.begin(), text.begin(), text.end());
  utf8::iterator textIterEnd(text.end(), text.begin(), text.end());

  while (textIter != textIterEnd) {
    auto codepoint = *textIter;
    if (phonemizeConfig.eSpeak->clauseBreakers.contains(codepoint)) {
      textClauseBreakers.push_back(codepoint);
    }

    textIter++;
  }

  const char *inputTextPointer = text.c_str();
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

    phonemizeConfig.phonemes->insert(phonemizeConfig.phonemes->end(),
                                     phonemeIter, phonemeEnd);
    if (clauseBreakerIndex < textClauseBreakers.size()) {
      phonemizeConfig.phonemes->push_back(
          textClauseBreakers[clauseBreakerIndex]);
      clauseBreakerIndex++;
    }
  }

} /* phonemize */

// Phonemes to ids using JSON map
void phonemes2ids(PhonemizeConfig &phonemizeConfig,
                  SynthesisConfig &synthesisConfig) {
  if (!phonemizeConfig.phonemes) {
    throw runtime_error("No phonemes present");
  }

  synthesisConfig.phonemeIds.push_back(phonemizeConfig.idBos);
  if (phonemizeConfig.interspersePad) {
    synthesisConfig.phonemeIds.push_back(phonemizeConfig.idPad);
  }

  for (auto phoneme = phonemizeConfig.phonemes->begin();
       phoneme != phonemizeConfig.phonemes->end(); phoneme++) {
    if (phonemizeConfig.phonemeIdMap.contains(*phoneme)) {
      for (auto id : phonemizeConfig.phonemeIdMap[*phoneme]) {
        synthesisConfig.phonemeIds.push_back(id);

        if (phonemizeConfig.interspersePad) {
          synthesisConfig.phonemeIds.push_back(phonemizeConfig.idPad);
        }
      }
    }
  }

  synthesisConfig.phonemeIds.push_back(phonemizeConfig.idEos);

} /* phonemes2ids */

} // namespace larynx

#endif // PHONEMIZE_H_
