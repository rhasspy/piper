#include <array>
#include <chrono>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>

#include <espeak-ng/speak_lib.h>
#include <onnxruntime_cxx_api.h>
#include <spdlog/spdlog.h>

#include "json.hpp"
#include "piper.hpp"
#include "utf8.h"
#include "wavfile.hpp"

namespace piper {

#ifdef _PIPER_VERSION
// https://stackoverflow.com/questions/47346133/how-to-use-a-define-inside-a-format-string
#define _STR(x) #x
#define STR(x) _STR(x)
const std::string VERSION = STR(_PIPER_VERSION);
#else
const std::string VERSION = "";
#endif

// Maximum value for 16-bit signed WAV sample
const float MAX_WAV_VALUE = 32767.0f;

const std::string instanceName{"piper"};

std::string getVersion() {
  return VERSION;
}

// True if the string is a single UTF-8 codepoint
bool isSingleCodepoint(std::string s) {
  return utf8::distance(s.begin(), s.end()) == 1;
}

// Get the first UTF-8 codepoint of a string
Phoneme getCodepoint(std::string s) {
  utf8::iterator character_iter(s.begin(), s.begin(), s.end());
  return *character_iter;
}

// Load JSON config information for phonemization
void parsePhonemizeConfig(json &configRoot, PhonemizeConfig &phonemizeConfig) {
  // {
  //     "espeak": {
  //         "voice": "<language code>"
  //     },
  //     "phoneme_type": "<espeak or text>",
  //     "phoneme_map": {
  //         "<from phoneme>": ["<to phoneme 1>", "<to phoneme 2>", ...]
  //     },
  //     "phoneme_id_map": {
  //         "<phoneme>": [<id1>, <id2>, ...]
  //     }
  // }

  if (configRoot.contains("espeak")) {
    auto espeakValue = configRoot["espeak"];
    if (espeakValue.contains("voice")) {
      phonemizeConfig.eSpeak.voice = espeakValue["voice"].get<std::string>();
    }
  }

  if (configRoot.contains("phoneme_type")) {
    auto phonemeTypeStr = configRoot["phoneme_type"].get<std::string>();
    if (phonemeTypeStr == "text") {
      phonemizeConfig.phonemeType = TextPhonemes;
    }
  }

  // phoneme to [id] map
  // Maps phonemes to one or more phoneme ids (required).
  if (configRoot.contains("phoneme_id_map")) {
    auto phonemeIdMapValue = configRoot["phoneme_id_map"];
    for (auto &fromPhonemeItem : phonemeIdMapValue.items()) {
      std::string fromPhoneme = fromPhonemeItem.key();
      if (!isSingleCodepoint(fromPhoneme)) {
        std::stringstream idsStr;
        for (auto &toIdValue : fromPhonemeItem.value()) {
          PhonemeId toId = toIdValue.get<PhonemeId>();
          idsStr << toId << ",";
        }

        spdlog::error("\"{}\" is not a single codepoint (ids={})", fromPhoneme,
                      idsStr.str());
        throw std::runtime_error(
            "Phonemes must be one codepoint (phoneme id map)");
      }

      auto fromCodepoint = getCodepoint(fromPhoneme);
      for (auto &toIdValue : fromPhonemeItem.value()) {
        PhonemeId toId = toIdValue.get<PhonemeId>();
        phonemizeConfig.phonemeIdMap[fromCodepoint].push_back(toId);
      }
    }
  }

  // phoneme to [phoneme] map
  // Maps phonemes to one or more other phonemes (not normally used).
  if (configRoot.contains("phoneme_map")) {
    if (!phonemizeConfig.phonemeMap) {
      phonemizeConfig.phonemeMap.emplace();
    }

    auto phonemeMapValue = configRoot["phoneme_map"];
    for (auto &fromPhonemeItem : phonemeMapValue.items()) {
      std::string fromPhoneme = fromPhonemeItem.key();
      if (!isSingleCodepoint(fromPhoneme)) {
        spdlog::error("\"{}\" is not a single codepoint", fromPhoneme);
        throw std::runtime_error(
            "Phonemes must be one codepoint (phoneme map)");
      }

      auto fromCodepoint = getCodepoint(fromPhoneme);
      for (auto &toPhonemeValue : fromPhonemeItem.value()) {
        std::string toPhoneme = toPhonemeValue.get<std::string>();
        if (!isSingleCodepoint(toPhoneme)) {
          throw std::runtime_error(
              "Phonemes must be one codepoint (phoneme map)");
        }

        auto toCodepoint = getCodepoint(toPhoneme);
        (*phonemizeConfig.phonemeMap)[fromCodepoint].push_back(toCodepoint);
      }
    }
  }

} /* parsePhonemizeConfig */

// Load JSON config for audio synthesis
void parseSynthesisConfig(json &configRoot, SynthesisConfig &synthesisConfig) {
  // {
  //     "audio": {
  //         "sample_rate": 22050
  //     },
  //     "inference": {
  //         "noise_scale": 0.667,
  //         "length_scale": 1,
  //         "noise_w": 0.8
  //     }
  // }

  if (configRoot.contains("audio")) {
    auto audioValue = configRoot["audio"];
    if (audioValue.contains("sample_rate")) {
      // Default sample rate is 22050 Hz
      synthesisConfig.sampleRate = audioValue.value("sample_rate", 22050);
    }
  }

  if (configRoot.contains("inference")) {
    // Overrides default inference settings
    auto inferenceValue = configRoot["inference"];
    if (inferenceValue.contains("noise_scale")) {
      synthesisConfig.noiseScale = inferenceValue.value("noise_scale", 0.667f);
    }

    if (inferenceValue.contains("length_scale")) {
      synthesisConfig.lengthScale = inferenceValue.value("length_scale", 1.0f);
    }

    if (inferenceValue.contains("noise_w")) {
      synthesisConfig.noiseW = inferenceValue.value("noise_w", 0.8f);
    }
  }

} /* parseSynthesisConfig */

void parseModelConfig(json &configRoot, ModelConfig &modelConfig) {

  modelConfig.numSpeakers = configRoot["num_speakers"].get<SpeakerId>();

  if (configRoot.contains("speaker_id_map")) {
    if (!modelConfig.speakerIdMap) {
      modelConfig.speakerIdMap.emplace();
    }

    auto speakerIdMapValue = configRoot["speaker_id_map"];
    for (auto &speakerItem : speakerIdMapValue.items()) {
      std::string speakerName = speakerItem.key();
      (*modelConfig.speakerIdMap)[speakerName] =
          speakerItem.value().get<SpeakerId>();
    }
  }

} /* parseModelConfig */

void initialize(PiperConfig &config) {
  if (config.useESpeak) {
    // Set up espeak-ng for calling espeak_TextToPhonemesWithTerminator
    // See: https://github.com/rhasspy/espeak-ng
    spdlog::debug("Initializing eSpeak");
    int result = espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS,
                                   /*buflength*/ 0,
                                   /*path*/ config.eSpeakDataPath.c_str(),
                                   /*options*/ 0);
    if (result < 0) {
      throw std::runtime_error("Failed to initialize eSpeak-ng");
    }

    spdlog::debug("Initialized eSpeak");
  }

