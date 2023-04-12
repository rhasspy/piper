#ifndef SYNTHESIZE_H_
#define SYNTHESIZE_H_

#include <array>
#include <chrono>
#include <limits>
#include <memory>
#include <vector>

#include <onnxruntime_cxx_api.h>

#include "config.hpp"
#include "model.hpp"

using namespace std;

namespace piper {

// Maximum value for 16-bit signed WAV sample
const float MAX_WAV_VALUE = 32767.0f;

struct SynthesisResult {
  double inferSeconds;
  double audioSeconds;
  double realTimeFactor;
};

// Phoneme ids to WAV audio
void synthesize(vector<PhonemeId> &phonemeIds, SynthesisConfig &synthesisConfig,
                ModelSession &session, vector<int16_t> &audioBuffer,
                SynthesisResult &result) {
  auto memoryInfo = Ort::MemoryInfo::CreateCpu(
      OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);

  // Allocate
  vector<int64_t> phonemeIdLengths{(int64_t)phonemeIds.size()};
  vector<float> scales{synthesisConfig.noiseScale, synthesisConfig.lengthScale,
                       synthesisConfig.noiseW};

  vector<Ort::Value> inputTensors;
  vector<int64_t> phonemeIdsShape{1, (int64_t)phonemeIds.size()};
  inputTensors.push_back(Ort::Value::CreateTensor<int64_t>(
      memoryInfo, phonemeIds.data(), phonemeIds.size(), phonemeIdsShape.data(),
      phonemeIdsShape.size()));

  vector<int64_t> phomemeIdLengthsShape{(int64_t)phonemeIdLengths.size()};
  inputTensors.push_back(Ort::Value::CreateTensor<int64_t>(
      memoryInfo, phonemeIdLengths.data(), phonemeIdLengths.size(),
      phomemeIdLengthsShape.data(), phomemeIdLengthsShape.size()));

  vector<int64_t> scalesShape{(int64_t)scales.size()};
  inputTensors.push_back(
      Ort::Value::CreateTensor<float>(memoryInfo, scales.data(), scales.size(),
                                      scalesShape.data(), scalesShape.size()));

  // Add speaker id.
  // NOTE: These must be kept outside the "if" below to avoid being deallocated.
  vector<int64_t> speakerId{(int64_t)synthesisConfig.speakerId.value_or(0)};
  vector<int64_t> speakerIdShape{(int64_t)speakerId.size()};

  if (synthesisConfig.speakerId) {
    inputTensors.push_back(Ort::Value::CreateTensor<int64_t>(
        memoryInfo, speakerId.data(), speakerId.size(), speakerIdShape.data(),
        speakerIdShape.size()));
  }

  // From export_onnx.py
  array<const char *, 4> inputNames = {"input", "input_lengths", "scales",
                                       "sid"};
  array<const char *, 1> outputNames = {"output"};

  // Infer
  auto startTime = chrono::steady_clock::now();
  auto outputTensors = session.onnx.Run(
      Ort::RunOptions{nullptr}, inputNames.data(), inputTensors.data(),
      inputTensors.size(), outputNames.data(), outputNames.size());
  auto endTime = chrono::steady_clock::now();

  if ((outputTensors.size() != 1) || (!outputTensors.front().IsTensor())) {
    throw runtime_error("Invalid output tensors");
  }
  auto inferDuration = chrono::duration<double>(endTime - startTime);
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
  float audioScale = (MAX_WAV_VALUE / max(0.01f, maxAudioValue));
  for (int64_t i = 0; i < audioCount; i++) {
    int16_t intAudioValue = static_cast<int16_t>(
        clamp(audio[i] * audioScale,
              static_cast<float>(numeric_limits<int16_t>::min()),
              static_cast<float>(numeric_limits<int16_t>::max())));

    audioBuffer.push_back(intAudioValue);
  }

  // Clean up
  for (size_t i = 0; i < outputTensors.size(); i++) {
    Ort::detail::OrtRelease(outputTensors[i].release());
  }

  for (size_t i = 0; i < inputTensors.size(); i++) {
    Ort::detail::OrtRelease(inputTensors[i].release());
  }
}
} // namespace piper

#endif // SYNTHESIZE_H_
