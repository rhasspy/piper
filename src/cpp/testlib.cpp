#include <fstream>
#include <functional>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "json.hpp"
#include "piper.hpp"
#include "libpiper.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

using namespace std;
using json = nlohmann::json;

int main(int argc, char* argv[]) {
  piper::PiperConfig piperConfig;
  piper::Voice voice;

  if (argc < 2) {
    std::cerr << "Need voice model path" << std::endl;
    return 1;
  }

  if (argc < 3) {
    std::cerr << "Need espeak-ng-data path" << std::endl;
    return 1;
  }

  if (argc < 4) {
    std::cerr << "Need output WAV path" << std::endl;
    return 1;
  }

  // Set logging to INFO messages
  ::setLogLevel(LIBPIPER_LEVEL_INFO);

  // Progress callback
  auto progressCallback = [](uint16_t progress, size_t total) {
    std::cout << "Audio conversion progress: " << (int)((double)progress / total * 100) << "%..." << std::endl;
  };

  auto modelPath = std::string(argv[1]);
  auto modelPathConfig = modelPath + ".json";
  piperConfig.eSpeakDataPath = std::string(argv[2]);
  auto outputPath = std::string(argv[3]);

  piper::SpeakerId speakerId;
  ::loadVoice(&piperConfig, modelPath.c_str(), modelPathConfig.c_str(), &voice, &speakerId);
  ::initializePiper(&piperConfig);

  piper::SynthesisResult result;
  ::textToWavFile(&piperConfig, &voice, "This is a test.", outputPath.c_str(), &result, progressCallback);
  ::terminatePiper(&piperConfig);

  // Output audio to WAV file
  ifstream audioFile(outputPath, ios::binary);

  // Verify that file has some data
  audioFile.seekg(0, ios_base::end);
  if (audioFile.tellg() < 10000) {
    std::cerr << "ERROR: Output file is smaller than expected!" << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "OK" << std::endl;

  return EXIT_SUCCESS;
}
