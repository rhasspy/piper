#include "openjtalk_phonemize.hpp"
#include "utf8.h"
#include "openjtalk_wrapper.h"
#include <spdlog/spdlog.h>

namespace piper {

static bool oj_initialized = false;
static OpenJTalk *oj = nullptr;
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
  
  auto it = phonemeStr.begin();
  return utf8::next(it, phonemeStr.end());
}

void phonemize_openjtalk(const std::string &text,
                         std::vector<std::vector<Phoneme>> &sentences) {
  ensure_init();
  if (!oj) {
    // Fallback: treat whole text as one sentence of codepoints
    std::vector<Phoneme> line;
    for (auto it = text.begin(); it != text.end(); ) {
      auto cp = utf8::next(it, text.end());
      line.push_back(cp);
    }
    sentences.push_back(line);
    return;
  }

  // Use OpenJTalk to extract full-context labels
  HTS_Label_Wrapper *labels = openjtalk_extract_fullcontext(oj, text.c_str());
  if (!labels) {
    spdlog::error("OpenJTalk failed; using fallback codepoints");
    std::vector<Phoneme> line;
    for (auto it = text.begin(); it != text.end(); ) {
      auto cp = utf8::next(it, text.end());
      line.push_back(cp);
    }
    sentences.push_back(line);
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
    // devoiced vowels
    if (token.size() == 1 && std::isupper(token[0]))
      token[0] = std::tolower(token[0]);

    // Handle multi-character phonemes by splitting them into individual characters
    // This matches the Python training side which uses single-character phonemes
    if (token.size() > 1) {
      spdlog::debug("  -> Multi-character phoneme '{}', splitting into individual characters", token);
      for (char c : token) {
        std::string single_char(1, c);
        Phoneme ph = mapPhonemeStr(single_char);
        spdlog::debug("    -> Added character '{}' as phoneme value: {}", c, static_cast<uint32_t>(ph));
        currentSentence.push_back(ph);
      }
    } else {
      Phoneme ph = mapPhonemeStr(token);
      spdlog::debug("  -> Mapped to phoneme value: {}", static_cast<uint32_t>(ph));
      currentSentence.push_back(ph);
    }
  }
  if (!currentSentence.empty())
    sentences.push_back(currentSentence);

  HTS_Label_clear(labels);
}

} 