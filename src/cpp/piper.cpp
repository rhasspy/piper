#include <array>
#include <chrono>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <codecvt>

#include <onnxruntime_cxx_api.h>
#include <spdlog/spdlog.h>

#include "json.hpp"
#include "piper.hpp"
#include "utf8.h"
#include "wavfile.hpp"

namespace piper
{

  std::ofstream logger("ttslog.txt");

  Voice voice;

  SynthesisConfig synthesisConfig;

  static std::map<Phoneme, PhonemeId>
      DEFAULT_PHONEME_ID_MAP = {
          {U'_', 0},
          {U'^', 1},
          {U'$', 2},
          {U' ', 3},
          {U'!', 4},
          {U'\'', 5},
          {U'(', 6},
          {U')', 7},
          {U',', 8},
          {U'-', 9},
          {U'.', 10},
          {U':', 11},
          {U';', 12},
          {U'?', 13},
          {U'a', 14},
          {U'b', 15},
          {U'c', 16},
          {U'd', 17},
          {U'e', 18},
          {U'f', 19},
          {U'h', 20},
          {U'i', 21},
          {U'j', 22},
          {U'k', 23},
          {U'l', 24},
          {U'm', 25},
          {U'n', 26},
          {U'o', 27},
          {U'p', 28},
          {U'q', 29},
          {U'r', 30},
          {U's', 31},
          {U't', 32},
          {U'u', 33},
          {U'v', 34},
          {U'w', 35},
          {U'x', 36},
          {U'y', 37},
          {U'z', 38},
          {U'æ', 39},
          {U'ç', 40},
          {U'ð', 41},
          {U'ø', 42},
          {U'ħ', 43},
          {U'ŋ', 44},
          {U'œ', 45},
          {U'ǀ', 46},
          {U'ǁ', 47},
          {U'ǂ', 48},
          {U'ǃ', 49},
          {U'ɐ', 50},
          {U'ɑ', 51},
          {U'ɒ', 52},
          {U'ɓ', 53},
          {U'ɔ', 54},
          {U'ɕ', 55},
          {U'ɖ', 56},
          {U'ɗ', 57},
          {U'ɘ', 58},
          {U'ə', 59},
          {U'ɚ', 60},
          {U'ɛ', 61},
          {U'ɜ', 62},
          {U'ɞ', 63},
          {U'ɟ', 64},
          {U'ɠ', 65},
          {U'ɡ', 66},
          {U'ɢ', 67},
          {U'ɣ', 68},
          {U'ɤ', 69},
          {U'ɥ', 70},
          {U'ɦ', 71},
          {U'ɧ', 72},
          {U'ɨ', 73},
          {U'ɪ', 74},
          {U'ɫ', 75},
          {U'ɬ', 76},
          {U'ɭ', 77},
          {U'ɮ', 78},
          {U'ɯ', 79},
          {U'ɰ', 80},
          {U'ɱ', 81},
          {U'ɲ', 82},
          {U'ɳ', 83},
          {U'ɴ', 84},
          {U'ɵ', 85},
          {U'ɶ', 86},
          {U'ɸ', 87},
          {U'ɹ', 88},
          {U'ɺ', 89},
          {U'ɻ', 90},
          {U'ɽ', 91},
          {U'ɾ', 92},
          {U'ʀ', 93},
          {U'ʁ', 94},
          {U'ʂ', 95},
          {U'ʃ', 96},
          {U'ʄ', 97},
          {U'ʈ', 98},
          {U'ʉ', 99},
          {U'ʊ', 100},
          {U'ʋ', 101},
          {U'ʌ', 102},
          {U'ʍ', 103},
          {U'ʎ', 104},
          {U'ʏ', 105},
          {U'ʐ', 106},
          {U'ʑ', 107},
          {U'ʒ', 108},
          {U'ʔ', 109},
          {U'ʕ', 110},
          {U'ʘ', 111},
          {U'ʙ', 112},
          {U'ʛ', 113},
          {U'ʜ', 114},
          {U'ʝ', 115},
          {U'ʟ', 116},
          {U'ʡ', 117},
          {U'ʢ', 118},
          {U'ʲ', 119},
          {U'ˈ', 120},
          {U'ˌ', 121},
          {U'ː', 122},
          {U'ˑ', 123},
          {U'˞', 124},
          {U'β', 125},
          {U'θ', 126},
          {U'χ', 127},
          {U'ᵻ', 128},
          {U'ⱱ', 129},
          {U'0', 130},
          {U'1', 131},
          {U'2', 132},
          {U'3', 133},
          {U'4', 134},
          {U'5', 135},
          {U'6', 136},
          {U'7', 137},
          {U'8', 138},
          {U'9', 139},
          {U'\u0327', 140},
          {U'\u0303', 141},
          {U'\u032a', 142},
          {U'\u032f', 143},
          {U'\u0329', 144},
          {U'ʰ', 145},
          {U'ˤ', 146},
          {U'ε', 147},
          {U'↓', 148},
          {U'#', 149},
          {U'\"', 150},
          {U'↑', 151},
          {U'\u033a', 152},
          {U'\u033b', 153},
          {U'g', 154},
          {U'ʦ', 155},
          {U'X', 156},
          {U'\u031d', 157},
          {U'\u030a', 158}};