  // Load onnx model for libtashkeel
  // https://github.com/mush42/libtashkeel/
  if (config.useTashkeel) {
    spdlog::debug("Using libtashkeel for diacritization");
    if (!config.tashkeelModelPath) {
      throw std::runtime_error("No path to libtashkeel model");
    }

    spdlog::debug("Loading libtashkeel model from {}",
                  config.tashkeelModelPath.value());
    config.tashkeelState = std::make_unique<tashkeel::State>();
    tashkeel::tashkeel_load(config.tashkeelModelPath.value(),
                            *config.tashkeelState);
    spdlog::debug("Initialized libtashkeel");
  }

  spdlog::info("Initialized piper");
}

void terminate(PiperConfig &config) {
  if (config.useESpeak) {
    // Clean up espeak-ng
    spdlog::debug("Terminating eSpeak");
    espeak_Terminate();
    spdlog::debug("Terminated eSpeak");
  }

  spdlog::info("Terminated piper");
}

void loadModel(std::string modelPath, ModelSession &session) {
  spdlog::debug("Loading onnx model from {}", modelPath);
  session.env = Ort::Env(OrtLoggingLevel::ORT_LOGGING_LEVEL_WARNING,
                         instanceName.c_str());
  session.env.DisableTelemetryEvents();

  // Slows down performance by ~2x
  // session.options.SetIntraOpNumThreads(1);

  // Roughly doubles load time for no visible inference benefit
  // session.options.SetGraphOptimizationLevel(
  //     GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

  session.options.SetGraphOptimizationLevel(
      GraphOptimizationLevel::ORT_DISABLE_ALL);

  // Slows down performance very slightly
  // session.options.SetExecutionMode(ExecutionMode::ORT_PARALLEL);

  session.options.DisableCpuMemArena();
  session.options.DisableMemPattern();
  session.options.DisableProfiling();

  auto startTime = std::chrono::steady_clock::now();
  session.onnx = Ort::Session(session.env, modelPath.c_str(), session.options);
  auto endTime = std::chrono::steady_clock::now();
  spdlog::debug("Loaded onnx model in {} second(s)",
                std::chrono::duration<double>(endTime - startTime).count());
}

