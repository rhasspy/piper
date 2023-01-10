#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef HAVE_PCAUDIO
// https://github.com/espeak-ng/pcaudiolib
#include <pcaudiolib/audio.h>
#endif

#include "larynx.hpp"

using namespace std;

enum OutputType { OUTPUT_FILE, OUTPUT_DIRECTORY, OUTPUT_STDOUT, OUTPUT_PLAY };

struct RunConfig {
  filesystem::path modelPath;
  filesystem::path modelConfigPath;
  OutputType outputType = OUTPUT_PLAY;
  optional<filesystem::path> outputPath;
  optional<larynx::SpeakerId> speakerId;
  optional<float> noiseScale;
  optional<float> lengthScale;
  optional<float> noiseW;
};

void parseArgs(int argc, char *argv[], RunConfig &runConfig);

int main(int argc, char *argv[]) {
  RunConfig runConfig;
  parseArgs(argc, argv, runConfig);

  auto exePath = filesystem::path(argv[0]);
  larynx::initialize(exePath.parent_path());

  larynx::Voice voice;
  auto startTime = chrono::steady_clock::now();
  loadVoice(runConfig.modelPath.string(), runConfig.modelConfigPath.string(),
            voice, runConfig.speakerId);
  auto endTime = chrono::steady_clock::now();
  auto loadSeconds = chrono::duration<double>(endTime - startTime).count();
  cerr << "Load time: " << loadSeconds << " sec" << endl;

  // Scales
  if (runConfig.noiseScale) {
    voice.synthesisConfig.noiseScale = runConfig.noiseScale.value();
  }

  if (runConfig.lengthScale) {
    voice.synthesisConfig.lengthScale = runConfig.lengthScale.value();
  }

  if (runConfig.noiseW) {
    voice.synthesisConfig.noiseW = runConfig.noiseW.value();
  }

#ifdef HAVE_PCAUDIO
  audio_object *my_audio = nullptr;

  if (runConfig.outputType == OUTPUT_PLAY) {
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
  if (runConfig.outputType == OUTPUT_PLAY) {
    // Cannot play audio directly
    cerr << "WARNING: Larynx was not compiled with pcaudiolib. Output audio "
            "will be written to the current directory."
         << endl;
    runConfig.outputType = OUTPUT_DIRECTORY;
    runConfig.outputPath = filesystem::path(".");
  }
#endif

  if (runConfig.outputType == OUTPUT_DIRECTORY) {
    runConfig.outputPath = filesystem::absolute(runConfig.outputPath.value());
    cerr << "Output directory: " << runConfig.outputPath.value() << endl;
  }

  string line;
  larynx::SynthesisResult result;
  while (getline(cin, line)) {

    // Path to output WAV file
    const auto now = chrono::system_clock::now();
    const auto timestamp =
        chrono::duration_cast<chrono::seconds>(now.time_since_epoch()).count();

    if (runConfig.outputType == OUTPUT_DIRECTORY) {
      stringstream outputName;
      outputName << timestamp << ".wav";
      filesystem::path outputPath = runConfig.outputPath.value();
      outputPath.append(outputName.str());

      // Output audio to automatically-named WAV file in a directory
      ofstream audioFile(outputPath.string(), ios::binary);
      larynx::textToWavFile(voice, line, audioFile, result);
      cout << outputPath.string() << endl;
    } else if (runConfig.outputType == OUTPUT_FILE) {
      // Output audio to WAV file
      ofstream audioFile(runConfig.outputPath.value().string(), ios::binary);
      larynx::textToWavFile(voice, line, audioFile, result);
    } else if (runConfig.outputType == OUTPUT_STDOUT) {
      // Output WAV to stdout
      larynx::textToWavFile(voice, line, cout, result);
    } else if (runConfig.outputType == OUTPUT_PLAY) {
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
      throw runtime_error("Cannot play audio! Not compiled with pcaudiolib.");
#endif
    }

    cerr << "Real-time factor: " << result.realTimeFactor
         << " (infer=" << result.inferSeconds
         << " sec, audio=" << result.audioSeconds << " sec)" << endl;
  }

  larynx::terminate();

#ifdef HAVE_PCAUDIO
  audio_object_close(my_audio);
  audio_object_destroy(my_audio);
  my_audio = nullptr;
#endif

  return EXIT_SUCCESS;
}

void printUsage(char *argv[]) {
  cerr << endl;
  cerr << "usage: " << argv[0] << " [options]" << endl;
  cerr << endl;
  cerr << "options:" << endl;
  cerr << "   -h        --help              show this message and exit" << endl;
  cerr << "   -m  FILE  --model       FILE  path to onnx model file" << endl;
  cerr << "   -c  FILE  --config      FILE  path to model config file "
          "(default: model path + .json)"
       << endl;
  cerr << "   -f  FILE  --output_file FILE  path to output WAV file ('-' for "
          "stdout)"
       << endl;
  cerr << "   -d  DIR   --output_dir  DIR   path to output directory (default: "
          "cwd)"
       << endl;
  cerr << "   -s  NUM   --speaker     NUM   id of speaker (default: 0)" << endl;
  cerr << "   --noise-scale           NUM   generator noise (default: 0.667)"
       << endl;
  cerr << "   --length-scale          NUM   phoneme length (default: 1.0)"
       << endl;
  cerr << "   --noise-w               NUM   phonene width noise (default: 0.8)"
       << endl;
  cerr << endl;
}

void ensureArg(int argc, char *argv[], int argi) {
  if ((argi + 1) >= argc) {
    printUsage(argv);
    exit(0);
  }
}

// Parse command-line arguments
void parseArgs(int argc, char *argv[], RunConfig &runConfig) {
  optional<filesystem::path> modelConfigPath;

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    if (arg == "-m" || arg == "--model") {
      ensureArg(argc, argv, i);
      runConfig.modelPath = filesystem::path(argv[++i]);
    } else if (arg == "-c" || arg == "--config") {
      ensureArg(argc, argv, i);
      modelConfigPath = filesystem::path(argv[++i]);
    } else if (arg == "-f" || arg == "--output_file") {
      ensureArg(argc, argv, i);
      std::string filePath = argv[++i];
      if (filePath == "-") {
        runConfig.outputType = OUTPUT_STDOUT;
        runConfig.outputPath = nullopt;
      } else {
        runConfig.outputType = OUTPUT_FILE;
        runConfig.outputPath = filesystem::path(filePath);
      }
    } else if (arg == "-d" || arg == "--output_dir") {
      ensureArg(argc, argv, i);
      runConfig.outputType = OUTPUT_DIRECTORY;
      runConfig.outputPath = filesystem::path(argv[++i]);
    } else if (arg == "-s" || arg == "--speaker") {
      ensureArg(argc, argv, i);
      runConfig.speakerId = (larynx::SpeakerId)stol(argv[++i]);
    } else if (arg == "--noise-scale") {
      ensureArg(argc, argv, i);
      runConfig.noiseScale = stof(argv[++i]);
    } else if (arg == "--length-scale") {
      ensureArg(argc, argv, i);
      runConfig.lengthScale = stof(argv[++i]);
    } else if (arg == "--noise-w") {
      ensureArg(argc, argv, i);
      runConfig.noiseW = stof(argv[++i]);
    } else if (arg == "-h" || arg == "--help") {
      printUsage(argv);
      exit(0);
    }
  }

  // Verify model file exists
  ifstream modelFile(runConfig.modelPath.c_str(), ios::binary);
  if (!modelFile.good()) {
    throw runtime_error("Model file doesn't exist");
  }

  if (!modelConfigPath) {
    runConfig.modelConfigPath =
        filesystem::path(runConfig.modelPath.string() + ".json");
  } else {
    runConfig.modelConfigPath = modelConfigPath.value();
  }

  // Verify model config exists
  ifstream modelConfigFile(runConfig.modelConfigPath.c_str());
  if (!modelConfigFile.good()) {
    throw runtime_error("Model config doesn't exist");
  }
}
