#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#ifdef HAVE_PCAUDIO
// https://github.com/espeak-ng/pcaudiolib
#include <pcaudiolib/audio.h>
#endif

#include "piper.hpp"

using namespace std;

enum OutputType {
  OUTPUT_FILE,
  OUTPUT_DIRECTORY,
  OUTPUT_STDOUT,
  OUTPUT_PLAY,
  OUTPUT_RAW
};

struct RunConfig {
  filesystem::path modelPath;
  filesystem::path modelConfigPath;
  OutputType outputType = OUTPUT_PLAY;
  optional<filesystem::path> outputPath;
  optional<piper::SpeakerId> speakerId;
  optional<float> noiseScale;
  optional<float> lengthScale;
  optional<float> noiseW;
};

void parseArgs(int argc, char *argv[], RunConfig &runConfig);
void rawOutputProc(vector<int16_t> &sharedAudioBuffer, mutex &mutAudio,
                   condition_variable &cvAudio, bool &audioReady,
                   bool &audioFinished);

#ifdef HAVE_PCAUDIO
void playProc(audio_object *my_audio, vector<int16_t> &sharedAudioBuffer,
              mutex &mutAudio, condition_variable &cvAudio, bool &audioReady,
              bool &audioFinished);
#endif

int main(int argc, char *argv[]) {
  RunConfig runConfig;
  parseArgs(argc, argv, runConfig);

  // NOTE: This won't work for Windows (need GetModuleFileName)
  auto exePath = filesystem::canonical("/proc/self/exe");
  piper::initialize(exePath.parent_path());

  piper::Voice voice;
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
    my_audio = create_audio_device_object(NULL, "piper", "Text-to-Speech");

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
    cerr << "WARNING: Piper was not compiled with pcaudiolib. Output audio "
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
  piper::SynthesisResult result;
  while (getline(cin, line)) {

    // Path to output WAV file
    const auto now = chrono::system_clock::now();
    const auto timestamp =
        chrono::duration_cast<chrono::nanoseconds>(now.time_since_epoch())
            .count();

    if (runConfig.outputType == OUTPUT_DIRECTORY) {
      stringstream outputName;
      outputName << timestamp << ".wav";
      filesystem::path outputPath = runConfig.outputPath.value();
      outputPath.append(outputName.str());

      // Output audio to automatically-named WAV file in a directory
      ofstream audioFile(outputPath.string(), ios::binary);
      piper::textToWavFile(voice, line, audioFile, result);
      cout << outputPath.string() << endl;
    } else if (runConfig.outputType == OUTPUT_FILE) {
      // Output audio to WAV file
      ofstream audioFile(runConfig.outputPath.value().string(), ios::binary);
      piper::textToWavFile(voice, line, audioFile, result);
    } else if (runConfig.outputType == OUTPUT_STDOUT) {
      // Output WAV to stdout
      piper::textToWavFile(voice, line, cout, result);
    } else if (runConfig.outputType == OUTPUT_RAW) {
      // Raw output to stdout
      mutex mutAudio;
      condition_variable cvAudio;
      bool audioReady = false;
      bool audioFinished = false;
      vector<int16_t> audioBuffer;
      vector<int16_t> sharedAudioBuffer;

      thread rawOutputThread(rawOutputProc, ref(sharedAudioBuffer),
                             ref(mutAudio), ref(cvAudio), ref(audioReady),
                             ref(audioFinished));
      auto audioCallback = [&audioBuffer, &sharedAudioBuffer, &mutAudio,
                            &cvAudio, &audioReady]() {
        // Signal thread that audio is ready
        {
          unique_lock lockAudio(mutAudio);
          copy(audioBuffer.begin(), audioBuffer.end(),
               back_inserter(sharedAudioBuffer));
          audioReady = true;
          cvAudio.notify_one();
        }
      };
      piper::textToAudio(voice, line, audioBuffer, result, audioCallback);

      // Signal thread that there is no more audio
      {
        unique_lock lockAudio(mutAudio);
        audioReady = true;
        audioFinished = true;
        cvAudio.notify_one();
      }

      // Wait for audio output to finish
      cerr << "Waiting for audio..." << endl;
      rawOutputThread.join();
    } else if (runConfig.outputType == OUTPUT_PLAY) {
#ifdef HAVE_PCAUDIO
      mutex mutAudio;
      condition_variable cvAudio;
      bool audioReady = false;
      bool audioFinished = false;
      vector<int16_t> audioBuffer;
      vector<int16_t> sharedAudioBuffer;

      thread playThread(playProc, my_audio, ref(sharedAudioBuffer),
                        ref(mutAudio), ref(cvAudio), ref(audioReady),
                        ref(audioFinished));
      auto audioCallback = [&audioBuffer, &sharedAudioBuffer, &mutAudio,
                            &cvAudio, &audioReady]() {
        // Signal thread that audio is ready
        {
          unique_lock lockAudio(mutAudio);
          copy(audioBuffer.begin(), audioBuffer.end(),
               back_inserter(sharedAudioBuffer));
          audioReady = true;
          cvAudio.notify_one();
        }
      };
      piper::textToAudio(voice, line, audioBuffer, result, audioCallback);

      // Signal thread that there is no more audio
      {
        unique_lock lockAudio(mutAudio);
        audioReady = true;
        audioFinished = true;
        cvAudio.notify_one();
      }

      // Wait for audio output to finish
      cerr << "Waiting for audio..." << endl;
      playThread.join();
#else
      throw runtime_error("Cannot play audio! Not compiled with pcaudiolib.");
#endif
    }

    cerr << "Real-time factor: " << result.realTimeFactor
         << " (infer=" << result.inferSeconds
         << " sec, audio=" << result.audioSeconds << " sec)" << endl;
  }

  piper::terminate();

#ifdef HAVE_PCAUDIO
  audio_object_close(my_audio);
  audio_object_destroy(my_audio);
  my_audio = nullptr;
#endif

  return EXIT_SUCCESS;
}