// Load Onnx model and JSON config file
void loadVoice(PiperConfig &config, std::string modelPath,
               std::string modelConfigPath, Voice &voice,
               std::optional<SpeakerId> &speakerId) {
  spdlog::debug("Parsing voice config at {}", modelConfigPath);
  std::ifstream modelConfigFile(modelConfigPath);
  voice.configRoot = json::parse(modelConfigFile);

  parsePhonemizeConfig(voice.configRoot, voice.phonemizeConfig);
  parseSynthesisConfig(voice.configRoot, voice.synthesisConfig);
  parseModelConfig(voice.configRoot, voice.modelConfig);

  if (voice.modelConfig.numSpeakers > 1) {
    // Multi-speaker model
    if (speakerId) {
      voice.synthesisConfig.speakerId = speakerId;
    } else {
      // Default speaker
      voice.synthesisConfig.speakerId = 0;
    }
  }

  spdlog::debug("Voice contains {} speaker(s)", voice.modelConfig.numSpeakers);

  loadModel(modelPath, voice.session);

} /* loadVoice */

// Phoneme ids to WAV audio
void synthesize(std::vector<PhonemeId> &phonemeIds,
                SynthesisConfig &synthesisConfig, ModelSession &session,
                std::vector<int16_t> &audioBuffer, SynthesisResult &result) {
  spdlog::debug("Synthesizing audio for {} phoneme id(s)", phonemeIds.size());

  auto memoryInfo = Ort::MemoryInfo::CreateCpu(
      OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);

  // Allocate
  std::vector<int64_t> phonemeIdLengths{(int64_t)phonemeIds.size()};
  std::vector<float> scales{synthesisConfig.noiseScale,
                            synthesisConfig.lengthScale,
                            synthesisConfig.noiseW};

  std::vector<Ort::Value> inputTensors;
  std::vector<int64_t> phonemeIdsShape{1, (int64_t)phonemeIds.size()};
  inputTensors.push_back(Ort::Value::CreateTensor<int64_t>(
      memoryInfo, phonemeIds.data(), phonemeIds.size(), phonemeIdsShape.data(),
      phonemeIdsShape.size()));

  std::vector<int64_t> phomemeIdLengthsShape{(int64_t)phonemeIdLengths.size()};
  inputTensors.push_back(Ort::Value::CreateTensor<int64_t>(
      memoryInfo, phonemeIdLengths.data(), phonemeIdLengths.size(),
      phomemeIdLengthsShape.data(), phomemeIdLengthsShape.size()));

  std::vector<int64_t> scalesShape{(int64_t)scales.size()};
  inputTensors.push_back(
      Ort::Value::CreateTensor<float>(memoryInfo, scales.data(), scales.size(),
                                      scalesShape.data(), scalesShape.size()));

  // Add speaker id.
  // NOTE: These must be kept outside the "if" below to avoid being deallocated.
  std::vector<int64_t> speakerId{
      (int64_t)synthesisConfig.speakerId.value_or(0)};
  std::vector<int64_t> speakerIdShape{(int64_t)speakerId.size()};

  if (synthesisConfig.speakerId) {
    inputTensors.push_back(Ort::Value::CreateTensor<int64_t>(
        memoryInfo, speakerId.data(), speakerId.size(), speakerIdShape.data(),
        speakerIdShape.size()));
  }

  // From export_onnx.py
  std::array<const char *, 4> inputNames = {"input", "input_lengths", "scales",
                                            "sid"};
  std::array<const char *, 1> outputNames = {"output"};

  // Infer
  auto startTime = std::chrono::steady_clock::now();
  auto outputTensors = session.onnx.Run(
      Ort::RunOptions{nullptr}, inputNames.data(), inputTensors.data(),
      inputTensors.size(), outputNames.data(), outputNames.size());
  auto endTime = std::chrono::steady_clock::now();

  if ((outputTensors.size() != 1) || (!outputTensors.front().IsTensor())) {
    throw std::runtime_error("Invalid output tensors");
  }
  auto inferDuration = std::chrono::duration<double>(endTime - startTime);
  result.inferSeconds = inferDuration.count();

  const float *audio = outputTensors.front().GetTensorData<float>();
  auto audioShape =
      outputTensors.front().GetTensorTypeAndShapeInfo().GetShape();
  int64_t audioCount = audioShape[audioShape.size() - 1];

  result.audioSeconds = (double)audioCount / (double)synthesisConfig.sampleRate;
  result.realTimeFactor = 0.0;
  if (result.audioSeconds > 0) {
    result.realTimeFactor = result.inferSeconds / result.audioSeconds;
  }
  spdlog::debug("Synthesized {} second(s) of audio in {} second(s)",
                result.audioSeconds, result.inferSeconds);

  // Get max audio value for scaling
  float maxAudioValue = 0.01f;
  for (int64_t i = 0; i < audioCount; i++) {
    float audioValue = abs(audio[i]);
    if (audioValue > maxAudioValue) {
      maxAudioValue = audioValue;
    }
  }

  // We know the size up front
  audioBuffer.reserve(audioCount);

  // Scale audio to fill range and convert to int16
  float audioScale = (MAX_WAV_VALUE / std::max(0.01f, maxAudioValue));
  for (int64_t i = 0; i < audioCount; i++) {
    int16_t intAudioValue = static_cast<int16_t>(
        std::clamp(audio[i] * audioScale,
                   static_cast<float>(std::numeric_limits<int16_t>::min()),
                   static_cast<float>(std::numeric_limits<int16_t>::max())));

    audioBuffer.push_back(intAudioValue);
  }

  // Clean up
  for (std::size_t i = 0; i < outputTensors.size(); i++) {
    Ort::detail::OrtRelease(outputTensors[i].release());
  }

  for (std::size_t i = 0; i < inputTensors.size(); i++) {
    Ort::detail::OrtRelease(inputTensors[i].release());
  }
}

