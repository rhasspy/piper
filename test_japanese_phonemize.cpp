#include <iostream>
#include <string>
#include <vector>
#include "src/cpp/openjtalk_phonemize.hpp"

int main(int argc, char* argv[]) {
    std::string text = argc > 1 ? argv[1] : "こんにちは";
    
    std::cout << "Testing Japanese phonemization for: " << text << std::endl;
    
    // Create OpenJTalk phonemizer
    piper::OpenJTalkPhonemizer phonemizer;
    
    // Initialize OpenJTalk
    if (!phonemizer.initialize()) {
        std::cerr << "Failed to initialize OpenJTalk phonemizer" << std::endl;
        return 1;
    }
    
    // Phonemize text
    std::vector<std::string> phonemes;
    if (!phonemizer.phonemize(text, phonemes)) {
        std::cerr << "Failed to phonemize text" << std::endl;
        return 1;
    }
    
    // Print results
    std::cout << "Phonemes (" << phonemes.size() << "):" << std::endl;
    for (size_t i = 0; i < phonemes.size(); i++) {
        std::cout << "  [" << i << "]: " << phonemes[i] << std::endl;
    }
    
    std::cout << "Test completed successfully!" << std::endl;
    return 0;
}