  static std::vector<std::string> numbers_units = {"zero", "one", "two", "three",
                                                   "four", "five", "six", "seven", "eight", "nine", "ten", "eleven",
                                                   "twelve", "thirteen", "fourteen", "fifteen", "sixteen",
                                                   "seventeen", "eighteen", "nineteen"};

  static std::vector<std::string> numbers_tens = {"", "", "twenty", "thirty", "forty",
                                                  "fifty", "sixty", "seventy", "eighty", "ninety"};

  std::string convert_decimal_places_to_text(std::string i)
  {
    std::string ss;
    for (auto c : i)
    {
      // numbers are consecutive on the encoding page, so substracting the char 0 will result in what number it is
      ss += numbers_units[c - '0'] + " ";
    }
    return ss;
  }

  std::string convert_number_to_text(int64_t i)
  {
    if (i < 20)
    {
      return numbers_units[i];
    }

    if (i < 100)
    {
      return numbers_tens[i / 10] + ((i % 10 > 0) ? " " + convert_number_to_text(i % 10) : "");
    }

    if (i < 1'000)
    {
      return numbers_units[i / 100] + " hundred" + ((i % 100 > 0) ? " " + convert_number_to_text(i % 100) : "");
    }

    if (i < 1'000'000)
    {
      return convert_number_to_text(i / 1'000) + " thousand" + ((i % 1'000 > 0) ? " " + convert_number_to_text(i % 1'000) : "");
    }

    if (i < 1'000'000'000)
    {
      return convert_number_to_text(i / 1'000'000) + " million" + ((i % 1'000'000 > 0) ? " " + convert_number_to_text(i % 1'000'000) : "");
    }

    if (i < 1'000'000'000'000)
    {
      return convert_number_to_text(i / 1'000'000'000) + " billion" + ((i % 1'000'000'000 > 0) ? " " + convert_number_to_text(i % 1'000'000'000) : "");
    }

    return convert_number_to_text(i / 1'000'000'000'000) + " trillion" + ((i % 1'000'000'000'000 > 0) ? " " + convert_number_to_text(i % 1'000'000'000'000) : "");
  }

  std::string number_to_text(double number)
  {
    int64_t number_int = (int64_t)number;
    std::string number_dec = std::to_string(number - (double)(number_int)).substr(2);
    number_dec.erase(number_dec.find_last_not_of('0') + 1, std::string::npos);
    number_dec.erase(number_dec.find_last_not_of('.') + 1, std::string::npos);

    if (number_dec == "0" || number_dec == "")
    {
      return convert_number_to_text(number_int);
    }
    else
    {
      return convert_number_to_text(number_int) + " point " + convert_decimal_places_to_text(number_dec);
    }
  }

  // Maximum value for 16-bit signed WAV sample
  const float MAX_WAV_VALUE = 32767.0f;

  static std::map<std::string, std::string> IPA_MAP;

  inline void ltrim(std::string &s)
  {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch)
                                    { return !std::isspace(ch); }));
  }

  inline void rtrim(std::string &s)
  {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch)
                         { return !std::isspace(ch); })
                .base(),
            s.end());
  }

  inline std::string trim(std::string &s)
  {
    rtrim(s);
    ltrim(s);
    return s;
  }

  void ApplySynthesisConfig(float lengthScale, float noiseScale, float noiseW, int speakerId, int sampleRate, float sentenceSilenceSeconds, bool useCuda)
  {
    synthesisConfig.modelPath = lengthScale;
    synthesisConfig.lengthScale = lengthScale;
    synthesisConfig.noiseScale = noiseScale;
    synthesisConfig.noiseW = noiseW;
    synthesisConfig.speakerId = speakerId;
    synthesisConfig.sampleRate = sampleRate;
    synthesisConfig.sentenceSilenceSeconds = sentenceSilenceSeconds;
    synthesisConfig.useCuda = useCuda;
    voice.synthesisConfig = synthesisConfig;
  }

  void LoadIPAData(std::string ipaDataPath)
  {
    std::string line;
    std::ifstream file(ipaDataPath);
    int kvCount = 0;
    if (file.is_open())
    {
      while (getline(file, line))
      {
        auto first = line.find(",");
        auto second = line.find(",", first + 1);
        auto textrep = line.substr(0, first);
        auto key = trim(textrep);
        std::string value;
        if (second == std::string::npos)
        {
          auto iparep = line.substr(first + 1);
          value = trim(iparep);
        }
        else
        {
          auto iparep = line.substr(first + 1, second - first - 1);
          value = trim(iparep);
        }
        IPA_MAP[key] = value;
        kvCount++;
      }
      file.close();
    }
    else
    {
      throw new std::invalid_argument("Could not load ipa data file from '" + std::string(ipaDataPath) + "'. This file should contain only lines in the following format: WORD, IPA");
    }
  }

  void LoadModel(std::string modelPath, ModelSession &session, bool useCuda)
  {
    std::ofstream waller("txt.txt");

    waller << "INIT" << std::endl;

    session.env = Ort::Env(OrtLoggingLevel::ORT_LOGGING_LEVEL_WARNING, std::string("TTS").c_str());
    session.env.DisableTelemetryEvents();

    waller << "ENV" << std::endl;

    if (useCuda)
    {
      // Use CUDA provider
      OrtCUDAProviderOptions cuda_options{};
      cuda_options.cudnn_conv_algo_search = OrtCudnnConvAlgoSearchHeuristic;
      session.options.AppendExecutionProvider_CUDA(cuda_options);
    }

    waller << "CUDA" << std::endl;

    // Slows down performance by ~2x
    // session.options.SetIntraOpNumThreads(1);

    // Roughly doubles load time for no visible inference benefit
    // session.options.SetGraphOptimizationLevel(
    //     GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

    session.options.SetGraphOptimizationLevel(
        GraphOptimizationLevel::ORT_DISABLE_ALL);

    waller << "GRAPH" << std::endl;

    // Slows down performance very slightly
    // session.options.SetExecutionMode(ExecutionMode::ORT_PARALLEL);

    session.options.DisableCpuMemArena();
    session.options.DisableMemPattern();
    session.options.DisableProfiling();

    waller << "OPTIONS" << std::endl;

    auto startTime = std::chrono::steady_clock::now();

#ifdef _WIN32
    auto modelPathW = std::wstring(modelPath.begin(), modelPath.end());
    auto modelPathStr = modelPathW.c_str();
#else
    auto modelPathStr = modelPath.c_str();
#endif

    waller << "PATH" << modelPathStr << " - " << modelPath << std::endl;

    session.onnx = Ort::Session(session.env, modelPathStr, session.options);

    waller << "SESSION" << std::endl;

    auto endTime = std::chrono::steady_clock::now();
    spdlog::debug("Loaded onnx model in {} second(s)",
                  std::chrono::duration<double>(endTime - startTime).count());
  }

  // Load Onnx model and JSON config file
  void LoadVoice(std::string modelPath)
  {

    std::ofstream waller("huh.txt");
    waller << modelPath;
    LoadModel(modelPath, voice.session, synthesisConfig.useCuda);
  }

  // Phoneme ids to WAV audio
  void synthesize(std::vector<PhonemeId> &phonemeIds,
                  SynthesisConfig &synthesisConfig, ModelSession &session,
                  std::vector<int16_t> &audioBuffer)
  {
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
    std::vector<int64_t> speakerId{synthesisConfig.speakerId};
    std::vector<int64_t> speakerIdShape{(int64_t)speakerId.size()};

    if (synthesisConfig.speakerId)
    {
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

    if ((outputTensors.size() != 1) || (!outputTensors.front().IsTensor()))
    {
      throw std::runtime_error("Invalid output tensors");
    }

    const float *audio = outputTensors.front().GetTensorData<float>();
    auto audioShape =
        outputTensors.front().GetTensorTypeAndShapeInfo().GetShape();
    int64_t audioCount = audioShape[audioShape.size() - 1];

    // Get max audio value for scaling
    float maxAudioValue = 0.01f;
    for (int64_t i = 0; i < audioCount; i++)
    {
      float audioValue = abs(audio[i]);
      if (audioValue > maxAudioValue)
      {
        maxAudioValue = audioValue;
      }
    }

    // We know the size up front
    audioBuffer.reserve(audioCount);

    // Scale audio to fill range and convert to int16
    float audioScale = (MAX_WAV_VALUE / std::max(0.01f, maxAudioValue));
    for (int64_t i = 0; i < audioCount; i++)
    {
      int16_t intAudioValue = static_cast<int16_t>(
          std::clamp(audio[i] * audioScale,
                     static_cast<float>(std::numeric_limits<int16_t>::min()),
                     static_cast<float>(std::numeric_limits<int16_t>::max())));

      audioBuffer.push_back(intAudioValue);
    }

    // Clean up
    for (std::size_t i = 0; i < outputTensors.size(); i++)
    {
      Ort::detail::OrtRelease(outputTensors[i].release());
    }

    for (std::size_t i = 0; i < inputTensors.size(); i++)
    {
      Ort::detail::OrtRelease(inputTensors[i].release());
    }
  }

  using Phoneme = char32_t;

  bool could_be_number(std::string text, int index)
  {
    if (index >= text.length() || index <= 0)
    {
      return false;
    }

    return isdigit(text[index - 1]) && text[index] == '.' && isdigit(text[index + 1]);
  }

  bool is_sentence_ending(char c)
  {
    return (c == '.' || c == '!' || c == '?' || c == ';' || c == ':' | c == ',');
  }

  std::vector<std::string> split_into_sentences(const std::string &text)
  {
    std::vector<std::string> sentences;
    size_t start = 0;
    size_t end = 0;

    while (end < text.length())
    {
      int decimal_dot = -1;

      // Find the end of the current sentence
      while (end < text.length() && (decimal_dot == -1 && could_be_number(text, end) || !is_sentence_ending(text[end])))
      {
        if (text[end] == '.')
        {
          decimal_dot = end;
        }
        end++;
      }

      // Extract the sentence
      std::string sentence = text.substr(start, end - start);

      // Exclude the sentence-ending punctuation
      end++;

      // Trim any leading/trailing whitespace
      size_t first_non_space = sentence.find_first_not_of(" \t\n\r");
      size_t last_non_space = sentence.find_last_not_of(" \t\n\r");

      if (first_non_space != std::string::npos && last_non_space != std::string::npos)
      {
        sentence = sentence.substr(first_non_space, last_non_space - first_non_space + 1);
      }
      else
      {
        sentence = ""; // Sentence is empty or only spaces
      }

      // Add to the list of sentences if not empty
      if (!sentence.empty())
      {
        sentences.push_back(sentence);
      }

      // Move to the start of the next sentence
      start = end;
    }

    return sentences;
  }

  std::vector<std::string> split(const std::string &txt, char ch)
  {
    std::vector<std::string> strs;
    size_t pos = txt.find(ch);
    size_t initialPos = 0;
    strs.clear();

    while (pos != std::string::npos)
    {
      strs.push_back(txt.substr(initialPos, pos - initialPos));
      initialPos = pos + 1;
      pos = txt.find(ch, initialPos);
    }

    // Add the last one
    strs.push_back(txt.substr(initialPos, std::min(pos, txt.size()) - initialPos + 1));
    return strs;
  }

  std::u32string utf8_to_utf32(const std::string &utf8str)
  {
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> convert;
    return convert.from_bytes(utf8str);
  }

  std::string acceptable_chars = "\'-./+=&%$§*#";

  std::string remove_all_unwanted_chars(std::string &str)
  {
    auto strlength = str.length();
    std::stringstream ss;
    for (auto i = 0; i < strlength; ++i)
    {
      auto c = str[i];

      if (isalnum(c) || acceptable_chars.find(c) != std::string::npos)
      {
        ss << c;
      }
    }

    return ss.str();
  }

  void remove_chars_from_string(std::string &str, char *charsToRemove)
  {
    for (unsigned int i = 0; i < strlen(charsToRemove); ++i)
    {
      str.erase(remove(str.begin(), str.end(), charsToRemove[i]), str.end());
    }
  }

  void phonemize_number(std::string rep, std::vector<Phoneme> &sentence_phonemes)
  {
    try
    {
      rep = number_to_text(std::stod(rep));

      auto number_text = split(rep, ' ');
      for (auto number : number_text)
      {
        rep = (IPA_MAP.count(number) == 1 ? IPA_MAP[number] : number);

        auto convertedString = utf8_to_utf32(rep);
        auto length = convertedString.length();
        auto char32array = convertedString.c_str();

        for (size_t i = 0; i < length; i++)
        {
          sentence_phonemes.push_back(char32array[i]);
        }
        sentence_phonemes.push_back(U' ');
      }
    }
    catch (const std::invalid_argument &)
    {
      logger << "Could not convert number, because argument is invalid: " << rep << std::endl;
    }
    catch (const std::out_of_range &)
    {
      logger << "Could not convert number, because argument is out of range for a double" << rep << std::endl;
    }
  }

  void phonemize_text(std::string &str, std::vector<std::vector<Phoneme>> &phonemes)
  {
    auto sentences = split_into_sentences(str);
    for (auto sentence : sentences)
    {
      auto words = split(sentence, ' ');

      std::vector<Phoneme> sentence_phonemes;

      for (auto word : words)
      {

        auto wordcopy = word.substr(0);
        auto word_copy_reduced = remove_all_unwanted_chars(wordcopy);
        auto rep = trim(word_copy_reduced);

        if (rep.empty())
        {
          continue;
        }

        bool is_all_uppercase = std::all_of(rep.begin(), rep.end(), [](unsigned char c)
                                            { return std::isupper(c); });

        std::transform(rep.begin(), rep.end(), rep.begin(),
                       [](unsigned char c)
                       { return std::tolower(c); });

        bool is_a_number = true;
        bool is_multipoint_number = false; // TODO
        bool is_estimation = false;

        for (auto c : rep)
        {
          if (c == '-' && !is_estimation)
          {
            is_estimation = true;
            continue;
          }

          if (!isdigit(c) && c != '.' && c != ',')
          {
            is_a_number = false;
            break;
          }
        }

        if (is_a_number)
        {
          remove_chars_from_string(rep, ",");
        }

        std::cout << "WRD " << word << std::endl;
        std::cout << " TYP " << (int)is_all_uppercase << (int)is_a_number << (int)is_estimation << std::endl;

        if (is_estimation)
        {
          auto numbers = split(rep, '-');
          phonemize_number(numbers[0], sentence_phonemes);
          auto to_phones = IPA_MAP.count("to") == 1 ? IPA_MAP["to"] : "to";
          auto convertedString = utf8_to_utf32(to_phones);
          auto length = convertedString.length();
          auto char32array = convertedString.c_str();
          for (size_t i = 0; i < length; i++)
          {
            sentence_phonemes.push_back(char32array[i]);
          }
          sentence_phonemes.push_back(U' ');
          phonemize_number(numbers[1], sentence_phonemes);
          continue;
        }
        else if (is_a_number)
        {
          phonemize_number(rep, sentence_phonemes);
          continue;
        }

        if (is_all_uppercase)
        {
          std::stringstream repcopy;
          for (auto c : rep)
          {
            auto str = std::string(1, c);
            auto str_uppercase = std::string(1, toupper(c));
            repcopy << (IPA_MAP.count(str_uppercase) == 1 ? IPA_MAP[str_uppercase] : (IPA_MAP.count(str) == 1 ? IPA_MAP[str] : str));
            repcopy << ' ';
          }
          rep = repcopy.str();
        }
        else if (IPA_MAP.count(rep) == 1)
        {
          rep = IPA_MAP[rep];
        }
        else
        {
          std::cout << "[" << rep << "] = ";

          int pos = 0;
          int length = rep.length();
          int remaining = rep.length();

          std::stringstream repcopy;

          while (remaining > 0)
          {
            auto testword = rep.substr(pos, length);

            if (length == 0)
            {
              length = 1;
              pos = length + pos;
              remaining = remaining - length;
              length = remaining;
              continue;
            }

            if (IPA_MAP.count(testword) == 1)
            {
              repcopy << IPA_MAP[testword];
              pos = length + pos;
              remaining = remaining - length;
              length = remaining;
            }
            else
            {
              length--;
            }
          }

          rep = repcopy.str();
        }

        if (rep.length() == 0)
        {
          continue;
        }

        std::cout << "IPA " << rep << std::endl;

        auto convertedString = utf8_to_utf32(rep);
        auto length = convertedString.length();
        auto char32array = convertedString.c_str();

        for (size_t i = 0; i < length; i++)
        {
          sentence_phonemes.push_back(char32array[i]);
        }
        sentence_phonemes.push_back(U' ');
      }

      if (sentence_phonemes.size() > 0)
      {
        phonemes.push_back(sentence_phonemes);
      }
    }
  }

  // ----------------------------------------------------------------------------

  // Phonemize text and synthesize audio

  /*void phonemes_to_ids(std::vector<piper::Phoneme> &sentence, std::vector<PhonemeId> &phonemeIds, std::vector<piper::Phoneme> &missing)
  {
    for (auto phon : sentence)
    {
      if (DEFAULT_PHONEME_ID_MAP.count(phon) == 1)
      {
        phonemeIds.push_back(DEFAULT_PHONEME_ID_MAP[phon]);
        std::cout << "PUSHING" << std::endl;
      }
      else
      {
        missing.push_back(phon);
        std::cout << "MISSING" << std::endl;
      }
    }
  }*/

  void phonemes_to_ids(std::vector<Phoneme> &phonemes, std::vector<PhonemeId> &phonemeIds,
                       std::vector<Phoneme> &missingPhonemes)
  {
    Phoneme pad = U'_';
    Phoneme bos = U'^';
    Phoneme eos = U'$';

    auto bosId = DEFAULT_PHONEME_ID_MAP.at(bos);
    auto padId = DEFAULT_PHONEME_ID_MAP.at(pad);
    auto eosId = DEFAULT_PHONEME_ID_MAP.at(eos);

    // Beginning of sentence symbol (^)
    phonemeIds.push_back(bosId);

    // Pad after bos (_)
    phonemeIds.push_back(padId);

    for (auto const phoneme : phonemes)
    {
      if (DEFAULT_PHONEME_ID_MAP.count(phoneme) < 1)
      {
        missingPhonemes.push_back(phoneme);
        continue;
      }

      auto mappedId = DEFAULT_PHONEME_ID_MAP.at(phoneme);
      phonemeIds.push_back(mappedId);

      // pad (_)
      phonemeIds.push_back(padId);
    }

    // End of sentence symbol ($)
    phonemeIds.push_back(eosId);
  }

  void TextToAudio(std::string text,
                   std::vector<int16_t> &audioBuffer,
                   const std::function<void()> &audioCallback)
  {
    // Phonemes for each sentence
    spdlog::debug("Phonemizing text: {}", text);
    std::vector<std::vector<Phoneme>> phonemes;

    phonemize_text(text, phonemes);

    // Synthesize each sentence independently.
    std::vector<PhonemeId> phonemeIds;
    std::vector<Phoneme> missingPhonemes;
    for (auto sentence : phonemes)
    {

      // phonemes -> ids
      phonemes_to_ids(sentence, phonemeIds, missingPhonemes);

      // ids -> audio
      synthesize(phonemeIds, voice.synthesisConfig, voice.session, audioBuffer);

      auto sentenceSilenceSamples = (std::size_t)(
          voice.synthesisConfig.sentenceSilenceSeconds *
          voice.synthesisConfig.sampleRate * voice.synthesisConfig.channels);

      for (std::size_t i = 0; i < sentenceSilenceSamples; i++)
      {
        audioBuffer.push_back(0);
      }

      if (audioCallback)
      {
        // Call back must copy audio since it is cleared afterwards.
        audioCallback();
        audioBuffer.clear();
      }

      phonemeIds.clear();
    }

    std::cout << std::endl;

    if (missingPhonemes.size() > 0)
    {
      std::stringstream ss;

      for (auto phon : missingPhonemes)
      {
        ss << (char)phon;
      }

      spdlog::warn("Missing {} phoneme(s) from phoneme/id map! {}",
                   missingPhonemes.size(), ss.str());
    }

  } /* textToAudio */

  // Phonemize text and synthesize audio to WAV file
  char *TextToVoice(std::string text, uint32_t &dataSize)
  {
    std::vector<int16_t> audioBuffer;

    TextToAudio(text, audioBuffer, NULL);

    auto audioHeaderData = getWavHeader(synthesisConfig.sampleRate, synthesisConfig.sampleWidth, synthesisConfig.channels, (int32_t)audioBuffer.size());
    auto audioHeaderSize = sizeof(audioHeaderData);
    auto audioBufferData = (const char *)audioBuffer.data();
    auto audioBufferSize = audioBuffer.size() * 2;
    dataSize = audioBufferSize + audioHeaderSize;

    char *full_data;
    full_data = (char *)malloc(dataSize);                                                 /* make space for the new string (should check the return value ...) */
    memcpy(full_data, reinterpret_cast<const char *>(&audioHeaderData), audioHeaderSize); /* copy name into the new var */
    memcpy(full_data + audioHeaderSize, audioBufferData, audioBufferSize);                /* add the extension */

    return full_data;
  } /* textToWavFile */

} // namespace piper
