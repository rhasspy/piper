#include "piper.hpp"
#include "libpiper.hpp"

#include <cstring>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

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

extern "C"
{
  void setLogLevel(int logLevel) {
    spdlog::set_default_logger(spdlog::stderr_color_st("libpiper"));
    spdlog::set_level(spdlog::level::level_enum{ logLevel });
  }

  void initializePiper(PiperConfig* config) {
    piper::initialize(*config);
  }

  void terminatePiper(PiperConfig* config) {
    piper::terminate(*config);
  }

  void loadVoice(PiperConfig* config, const char* modelPath, const char* modelConfigPath, Voice* voice, SpeakerId* speakerId) {
    std::optional<piper::SpeakerId> optSpeakerId;
    if (speakerId) {
      optSpeakerId = *speakerId;
    }
    piper::loadVoice(*config, modelPath, modelConfigPath, *voice, optSpeakerId);

    // Get the path to the piper executable so we can locate espeak-ng-data, etc.
    // next to it.
#ifdef _MSC_VER
    auto exePath = []() {
      wchar_t moduleFileName[MAX_PATH] = { 0 };
      GetModuleFileNameW(nullptr, moduleFileName, std::size(moduleFileName));
      return std::filesystem::path(moduleFileName);
      }();
#else
#ifdef __APPLE__
    auto exePath = []() {
      char moduleFileName[PATH_MAX] = { 0 };
      uint32_t moduleFileNameSize = std::size(moduleFileName);
      _NSGetExecutablePath(moduleFileName, &moduleFileNameSize);
      return filesystem::path(moduleFileName);
      }();
#else
    auto exePath = filesystem::canonical("/proc/self/exe");
#endif
#endif

    // Enable libtashkeel for Arabic
    if (voice->phonemizeConfig.eSpeak.voice == "ar") {
      config->useTashkeel = true;
      // Assume next to piper executable
      config->tashkeelModelPath =
        std::filesystem::absolute(
          exePath.parent_path().append("libtashkeel_model.ort"))
        .string();
    }
  }

  void textToAudio(PiperConfig* config, Voice* voice, const char* text, SynthesisResult* result, AudioCallback audioCallback, ProgressCallback progressCallback) {
    std::vector<int16_t> audioBuf;
    piper::textToAudio(*config, *voice, text, audioBuf, *result, [&audioBuf, audioCallback] { audioCallback(audioBuf.data(), audioBuf.size()); }, progressCallback);
  }

  void textToWavFile(PiperConfig* config, Voice* voice, const char* text, const char* audioFile, SynthesisResult* result, ProgressCallback progressCallback) {
    std::string audioFilePath = audioFile;
    std::ofstream audioFileStream(audioFilePath, std::ios::binary);
    piper::textToWavFile(*config, *voice, text, audioFileStream, *result, progressCallback);
    audioFileStream.flush();
    audioFileStream.close();
  }
}
