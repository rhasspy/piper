#ifndef API_H_
#define API_H_

#include <iostream>
#include <string>
#include <vector>

#include "json.hpp"
#include <espeak-ng/speak_lib.h>

#include "config.hpp"
#include "model.hpp"
#include "phonemize.hpp"
#include "synthesize.hpp"
#include "wavfile.hpp"

using json = nlohmann::json;

namespace larynx {

struct Voice {
  json configRoot;
  PhonemizeConfig phonemizeConfig;
  SynthesisConfig synthesisConfig;
  ModelSession session;
};

void initialize() {
  // Set up espeak-ng for calling espeak_TextToPhonemes
  int result = espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS,
                                 /*buflength*/ 0,
                                 /*path*/ NULL,
                                 /*options*/ 0);
  if (result < 0) {
    throw runtime_error("Failed to initialize eSpeak-ng");
  }
}

void terminate() {
  // Clean up espeak-ng
  espeak_Terminate();
}

// Load Onnx model and JSON config file
void loadVoice(string modelPath, string modelConfigPath, Voice &voice) {
  ifstream modelConfigFile(modelConfigPath.c_str());
  voice.configRoot = json::parse(modelConfigFile);

  parsePhonemizeConfig(voice.configRoot, voice.phonemizeConfig);
  parseSynthesisConfig(voice.configRoot, voice.synthesisConfig);

  loadModel(modelPath, voice.session);

} /* loadVoice */

// Phonemize text and synthesize audio
void textToAudio(Voice &voice, string text, vector<int16_t> &audioBuffer,
                 SynthesisResult &result) {
  voice.phonemizeConfig.text = text;
  voice.phonemizeConfig.phonemes.reset();
  phonemize(voice.phonemizeConfig);

  voice.synthesisConfig.phonemeIds.clear();
  phonemes2ids(voice.phonemizeConfig, voice.synthesisConfig);

  synthesize(voice.synthesisConfig, voice.session, audioBuffer, result);

} /* textToAudio */

// Phonemize text and synthesize audio to WAV file
void textToWavFile(Voice &voice, string text, ostream &audioFile,
                   SynthesisResult &result) {

  vector<int16_t> audioBuffer;
  textToAudio(voice, text, audioBuffer, result);

  // Write WAV
  auto synthesisConfig = voice.synthesisConfig;
  writeWavHeader(synthesisConfig.sampleRate, synthesisConfig.sampleWidth,
                 synthesisConfig.channels, (int32_t)audioBuffer.size(),
                 audioFile);

  audioFile.write((const char *)audioBuffer.data(),
                  sizeof(int16_t) * audioBuffer.size());

} /* textToAudio */

} // namespace larynx

#endif // API_H_