void rawOutputProc(vector<int16_t> &sharedAudioBuffer, mutex &mutAudio,
                   condition_variable &cvAudio, bool &audioReady,
                   bool &audioFinished) {
  vector<int16_t> internalAudioBuffer;
  while (true) {
    {
      unique_lock lockAudio{mutAudio};
      cvAudio.wait(lockAudio, [&audioReady] { return audioReady; });

      if (sharedAudioBuffer.empty() && audioFinished) {
        break;
      }

      copy(sharedAudioBuffer.begin(), sharedAudioBuffer.end(),
           back_inserter(internalAudioBuffer));

      sharedAudioBuffer.clear();

      if (!audioFinished) {
        audioReady = false;
      }
    }

    cout.write((const char *)internalAudioBuffer.data(),
               sizeof(int16_t) * internalAudioBuffer.size());
    cout.flush();
    internalAudioBuffer.clear();
  }

} // rawOutputProc

#ifdef HAVE_PCAUDIO
void playProc(audio_object *my_audio, vector<int16_t> &sharedAudioBuffer,
              mutex &mutAudio, condition_variable &cvAudio, bool &audioReady,
              bool &audioFinished) {
  vector<int16_t> internalAudioBuffer;
  while (true) {
    {
      unique_lock lockAudio{mutAudio};
      cvAudio.wait(lockAudio, [&audioReady] { return audioReady; });

      if (sharedAudioBuffer.empty() && audioFinished) {
        break;
      }

      copy(sharedAudioBuffer.begin(), sharedAudioBuffer.end(),
           back_inserter(internalAudioBuffer));

      sharedAudioBuffer.clear();

      if (!audioFinished) {
        audioReady = false;
      }
    }

    int error =
        audio_object_write(my_audio, (const char *)internalAudioBuffer.data(),
                           sizeof(int16_t) * internalAudioBuffer.size());
    if (error != 0) {
      throw runtime_error(audio_object_strerror(my_audio, error));
    }
    audio_object_flush(my_audio);
    internalAudioBuffer.clear();
  }

} // playProc
#endif

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
  cerr << "   --output_raw                  output raw audio to stdout as it "
          "becomes available"
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
    } else if (arg == "--output_raw") {
      runConfig.outputType = OUTPUT_RAW;
    } else if (arg == "-s" || arg == "--speaker") {
      ensureArg(argc, argv, i);
      runConfig.speakerId = (piper::SpeakerId)stol(argv[++i]);
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
