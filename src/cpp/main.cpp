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

enum OutputType
{
  OUTPUT_FILE,
  OUTPUT_DIRECTORY,
  OUTPUT_STDOUT,
  OUTPUT_RAW
};

struct RunConfig
{
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

// ----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
  spdlog::set_default_logger(spdlog::stderr_color_st("piper"));

  RunConfig runConfig;
  parseArgs(argc, argv, runConfig);

#ifdef _WIN32
  // Required on Windows to show IPA symbols
  SetConsoleOutputCP(CP_UTF8);
#endif

  piper::PiperConfig piperConfig;
  piper::Voice voice;

  spdlog::debug("Loading voice from {} (config={})",
                runConfig.modelPath.string(),
                runConfig.modelConfigPath.string());

  auto startTime = chrono::steady_clock::now();
  loadVoice(piperConfig, runConfig.modelPath.string(),
            runConfig.modelConfigPath.string(), voice, runConfig.speakerId,
            runConfig.useCuda);
  auto endTime = chrono::steady_clock::now();
  spdlog::info("Loaded voice in {} second(s)",
               chrono::duration<double>(endTime - startTime).count());

#ifdef _MSC_VER
  auto exePath = []()
  {
    wchar_t moduleFileName[MAX_PATH] = {0};
    GetModuleFileNameW(nullptr, moduleFileName, (DWORD)std::size(moduleFileName));
    return filesystem::path(moduleFileName);
  }();
#else
#ifdef __APPLE__
  auto exePath = []()
  {
    char moduleFileName[PATH_MAX] = {0};
    uint32_t moduleFileNameSize = std::size(moduleFileName);
    _NSGetExecutablePath(moduleFileName, &moduleFileNameSize);
    return filesystem::path(moduleFileName);
  }();
#else
  auto exePath = filesystem::canonical("/proc/self/exe");
#endif
#endif

  piper::initialize(piperConfig);

  // Scales
  if (runConfig.noiseScale)
  {
    voice.synthesisConfig.noiseScale = runConfig.noiseScale.value();
  }

  if (runConfig.lengthScale)
  {
    voice.synthesisConfig.lengthScale = runConfig.lengthScale.value();
  }

  if (runConfig.noiseW)
  {
    voice.synthesisConfig.noiseW = runConfig.noiseW.value();
  }

  if (runConfig.sentenceSilenceSeconds)
  {
    voice.synthesisConfig.sentenceSilenceSeconds =
        runConfig.sentenceSilenceSeconds.value();
  }

  if (runConfig.phonemeSilenceSeconds)
  {
    if (!voice.synthesisConfig.phonemeSilenceSeconds)
    {
      // Overwrite
      voice.synthesisConfig.phonemeSilenceSeconds =
          runConfig.phonemeSilenceSeconds;
    }
    else
    {
      // Merge
      for (const auto &[phoneme, silenceSeconds] :
           *runConfig.phonemeSilenceSeconds)
      {
        voice.synthesisConfig.phonemeSilenceSeconds->try_emplace(
            phoneme, silenceSeconds);
      }
    }

  } // if phonemeSilenceSeconds

  if (runConfig.outputType == OUTPUT_DIRECTORY)
  {
    runConfig.outputPath = filesystem::absolute(runConfig.outputPath.value());
  }

  string line;
  piper::SynthesisResult result;

  cout << "-READY-" << endl;

  while (true)
  {
    if (getline(cin, line))
    {
      auto speaker_string = line.substr(0, 3);
      auto text_string = line.substr(3);

      auto outputType = runConfig.outputType;
      auto speakerId = stoi(speaker_string);
      voice.synthesisConfig.speakerId = speakerId;
      std::optional<filesystem::path> maybeOutputPath = runConfig.outputPath;

      // Timestamp is used for path to output WAV file
      const auto now = chrono::system_clock::now();
      const auto timestamp =
          chrono::duration_cast<chrono::nanoseconds>(now.time_since_epoch())
              .count();

      if (outputType == OUTPUT_DIRECTORY)
      {
        // Generate path using timestamp
        stringstream outputName;
        outputName << timestamp << ".wav";
        filesystem::path outputPath = runConfig.outputPath.value();
        outputPath.append(outputName.str());

        // Output audio to automatically-named WAV file in a directory
        ofstream audioFile(outputPath.string(), ios::binary);
        piper::textToWavFile(piperConfig, voice, text_string, audioFile, result);
        std::cout << std::endl
                  << "-FILEPATH-" << outputPath.string() << std::endl;
      }
    }
    else
    {
      std::this_thread::sleep_for(10ms);
    }
  }

  piper::terminate(piperConfig);

  return EXIT_SUCCESS;
}

// ----------------------------------------------------------------------------

void rawOutputProc(vector<int16_t> &sharedAudioBuffer, mutex &mutAudio,
                   condition_variable &cvAudio, bool &audioReady,
                   bool &audioFinished)
{
  vector<int16_t> internalAudioBuffer;
  while (true)
  {
    {
      unique_lock lockAudio{mutAudio};
      cvAudio.wait(lockAudio, [&audioReady]
                   { return audioReady; });

      if (sharedAudioBuffer.empty() && audioFinished)
      {
        break;
      }

      copy(sharedAudioBuffer.begin(), sharedAudioBuffer.end(),
           back_inserter(internalAudioBuffer));

      sharedAudioBuffer.clear();

      if (!audioFinished)
      {
        audioReady = false;
      }
    }

    cout.write((const char *)internalAudioBuffer.data(),
               sizeof(int16_t) * internalAudioBuffer.size());
    cout.flush();
    internalAudioBuffer.clear();
  }

} // rawOutputProc

