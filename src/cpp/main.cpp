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
#include <optional>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

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

#include "json.hpp"
#include "piper.hpp"

using namespace std;
using json = nlohmann::json;

enum OutputType { OUTPUT_FILE, OUTPUT_DIRECTORY, OUTPUT_STDOUT, OUTPUT_RAW };

struct RunConfig {
  // Path to .onnx voice file
  filesystem::path modelPath;

  // Path to JSON voice config file
  filesystem::path modelConfigPath;

  // Type of output to produce.
  // Default is to write a WAV file in the current directory.
  OutputType outputType = OUTPUT_DIRECTORY;

  // Path for output
  optional<filesystem::path> outputPath = filesystem::path(".");

  // Numerical id of the default speaker (multi-speaker voices)
  optional<piper::SpeakerId> speakerId;

  // Amount of noise to add during audio generation
  optional<float> noiseScale;

  // Speed of speaking (1 = normal, < 1 is faster, > 1 is slower)
  optional<float> lengthScale;

  // Variation in phoneme lengths
  optional<float> noiseW;

  // Seconds of silence to add after each sentence
  optional<float> sentenceSilenceSeconds;

  // Path to espeak-ng data directory (default is next to piper executable)
  optional<filesystem::path> eSpeakDataPath;

  // Path to libtashkeel ort model
  // https://github.com/mush42/libtashkeel/
  optional<filesystem::path> tashkeelModelPath;

  // stdin input is lines of JSON instead of text with format:
  // {
  //   "text": str,               (required)
  //   "speaker_id": int,         (optional)
  //   "speaker": str,            (optional)
  //   "output_file": str,        (optional)
  // }
  bool jsonInput = false;

  // Seconds of extra silence to insert after a single phoneme
  optional<std::map<piper::Phoneme, float>> phonemeSilenceSeconds;

  // true to use CUDA execution provider
  bool useCuda = false;
};

void parseArgs(int argc, char *argv[], RunConfig &runConfig);
void rawOutputProc(vector<int16_t> &sharedAudioBuffer, mutex &mutAudio,
                   condition_variable &cvAudio, bool &audioReady,
                   bool &audioFinished);

void loadVoiceModel(piper::PiperConfig& piperConfig, piper::Voice& voice, const RunConfig& runConfig, const filesystem::path& exePath) {
  unloadVoice(voice);

  spdlog::debug("Loading voice from {} (config={})",
              runConfig.modelPath.string(),
              runConfig.modelConfigPath.string());

  std::optional<piper::SpeakerId> speakerId = runConfig.speakerId;
  auto startTime = chrono::steady_clock::now();
  loadVoice(piperConfig, runConfig.modelPath.string(),
            runConfig.modelConfigPath.string(), voice, speakerId,
            runConfig.useCuda);
  auto endTime = chrono::steady_clock::now();
  spdlog::info("Loaded voice in {} second(s)",
               chrono::duration<double>(endTime - startTime).count());

    if (voice.phonemizeConfig.phonemeType == piper::eSpeakPhonemes) {
    spdlog::debug("Voice uses eSpeak phonemes ({})",
                  voice.phonemizeConfig.eSpeak.voice);

    if (runConfig.eSpeakDataPath) {
      // User provided path
      piperConfig.eSpeakDataPath = runConfig.eSpeakDataPath.value().string();
    } else {
      // Assume next to piper executable
      piperConfig.eSpeakDataPath =
          std::filesystem::absolute(
              exePath.parent_path().append("espeak-ng-data"))
              .string();

      spdlog::debug("espeak-ng-data directory is expected at {}",
                    piperConfig.eSpeakDataPath);
    }
  } else {
    // Not using eSpeak
    piperConfig.useESpeak = false;
  }

  // Enable libtashkeel for Arabic
  if (voice.phonemizeConfig.eSpeak.voice == "ar") {
    piperConfig.useTashkeel = true;
    if (runConfig.tashkeelModelPath) {
      // User provided path
      piperConfig.tashkeelModelPath =
          runConfig.tashkeelModelPath.value().string();
    } else {
      // Assume next to piper executable
      piperConfig.tashkeelModelPath =
          std::filesystem::absolute(
              exePath.parent_path().append("libtashkeel_model.ort"))
              .string();

      spdlog::debug("libtashkeel model is expected at {}",
                    piperConfig.tashkeelModelPath.value());
    }
  }

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

  if (runConfig.sentenceSilenceSeconds) {
    voice.synthesisConfig.sentenceSilenceSeconds =
        runConfig.sentenceSilenceSeconds.value();
  } else {
    voice.synthesisConfig.sentenceSilenceSeconds = 0;
  }

  if (runConfig.phonemeSilenceSeconds) {
    if (!voice.synthesisConfig.phonemeSilenceSeconds) {
      // Overwrite
      voice.synthesisConfig.phonemeSilenceSeconds =
          runConfig.phonemeSilenceSeconds;
    } else {
      // Merge
      for (const auto &[phoneme, silenceSeconds] :
           *runConfig.phonemeSilenceSeconds) {
        voice.synthesisConfig.phonemeSilenceSeconds->try_emplace(
            phoneme, silenceSeconds);
      }
    }

  } // if phonemeSilenceSeconds
}

