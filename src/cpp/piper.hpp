#ifndef PIPER_H_
#define PIPER_H_

#include <filesystem>
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

namespace piper {

struct Voice {
  json configRoot;
  PhonemizeConfig phonemizeConfig;
  SynthesisConfig synthesisConfig;
  ModelConfig modelConfig;
  ModelSession session;
};

void initialize(std::filesystem::path cwd) {
  const char *dataPath = NULL;

  auto cwdDataPath = cwd.append("espeak-ng-data");
  if (std::filesystem::is_directory(cwdDataPath)) {
    dataPath = cwdDataPath.c_str();
  }

  // Set up espeak-ng for calling espeak_TextToPhonemes
  int result = espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS,
                                 /*buflength*/ 0,
                                 /*path*/ dataPath,
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
void loadVoice(string modelPath, string modelConfigPath, Voice &voice,
               optional<SpeakerId> &speakerId) {
  ifstream modelConfigFile(modelConfigPath.c_str());
  voice.configRoot = json::parse(modelConfigFile);

  parsePhonemizeConfig(voice.configRoot, voice.phonemizeConfig);
  parseSynthesisConfig(voice.configRoot, voice.synthesisConfig);
  parseModelConfig(voice.configRoot, voice.modelConfig);

  if (voice.modelConfig.numSpeakers > 1) {
    // Multispeaker model
    if (speakerId) {
      voice.synthesisConfig.speakerId = speakerId;
    } else {
      // Default speaker
      voice.synthesisConfig.speakerId = 0;
    }
  }

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

} /* textToWavFile */

} // namespace piper

#endif // PIPER_H_