// ----------------------------------------------------------------------------

void printUsage(char *argv[])
{
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
  cerr << "   --noise_scale           NUM   generator noise (default: 0.667)"
       << endl;
  cerr << "   --length_scale          NUM   phoneme length (default: 1.0)"
       << endl;
  cerr << "   --noise_w               NUM   phoneme width noise (default: 0.8)"
       << endl;
  cerr << "   --sentence_silence      NUM   seconds of silence after each "
          "sentence (default: 0.2)"
       << endl;
  cerr << "   --json-input                  stdin input is lines of JSON "
          "instead of plain text"
       << endl;
  cerr << "   --use-cuda                    use CUDA execution provider"
       << endl;
  cerr << "   --debug                       print DEBUG messages to the console"
       << endl;
  cerr << "   -q       --quiet              disable logging" << endl;
  cerr << endl;
}

void ensureArg(int argc, char *argv[], int argi)
{
  if ((argi + 1) >= argc)
  {
    printUsage(argv);
    exit(0);
  }
}

// Parse command-line arguments
void parseArgs(int argc, char *argv[], RunConfig &runConfig)
{
  optional<filesystem::path> modelConfigPath;

  for (int i = 1; i < argc; i++)
  {
    std::string arg = argv[i];

    if (arg == "-m" || arg == "--model")
    {
      ensureArg(argc, argv, i);
      runConfig.modelPath = filesystem::path(argv[++i]);
    }
    else if (arg == "-c" || arg == "--config")
    {
      ensureArg(argc, argv, i);
      modelConfigPath = filesystem::path(argv[++i]);
    }
    else if (arg == "-f" || arg == "--output_file" ||
             arg == "--output-file")
    {
      ensureArg(argc, argv, i);
      std::string filePath = argv[++i];
      if (filePath == "-")
      {
        runConfig.outputType = OUTPUT_STDOUT;
        runConfig.outputPath = nullopt;
      }
      else
      {
        runConfig.outputType = OUTPUT_FILE;
        runConfig.outputPath = filesystem::path(filePath);
      }
    }
    else if (arg == "-d" || arg == "--output_dir" || arg == "output-dir")
    {
      ensureArg(argc, argv, i);
      runConfig.outputType = OUTPUT_DIRECTORY;
      runConfig.outputPath = filesystem::path(argv[++i]);
    }
    else if (arg == "--output_raw" || arg == "--output-raw")
    {
      runConfig.outputType = OUTPUT_RAW;
    }
    else if (arg == "-s" || arg == "--speaker")
    {
      ensureArg(argc, argv, i);
      runConfig.speakerId = (piper::SpeakerId)stol(argv[++i]);
    }
    else if (arg == "--noise_scale" || arg == "--noise-scale")
    {
      ensureArg(argc, argv, i);
      runConfig.noiseScale = stof(argv[++i]);
    }
    else if (arg == "--length_scale" || arg == "--length-scale")
    {
      ensureArg(argc, argv, i);
      runConfig.lengthScale = stof(argv[++i]);
    }
    else if (arg == "--noise_w" || arg == "--noise-w")
    {
      ensureArg(argc, argv, i);
      runConfig.noiseW = stof(argv[++i]);
    }
    else if (arg == "--sentence_silence" || arg == "--sentence-silence")
    {
      ensureArg(argc, argv, i);
      runConfig.sentenceSilenceSeconds = stof(argv[++i]);
    }
    else if (arg == "--json_input" || arg == "--json-input")
    {
      runConfig.jsonInput = true;
    }
    else if (arg == "--use_cuda" || arg == "--use-cuda")
    {
      runConfig.useCuda = true;
    }
    else if (arg == "--version")
    {
      std::cout << piper::getVersion() << std::endl;
      exit(0);
    }
    else if (arg == "--debug")
    {
      // Set DEBUG logging
      spdlog::set_level(spdlog::level::debug);
    }
    else if (arg == "-q" || arg == "--quiet")
    {
      // diable logging
      spdlog::set_level(spdlog::level::off);
    }
    else if (arg == "-h" || arg == "--help")
    {
      printUsage(argv);
      exit(0);
    }
  }

  // Verify model file exists
  ifstream modelFile(runConfig.modelPath.c_str(), ios::binary);
  if (!modelFile.good())
  {
    throw runtime_error("Model file doesn't exist");
  }

  if (!modelConfigPath)
  {
    runConfig.modelConfigPath =
        filesystem::path(runConfig.modelPath.string() + ".json");
  }
  else
  {
    runConfig.modelConfigPath = modelConfigPath.value();
  }

  // Verify model config exists
  ifstream modelConfigFile(runConfig.modelConfigPath.c_str());
  if (!modelConfigFile.good())
  {
    throw runtime_error("Model config doesn't exist");
  }
}