void synthesizeAudio(piper::PiperConfig& piperConfig, piper::Voice& voice, const std::string& text, const RunConfig& runConfig) {
    std::string line = text;
    piper::SynthesisResult result;

    auto outputType = runConfig.outputType;
    auto speakerId = voice.synthesisConfig.speakerId;
    std::optional<filesystem::path> maybeOutputPath = runConfig.outputPath;

    if (runConfig.jsonInput) {
        // Each line is a JSON object
        json lineRoot = json::parse(line);

        // Text is required
        line = lineRoot["text"].get<std::string>();

        if (lineRoot.contains("output_file")) {
            // Override output WAV file path
            outputType = OUTPUT_FILE;
            maybeOutputPath =
                filesystem::path(lineRoot["output_file"].get<std::string>());
        }

        if (lineRoot.contains("speaker_id")) {
            // Override speaker id
            voice.synthesisConfig.speakerId =
                lineRoot["speaker_id"].get<piper::SpeakerId>();
        } else if (lineRoot.contains("speaker")) {
            // Resolve to id using speaker id map
            auto speakerName = lineRoot["speaker"].get<std::string>();
            if ((voice.modelConfig.speakerIdMap) &&
                (voice.modelConfig.speakerIdMap->count(speakerName) > 0)) {
                voice.synthesisConfig.speakerId =
                    (*voice.modelConfig.speakerIdMap)[speakerName];
            } else {
                spdlog::warn("No speaker named: {}", speakerName);
            }
        }
    }

    // Timestamp is used for path to output WAV file
    //const auto now = std::chrono::system_clock::now();
    //const auto timestamp =
    //    std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch())
    //        .count();

    if (outputType == OUTPUT_DIRECTORY) {
        // Generate path using timestamp
        std::stringstream outputName;
        outputName << "out.wav"; //outputName << timestamp << ".wav";
        filesystem::path outputPath = runConfig.outputPath.value();
        outputPath.append(outputName.str());

        // Output audio to automatically-named WAV file in a directory
        std::ofstream audioFile(outputPath.string(), std::ios::binary);
        piper::textToWavFile(piperConfig, voice, line, audioFile, result);
        std::cout << outputPath.string() << std::endl;
    } else if (outputType == OUTPUT_FILE) {
        if (!maybeOutputPath || maybeOutputPath->empty()) {
            throw std::runtime_error("No output path provided");
        }

        filesystem::path outputPath = maybeOutputPath.value();

        // Output audio to WAV file
        std::ofstream audioFile(outputPath.string(), std::ios::binary);
        piper::textToWavFile(piperConfig, voice, line, audioFile, result);
        std::cout << outputPath.string() << std::endl;
    } else if (outputType == OUTPUT_STDOUT) {
        // Output WAV to stdout
        piper::textToWavFile(piperConfig, voice, line, std::cout, result);
    } else if (outputType == OUTPUT_RAW) {
        // Raw output to stdout
        std::mutex mutAudio;
        std::condition_variable cvAudio;
        bool audioReady = false;
        bool audioFinished = false;
        std::vector<int16_t> audioBuffer;
        std::vector<int16_t> sharedAudioBuffer;

#ifdef _WIN32
        // Needed on Windows to avoid terminal conversions
        setmode(fileno(stdout), O_BINARY);
        setmode(fileno(stdin), O_BINARY);
#endif

        std::thread rawOutputThread(rawOutputProc, std::ref(sharedAudioBuffer),
                                    std::ref(mutAudio), std::ref(cvAudio), std::ref(audioReady),
                                    std::ref(audioFinished));
        auto audioCallback = [&audioBuffer, &sharedAudioBuffer, &mutAudio,
                              &cvAudio, &audioReady]() {
            // Signal thread that audio is ready
            {
                std::unique_lock lockAudio(mutAudio);
                std::copy(audioBuffer.begin(), audioBuffer.end(),
                          std::back_inserter(sharedAudioBuffer));
                audioReady = true;
                cvAudio.notify_one();
            }
        };
        piper::textToAudio(piperConfig, voice, line, audioBuffer, result,
                           audioCallback);

        // Signal thread that there is no more audio
        {
            std::unique_lock lockAudio(mutAudio);
            audioReady = true;
            audioFinished = true;
            cvAudio.notify_one();
        }

        // Wait for audio output to finish
        spdlog::info("Waiting for audio to finish playing...");
        rawOutputThread.join();
    }

    spdlog::info("Real-time factor: {} (infer={} sec, audio={} sec)",
                 result.realTimeFactor, result.inferSeconds,
                 result.audioSeconds);

    // Restore config (--json-input)
    voice.synthesisConfig.speakerId = speakerId;
}


