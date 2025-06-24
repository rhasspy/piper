#ifndef OPENJTALK_PHONEMIZE_HPP_
#define OPENJTALK_PHONEMIZE_HPP_

#include <string>
#include <vector>
#include "piper.hpp"

namespace piper {
// Rough wrapper around libopenjtalk. Returns phonemes per sentence as list.
void phonemize_openjtalk(const std::string &text,
                         std::vector<std::vector<Phoneme>> &sentences);
}

#endif 