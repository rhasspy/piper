#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <piper-phonemize/phonemize.hpp>
#include "src/cpp/piper.hpp"
#include "src/cpp/openjtalk_phonemize.hpp"

int main(int argc, char* argv[]) {
    std::string text = argc > 1 ? argv[1] : "こんにちは";
    
    std::cout << "Testing Japanese phonemization for: " << text << std::endl;
    
    // Phonemize text
    std::vector<std::vector<piper::Phoneme>> sentences;
    piper::phonemize_openjtalk(text, sentences);
    
    // Print results
    std::cout << "Number of sentences: " << sentences.size() << std::endl;
    for (size_t i = 0; i < sentences.size(); i++) {
        std::cout << "Sentence " << i << " (" << sentences[i].size() << " phonemes):" << std::endl;
        for (size_t j = 0; j < sentences[i].size(); j++) {
            // Convert char32_t to UTF-8 for display
            char32_t phoneme = sentences[i][j];
            if (phoneme < 128) {
                std::cout << "  [" << j << "]: " << (char)phoneme << std::endl;
            } else {
                std::cout << "  [" << j << "]: U+" << std::hex << (uint32_t)phoneme << std::dec << std::endl;
            }
        }
    }
    
    std::cout << "Test completed successfully!" << std::endl;
    return 0;
}