int main(int argc, char *argv[]) {
    spdlog::set_default_logger(spdlog::stderr_color_st("piper"));

    RunConfig runConfig;
    //parseArgs(argc, argv, runConfig);

    piper::PiperConfig piperConfig;
    //piper::Voice voice;
    std::unique_ptr<piper::Voice> currentVoice;
    bool modelLoaded = false;

    // Get the path to the piper executable so we can locate espeak-ng-data, etc.
    // next to it.
    filesystem::path exePath;
  #ifdef _MSC_VER
    exePath = []() {
      wchar_t moduleFileName[MAX_PATH] = {0};
      GetModuleFileNameW(nullptr, moduleFileName, std::size(moduleFileName));
      return filesystem::path(moduleFileName);
    }();
  #else
  #ifdef __APPLE__
    exePath = []() {
      char moduleFileName[PATH_MAX] = {0};
      uint32_t moduleFileNameSize = std::size(moduleFileName);
      _NSGetExecutablePath(moduleFileName, &moduleFileNameSize);
      return filesystem::path(moduleFileName);
    }();
  #else
    exePath = filesystem::canonical("/proc/self/exe");
  #endif
  #endif

    #include <sstream>  // Include for stringstream

while (true) {
    std::cout << "Enter command (load_model <path>, synthesize <text>, exit): ";
    std::string inputLine;
    if (!std::getline(std::cin, inputLine)) {
        spdlog::info("No input received, exiting.");
        break;
    }

    spdlog::info("Received input: {}", inputLine);

    std::istringstream iss(inputLine);
    std::string command;
    iss >> command;

    if (command == "load_model") {
        std::string modelPath;
        iss >> std::ws; // Eat up any leading whitespace
        std::getline(iss, modelPath); // Get the rest of the line as modelPath

        if (modelPath.empty()) {
            spdlog::error("Error: Missing model path");
            continue;
        }

        try {
            runConfig.modelPath = modelPath;
            runConfig.modelConfigPath = filesystem::path(runConfig.modelPath.string() + ".json");
            // Create a new Voice instance
            currentVoice = std::make_unique<piper::Voice>();
            loadVoiceModel(piperConfig, *currentVoice, runConfig, exePath);
            if (!modelLoaded) {
              piper::initialize(piperConfig);
            }
            modelLoaded = true;
            std::cout << "\nModel loaded\n"; // Status message
        } catch (const std::exception &e) {
            spdlog::error("Error loading model: {}", e.what());
        }
    } else if (command == "synthesize") {
        if (!modelLoaded) {
            spdlog::error("Error: No model loaded. Please load a model using the 'load_model' command.");
            continue;
        }

        std::string text;
        iss >> std::ws; // Eat up any leading whitespace
        std::getline(iss, text); // Get the rest of the line as text

        if (text.empty()) {
            spdlog::error("Error: Missing text for synthesize");
            continue;
        }

        try {
            synthesizeAudio(piperConfig, *currentVoice, text, runConfig);
            std::cout << "\nSynthesis complete\n"; // Status message
        } catch (const std::exception &e) {
            spdlog::error("Error synthesizing audio: {}", e.what());
        }
    } else if (command == "exit") {
        spdlog::info("Exiting program.");
        break;
    } else {
        spdlog::error("Error: Invalid command: {}", command);
    }
  }

// Clean up
if (currentVoice) {
    unloadVoice(*currentVoice);
}
piper::terminate(piperConfig);

#ifdef _WIN32
    system("pause");
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

void parseArgs(int argc, char *argv[], RunConfig &runConfig) {
    // Dummy implementation for argument parsing
    // Update this based on your actual requirements
}

// void ensureArg(int argc, char *argv[], int argi) {
//   if ((argi + 1) >= argc) {
//     //printUsage(argv);
//     exit(0);
//   }
// }

// // Parse command-line arguments
// void parseArgs(int argc, char *argv[], RunConfig &runConfig) {
//   optional<filesystem::path> modelConfigPath;

//   for (int i = 1; i < argc; i++) {
//     std::string arg = argv[i];

//     if (arg == "-m" || arg == "--model") {
//       ensureArg(argc, argv, i);
//       runConfig.modelPath = filesystem::path(argv[++i]);
//     } else if (arg == "-c" || arg == "--config") {
//       ensureArg(argc, argv, i);
//       modelConfigPath = filesystem::path(argv[++i]);
//     } else if (arg == "-f" || arg == "--output_file" ||
//                arg == "--output-file") {
//       ensureArg(argc, argv, i);
//       std::string filePath = argv[++i];
//       if (filePath == "-") {
//         runConfig.outputType = OUTPUT_STDOUT;
//         runConfig.outputPath = nullopt;
//       } else {
//         runConfig.outputType = OUTPUT_FILE;
//         runConfig.outputPath = filesystem::path(filePath);
//       }
//     } else if (arg == "-d" || arg == "--output_dir" || arg == "output-dir") {
//       ensureArg(argc, argv, i);
//       runConfig.outputType = OUTPUT_DIRECTORY;
//       runConfig.outputPath = filesystem::path(argv[++i]);
//     } else if (arg == "--output_raw" || arg == "--output-raw") {
//       runConfig.outputType = OUTPUT_RAW;
//     } else if (arg == "-s" || arg == "--speaker") {
//       ensureArg(argc, argv, i);
//       runConfig.speakerId = (piper::SpeakerId)stol(argv[++i]);
//     } else if (arg == "--noise_scale" || arg == "--noise-scale") {
//       ensureArg(argc, argv, i);
//       runConfig.noiseScale = stof(argv[++i]);
//     } else if (arg == "--length_scale" || arg == "--length-scale") {
//       ensureArg(argc, argv, i);
//       runConfig.lengthScale = stof(argv[++i]);
//     } else if (arg == "--noise_w" || arg == "--noise-w") {
//       ensureArg(argc, argv, i);
//       runConfig.noiseW = stof(argv[++i]);
//     } else if (arg == "--sentence_silence" || arg == "--sentence-silence") {
//       ensureArg(argc, argv, i);
//       runConfig.sentenceSilenceSeconds = stof(argv[++i]);
//     } else if (arg == "--phoneme_silence" || arg == "--phoneme-silence") {
//       ensureArg(argc, argv, i);
//       ensureArg(argc, argv, i + 1);
//       auto phonemeStr = std::string(argv[++i]);
//       if (!piper::isSingleCodepoint(phonemeStr)) {
//         std::cerr << "Phoneme '" << phonemeStr
//                   << "' is not a single codepoint (--phoneme_silence)"
//                   << std::endl;
//         exit(1);
//       }

//       if (!runConfig.phonemeSilenceSeconds) {
//         runConfig.phonemeSilenceSeconds.emplace();
//       }

//       auto phoneme = piper::getCodepoint(phonemeStr);
//       (*runConfig.phonemeSilenceSeconds)[phoneme] = stof(argv[++i]);
//     } else if (arg == "--espeak_data" || arg == "--espeak-data") {
//       ensureArg(argc, argv, i);
//       runConfig.eSpeakDataPath = filesystem::path(argv[++i]);
//     } else if (arg == "--tashkeel_model" || arg == "--tashkeel-model") {
//       ensureArg(argc, argv, i);
//       runConfig.tashkeelModelPath = filesystem::path(argv[++i]);
//     } else if (arg == "--json_input" || arg == "--json-input") {
//       runConfig.jsonInput = true;
//     } else if (arg == "--use_cuda" || arg == "--use-cuda") {
//       runConfig.useCuda = true;
//     } else if (arg == "--version") {
//       std::cout << piper::getVersion() << std::endl;
//       exit(0);
//     } else if (arg == "--debug") {
//       // Set DEBUG logging
//       spdlog::set_level(spdlog::level::debug);
//     } else if (arg == "-q" || arg == "--quiet") {
//       // diable logging
//       spdlog::set_level(spdlog::level::off);
//     } else if (arg == "-h" || arg == "--help") {
//       //printUsage(argv);
//       exit(0);
//     }
//   }

//   // Verify model file exists
//   ifstream modelFile(runConfig.modelPath.c_str(), ios::binary);
//   if (!modelFile.good()) {
//     throw runtime_error("Model file doesn't exist");
//   }

//   if (!modelConfigPath) {
//     runConfig.modelConfigPath =
//         filesystem::path(runConfig.modelPath.string() + ".json");
//   } else {
//     runConfig.modelConfigPath = modelConfigPath.value();
//   }

//   // Verify model config exists
//   ifstream modelConfigFile(runConfig.modelConfigPath.c_str());
//   if (!modelConfigFile.good()) {
//     throw runtime_error("Model config doesn't exist");
//   }
// }