// ----------------------------------------------------------------------------

// Phonemize text and synthesize audio
void textToAudio(PiperConfig &config, Voice &voice, std::string text,
                 std::vector<int16_t> &audioBuffer, SynthesisResult &result,
                 const std::function<void()> &audioCallback) {

  std::size_t sentenceSilenceSamples = 0;
  if (voice.synthesisConfig.sentenceSilenceSeconds > 0) {
    sentenceSilenceSamples = (std::size_t)(
        voice.synthesisConfig.sentenceSilenceSeconds *
        voice.synthesisConfig.sampleRate * voice.synthesisConfig.channels);
  }

  if (config.useTashkeel) {
    if (!config.tashkeelState) {
      throw std::runtime_error("Tashkeel model is not loaded");
    }

    spdlog::debug("Diacritizing text with libtashkeel: {}", text);
    text = tashkeel::tashkeel_run(text, *config.tashkeelState);
  }

  // Phonemes for each sentence
  spdlog::debug("Phonemizing text: {}", text);
  std::vector<std::vector<Phoneme>> phonemes;

  if (voice.phonemizeConfig.phonemeType == eSpeakPhonemes) {
    // Use espeak-ng for phonemization
    eSpeakPhonemeConfig eSpeakConfig;
    eSpeakConfig.voice = voice.phonemizeConfig.eSpeak.voice;
    phonemize_eSpeak(text, eSpeakConfig, phonemes);
  } else {
    // Use UTF-8 codepoints as "phonemes"
    CodepointsPhonemeConfig codepointsConfig;
    phonemize_codepoints(text, codepointsConfig, phonemes);
  }

  // Synthesize each sentence independently.
  std::vector<PhonemeId> phonemeIds;
  std::map<Phoneme, std::size_t> missingPhonemes;
  for (auto phonemesIter = phonemes.begin(); phonemesIter != phonemes.end();
       ++phonemesIter) {
    std::vector<Phoneme> &sentencePhonemes = *phonemesIter;

    if (spdlog::should_log(spdlog::level::debug)) {
      // DEBUG log for phonemes
      std::string phonemesStr;
      for (auto phoneme : sentencePhonemes) {
        utf8::append(phoneme, phonemesStr);
      }

      spdlog::debug("Converting {} phoneme(s) to ids: {}",
                    sentencePhonemes.size(), phonemesStr);
    }

    SynthesisResult sentenceResult;

    // Use phoneme/id map from config
    PhonemeIdConfig idConfig;
    idConfig.phonemeIdMap =
        std::make_shared<PhonemeIdMap>(voice.phonemizeConfig.phonemeIdMap);

    // phonemes -> ids
    phonemes_to_ids(sentencePhonemes, idConfig, phonemeIds, missingPhonemes);
    if (spdlog::should_log(spdlog::level::debug)) {
      // DEBUG log for phoneme ids
      std::stringstream phonemeIdsStr;
      for (auto phonemeId : phonemeIds) {
        phonemeIdsStr << phonemeId << ", ";
      }

      spdlog::debug("Converted {} phoneme(s) to {} phoneme id(s): {}",
                    sentencePhonemes.size(), phonemeIds.size(),
                    phonemeIdsStr.str());
    }

    // ids -> audio
    synthesize(phonemeIds, voice.synthesisConfig, voice.session, audioBuffer,
               sentenceResult);

    // Add end of sentence silence
    if (sentenceSilenceSamples > 0) {
      for (std::size_t i = 0; i < sentenceSilenceSamples; i++) {
        audioBuffer.push_back(0);
      }
    }

    if (audioCallback) {
      // Call back must copy audio since it is cleared afterwards.
      audioCallback();
      audioBuffer.clear();
    }

    result.audioSeconds += sentenceResult.audioSeconds;
    result.inferSeconds += sentenceResult.inferSeconds;

    phonemeIds.clear();
  }

  if (missingPhonemes.size() > 0) {
    spdlog::warn("Missing {} phoneme(s) from phoneme/id map!",
                 missingPhonemes.size());

    for (auto phonemeCount : missingPhonemes) {
      std::string phonemeStr;
      utf8::append(phonemeCount.first, phonemeStr);
      spdlog::warn("Missing \"{}\" (\\u{:04X}): {} time(s)", phonemeStr,
                   (uint32_t)phonemeCount.first, phonemeCount.second);
    }
  }

  if (result.audioSeconds > 0) {
    result.realTimeFactor = result.inferSeconds / result.audioSeconds;
  }

} /* textToAudio */

// Phonemize text and synthesize audio to WAV file
void textToWavFile(PiperConfig &config, Voice &voice, std::string text,
                   std::ostream &audioFile, SynthesisResult &result) {

  std::vector<int16_t> audioBuffer;
  textToAudio(config, voice, text, audioBuffer, result, NULL);

  // Write WAV
  auto synthesisConfig = voice.synthesisConfig;
  writeWavHeader(synthesisConfig.sampleRate, synthesisConfig.sampleWidth,
                 synthesisConfig.channels, (int32_t)audioBuffer.size(),
                 audioFile);

  audioFile.write((const char *)audioBuffer.data(),
                  sizeof(int16_t) * audioBuffer.size());

} /* textToWavFile */

} // namespace piper
