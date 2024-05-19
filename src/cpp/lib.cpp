#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#if defined(_MSC_VER)
#define LIB_API __declspec(dllexport) // Microsoft
#elif defined(__GNUC__)
#define LIB_API extern "C" __attribute__((visibility("default"))) // GCC
#else
#define LIB_API
#endif

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "base64.hpp"
#include "json.hpp"
#include "piper.hpp"

using namespace std;
using json = nlohmann::json;

// ----------------------------------------------------------------------------

LIB_API int tester()
{
  return 420;
}

LIB_API piper::ModelConfig *LoadModelConfig(const char *configPath)
{
  return piper::LoadModelConfig(configPath);
}

LIB_API piper::SynthesisConfig *LoadSynthesisConfig(const char *configPath)
{
  return piper::LoadSynthesisConfig(configPath);
}

LIB_API piper::Voice *LoadVoice(piper::SynthesisConfig &synthConfig)
{
  return piper::LoadVoice(synthConfig);
}

LIB_API void ChangeSpeaker(piper::Voice &voice, int speaker)
{
  voice.synthesisConfig.speakerId = speaker;
}

LIB_API void ChangeLengthScale(piper::Voice &voice, float length)
{
  voice.synthesisConfig.lengthScale = length;
}

LIB_API void ChangeNoiseScale(piper::Voice &voice, float scale)
{
  voice.synthesisConfig.noiseScale = scale;
}

LIB_API void ChangeNoiseWidth(piper::Voice &voice, float width)
{
  voice.synthesisConfig.noiseScale = width;
}

LIB_API void LoadIPAData(char *path)
{
  piper::LoadIPAData(path);
}

char *GenerateVoiceData(piper::Voice &voice, piper::SynthesisConfig &synthConfig, char *text)
{
  uint32_t dataSize = 0;
  auto data = piper::textToVoice(voice, text, dataSize);

  if (synthConfig.writeFile)
  {
    // Timestamp is used for path to output WAV file
    const auto now = chrono::system_clock::now();
    const auto timestamp =
        chrono::duration_cast<chrono::nanoseconds>(now.time_since_epoch())
            .count();
    // Generate path using timestamp
    stringstream outputName;
    outputName << synthConfig.outputPath << timestamp << ".wav";

    // Output audio to automatically-named WAV file in a directory
    ofstream audioFile(outputName.str(), ios::binary);
    audioFile.write(data, dataSize);
    audioFile.close();
  }

  return data;
}

void DiscardVoiceData(char *data)
{
  free(data);
}
