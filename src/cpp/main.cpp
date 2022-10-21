#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/program_options.hpp>

#ifdef HAVE_PCAUDIO
// https://github.com/espeak-ng/pcaudiolib
#include <pcaudiolib/audio.h>
#endif

#include "api.hpp"

using namespace std;
namespace po = boost::program_options;

struct RunConfig {
  filesystem::path modelPath;
  filesystem::path modelConfigPath;
  optional<filesystem::path> outputDirectory;
};

void parseArgs(int argc, char *argv[], RunConfig &runConfig);

int main(int argc, char *argv[]) {
  RunConfig runConfig;
  parseArgs(argc, argv, runConfig);

  larynx::initialize();

  larynx::Voice voice;
  auto startTime = chrono::steady_clock::now();
  loadVoice(runConfig.modelPath.string(), runConfig.modelConfigPath.string(),
            voice);
  auto endTime = chrono::steady_clock::now();
  auto loadSeconds = chrono::duration<double>(endTime - startTime).count();
  cerr << "Load time: " << loadSeconds << " sec" << endl;

#ifdef HAVE_PCAUDIO
  audio_object *my_audio = nullptr;

  if (!runConfig.outputDirectory) {
    // Output audio to the default audio device
    my_audio = create_audio_device_object(NULL, "larynx", "Text-to-Speech");

    // TODO: Support 32-bit sample widths
    auto audioFormat = AUDIO_OBJECT_FORMAT_S16LE;
    int error = audio_object_open(my_audio, audioFormat,
                                  voice.synthesisConfig.sampleRate,
                                  voice.synthesisConfig.channels);
    if (error != 0) {
      throw runtime_error(audio_object_strerror(my_audio, error));
    }
  }
#else
  // Cannot play audio directly
  if (!runConfig.outputDirectory) {
    // Default to current directory
    runConfig.outputDirectory = filesystem::current_path();
  }
#endif

  if (runConfig.outputDirectory) {
    runConfig.outputDirectory =
        filesystem::absolute(runConfig.outputDirectory.value());
    cerr << "Output directory: " << runConfig.outputDirectory.value() << endl;
  }

  string line;
  larynx::SynthesisResult result;
  while (getline(cin, line)) {

    // Path to output WAV file
    const auto now = chrono::system_clock::now();
    const auto timestamp =
        chrono::duration_cast<chrono::seconds>(now.time_since_epoch()).count();

    if (runConfig.outputDirectory) {
      stringstream outputName;
      outputName << timestamp << ".wav";
      filesystem::path outputPath = runConfig.outputDirectory.value();
      outputPath.append(outputName.str());

      // Output audio to WAV file
      ofstream audioFile(outputPath.string(), ios::binary);
      larynx::textToWavFile(voice, line, audioFile, result);
      cout << outputPath.string() << endl;
    } else {
#ifdef HAVE_PCAUDIO
      vector<int16_t> audioBuffer;
      larynx::textToAudio(voice, line, audioBuffer, result);

      int error = audio_object_write(my_audio, (const char *)audioBuffer.data(),
                                     sizeof(int16_t) * audioBuffer.size());
      if (error != 0) {
        throw runtime_error(audio_object_strerror(my_audio, error));
      }
      audio_object_flush(my_audio);
#else
      throw runtime_error("Should not happen");
#endif
    }

    cerr << "Real-time factor: " << result.realTimeFactor
         << " (infer=" << result.inferSeconds
         << " sec, audio=" << result.audioSeconds << " sec)" << endl;
  }

  larynx::terminate();

  audio_object_close(my_audio);
  audio_object_destroy(my_audio);
  my_audio = nullptr;

  return EXIT_SUCCESS;
}

// Parse command-line arguments
void parseArgs(int argc, char *argv[], RunConfig &runConfig) {
  string modelPathStr;
  string modelConfigPathStr;
  string outputDirectoryStr;

  // TODO: Add --stdout
  po::options_description options("Larynx options");
  options.add_options()("help", "Print help message and exit")(
      "model", po::value<string>(&modelPathStr)->required(),
      "Path to onnx model file")(
      "config", po::value<string>(&modelConfigPathStr),
      "Path to JSON model config file (default: model path + .json)")(
      "output_dir", po::value<string>(&outputDirectoryStr),
      "Path to output directory (default: cwd)");

  po::variables_map args;
  po::store(po::parse_command_line(argc, argv, options), args);

  if (args.count("help")) {
    cout << options << "\n";
    exit(EXIT_SUCCESS);
  }

  po::notify(args);

  runConfig.modelPath = filesystem::path(modelPathStr);

  // Verify model file exists
  ifstream modelFile(runConfig.modelPath.c_str(), ios::binary);
  if (!modelFile.good()) {
    throw runtime_error("Model file doesn't exist");
  }

  if (modelConfigPathStr.empty()) {
    modelConfigPathStr = modelPathStr + ".json";
  }

  runConfig.modelConfigPath = filesystem::path(modelConfigPathStr);

  // Verify model config exists
  ifstream modelConfigFile(runConfig.modelConfigPath.c_str());
  if (!modelConfigFile.good()) {
    throw runtime_error("Model config doesn't exist");
  }

  if (!outputDirectoryStr.empty()) {
    runConfig.outputDirectory = filesystem::path(outputDirectoryStr);
  }
}
