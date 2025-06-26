#include "openjtalk_phonemize.hpp"
#include "utf8.h"
#include "openjtalk_wrapper.h"
#include <spdlog/spdlog.h>
#include <unordered_map>
#include <sstream>

namespace piper {

static bool oj_initialized = false;
static OpenJTalk *oj = nullptr;

// Multi-character phoneme to PUA character mapping
// This must match the Python side token_mapper.py implementation
static std::unordered_map<std::string, char32_t> multiCharToPUA = {
    // Long vowels
    {"a:", 0xE000},
    {"i:", 0xE001},
    {"u:", 0xE002},
    {"e:", 0xE003},
    {"o:", 0xE004},
    // Special consonants
    {"cl", 0xE005},
    // Palatalized consonants
    {"ky", 0xE006},
    {"kw", 0xE007},
    {"gy", 0xE008},
    {"gw", 0xE009},
    {"ty", 0xE00A},
    {"dy", 0xE00B},
    {"py", 0xE00C},
    {"by", 0xE00D},
    // Affricates and special sounds
    {"ch", 0xE00E},
    {"ts", 0xE00F},
    {"sh", 0xE010},
    {"zy", 0xE011},
    {"hy", 0xE012},
    // Palatalized nasals/liquids
    {"ny", 0xE013},
    {"my", 0xE014},
    {"ry", 0xE015}
};

// Reverse mapping for display purposes
static std::unordered_map<char32_t, std::string> puaToMultiChar;

// Initialize reverse mapping
static void initReverseMapping() {
    static bool initialized = false;
    if (!initialized) {
        for (const auto& pair : multiCharToPUA) {
            puaToMultiChar[pair.second] = pair.first;
        }
        initialized = true;
    }
}

// Convert phoneme for display (PUA to readable form)
static std::string phonemeToDisplayString(Phoneme ph) {
    initReverseMapping();
    
    // Check if it's a PUA character
    if (ph >= 0xE000 && ph <= 0xF8FF) {
        auto it = puaToMultiChar.find(ph);
        if (it != puaToMultiChar.end()) {
            return it->second;
        }
    }
    
    // Convert regular character to string
    std::string result;
    utf8::append(ph, std::back_inserter(result));
    return result;
}

static void ensure_init() {
  if (oj_initialized)
    return;
  oj_initialized = true;
  oj = openjtalk_initialize();
  if (!oj) {
    spdlog::error("Failed to initialize OpenJTalk; falling back to codepoints");
  }
}

// Convert string to phoneme (char32_t)
Phoneme mapPhonemeStr(const std::string &phonemeStr) {
  if (phonemeStr.empty()) return 0;
  
  // Check if this is a multi-character phoneme that should map to PUA
  auto it = multiCharToPUA.find(phonemeStr);
  if (it != multiCharToPUA.end()) {
    return it->second;
  }
  
  // Otherwise, return the Unicode codepoint of the first character
  auto str_it = phonemeStr.begin();
  return utf8::next(str_it, phonemeStr.end());
}

void phonemize_openjtalk(const std::string &text,
                         std::vector<std::vector<Phoneme>> &sentences) {
  ensure_init();
  if (!oj) {
    // OpenJTalk not available - this should only happen for English text
    // Don't try to apply PUA mapping on regular text
    // Just return empty to indicate phonemization failed
    return;
  }

  // Use OpenJTalk to extract full-context labels
  HTS_Label_Wrapper *labels = openjtalk_extract_fullcontext(oj, text.c_str());
  if (!labels) {
    spdlog::error("OpenJTalk failed to extract phonemes");
    // Return empty sentences to indicate failure
    // This will prevent the crash from trying to map Japanese characters
    return;
  }

  std::vector<Phoneme> currentSentence;
  size_t num = HTS_Label_get_size(labels);
  for (size_t i = 0; i < num; ++i) {
    const char *label = HTS_Label_get_string(labels, i);
    std::string lab(label);
    // simple parse: find between '-' and '+'
    auto pos1 = lab.find('-');
    auto pos2 = lab.find('+');
    if (pos1 == std::string::npos || pos2 == std::string::npos || pos2 <= pos1)
      continue;
    std::string token = lab.substr(pos1 + 1, pos2 - pos1 - 1);
    if (token == "sil" && i == 0) {
      currentSentence.push_back(mapPhonemeStr("^"));
      continue;
    }
    if (token == "sil") {
      currentSentence.push_back(mapPhonemeStr("$"));
      sentences.push_back(currentSentence);
      currentSentence.clear();
      continue;
    }
    if (token == "pau") {
      currentSentence.push_back(mapPhonemeStr("_"));
      continue;
    }
    // devoiced vowels (but NOT 'N' which is a special phoneme)
    if (token.size() == 1 && std::isupper(token[0]) && token[0] != 'N') {
      char lower = std::tolower(token[0]);
      spdlog::debug("  -> Devoiced vowel '{}' converted to '{}'", token[0], lower);
      token[0] = lower;
    }

    // Map the phoneme token to its corresponding character
    // Multi-character phonemes are mapped to PUA characters by mapPhonemeStr
    Phoneme ph = mapPhonemeStr(token);
    spdlog::debug("  -> Mapped to phoneme value: {} (U+{:04X})", static_cast<uint32_t>(ph), static_cast<uint32_t>(ph));
    currentSentence.push_back(ph);
  }
  if (!currentSentence.empty())
    sentences.push_back(currentSentence);
  
  spdlog::debug("Total sentences: {}", sentences.size());
  for (size_t i = 0; i < sentences.size(); ++i) {
    spdlog::debug("Sentence {}: {} phonemes", i, sentences[i].size());
    
    // Log phoneme sequence in readable form
    if (spdlog::get_level() <= spdlog::level::debug) {
      std::stringstream ss;
      for (const auto& ph : sentences[i]) {
        ss << phonemeToDisplayString(ph) << " ";
      }
      spdlog::debug("Phoneme sequence: {}", ss.str());
    }
  }

  HTS_Label_clear(labels);
}

} 