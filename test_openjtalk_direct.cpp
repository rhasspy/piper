#include <iostream>
#include <string>
#include <vector>
#include <spdlog/spdlog.h>
#include "src/cpp/openjtalk_phonemize.hpp"

extern "C" {
#include "src/cpp/openjtalk_api.h"
}

int main(int argc, char* argv[]) {
    const char* text = argc > 1 ? argv[1] : "今日は良い天気です";
    
    std::cout << "Testing OpenJTalk directly with text: " << text << std::endl;
    
    // Initialize OpenJTalk
    setenv("OPENJTALK_DICTIONARY_DIR", "/Users/s19447/Desktop/piper/build/naist-jdic", 1);
    
    OpenJTalk* oj = openjtalk_initialize();
    if (!oj) {
        std::cerr << "Failed to initialize OpenJTalk" << std::endl;
        return 1;
    }
    
    // Extract labels
    OJ_Label* label = openjtalk_extract_fullcontext(oj, text);
    if (!label) {
        std::cerr << "Failed to extract fullcontext" << std::endl;
        openjtalk_finalize(oj);
        return 1;
    }
    
    // Print labels
    size_t label_size = OJ_Label_get_size(label);
    std::cout << "Generated " << label_size << " labels" << std::endl;
    
    // Test phonemize_openjtalk function
    std::vector<std::vector<piper::Phoneme>> sentences;
    piper::phonemize_openjtalk(text, sentences);
    
    std::cout << "\nPhonemized into " << sentences.size() << " sentences:" << std::endl;
    for (size_t i = 0; i < sentences.size(); i++) {
        std::cout << "Sentence " << i << ": ";
        for (size_t j = 0; j < sentences[i].size(); j++) {
            char32_t ph = sentences[i][j];
            if (ph < 128) {
                std::cout << (char)ph;
            } else {
                std::cout << "[U+" << std::hex << (uint32_t)ph << std::dec << "]";
            }
            if (j < sentences[i].size() - 1) std::cout << " ";
        }
        std::cout << std::endl;
    }
    
    // Clean up
    OJ_Label_clear(label);
    openjtalk_finalize(oj);
    
    std::cout << "\nTest completed successfully!" << std::endl;
    return 0;
}