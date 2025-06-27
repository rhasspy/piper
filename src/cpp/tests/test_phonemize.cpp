/**
 * Unit tests for phonemization functionality
 */

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <map>

// Mock the phonemize functions for testing
namespace piper {

// Mock PUA mappings for testing
std::map<std::string, char32_t> testMultiCharToPUA = {
    {"ch", 0xE00E},
    {"ts", 0xE00F},
    {"ky", 0xE006},
    {"sh", 0xE010}
};

// Mock phoneme mapping function
std::vector<std::string> mapPhonemes(const std::vector<std::string>& phonemes) {
    std::vector<std::string> mapped;

    for (const auto& phoneme : phonemes) {
        auto it = testMultiCharToPUA.find(phoneme);
        if (it != testMultiCharToPUA.end()) {
            // Convert to UTF-8 string
            char32_t codepoint = it->second;
            if (codepoint <= 0x7F) {
                mapped.push_back(std::string(1, static_cast<char>(codepoint)));
            } else if (codepoint <= 0x7FF) {
                mapped.push_back(std::string{
                    static_cast<char>(0xC0 | (codepoint >> 6)),
                    static_cast<char>(0x80 | (codepoint & 0x3F))
                });
            } else if (codepoint <= 0xFFFF) {
                mapped.push_back(std::string{
                    static_cast<char>(0xE0 | (codepoint >> 12)),
                    static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)),
                    static_cast<char>(0x80 | (codepoint & 0x3F))
                });
            }
        } else {
            mapped.push_back(phoneme);
        }
    }

    return mapped;
}

} // namespace piper

// Test basic phoneme mapping
TEST(PhonemizeTest, BasicPhonemeMapping) {
    std::vector<std::string> input = {"a", "ch", "i"};
    auto result = piper::mapPhonemes(input);

    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], "a");
    EXPECT_NE(result[1], "ch"); // Should be mapped to PUA
    EXPECT_EQ(result[2], "i");
}

// Test all multi-char phoneme mappings
TEST(PhonemizeTest, AllMultiCharMappings) {
    std::vector<std::string> multiCharPhonemes = {"ch", "ts", "ky", "sh"};
    auto result = piper::mapPhonemes(multiCharPhonemes);

    EXPECT_EQ(result.size(), multiCharPhonemes.size());

    // All should be mapped (not equal to original)
    for (size_t i = 0; i < multiCharPhonemes.size(); ++i) {
        EXPECT_NE(result[i], multiCharPhonemes[i]);
    }
}

// Test mixed phonemes
TEST(PhonemizeTest, MixedPhonemes) {
    std::vector<std::string> input = {"k", "o", "n", "n", "i", "ch", "i", "w", "a"};
    auto result = piper::mapPhonemes(input);

    EXPECT_EQ(result.size(), input.size());

    // Single char phonemes should remain unchanged
    EXPECT_EQ(result[0], "k");
    EXPECT_EQ(result[1], "o");

    // Multi-char should be mapped
    EXPECT_NE(result[5], "ch");
}

// Test empty input
TEST(PhonemizeTest, EmptyInput) {
    std::vector<std::string> input = {};
    auto result = piper::mapPhonemes(input);

    EXPECT_TRUE(result.empty());
}

// Test single character phonemes
TEST(PhonemizeTest, SingleCharPhonemes) {
    std::vector<std::string> input = {"a", "i", "u", "e", "o"};
    auto result = piper::mapPhonemes(input);

    EXPECT_EQ(result, input); // Should be unchanged
}

// Test Japanese specific phonemes
TEST(PhonemizeTest, JapanesePhonemes) {
    std::vector<std::string> input = {"^", "k", "o", "N", "n", "i", "ch", "i", "w", "a", "$"};
    auto result = piper::mapPhonemes(input);

    // Check markers are preserved
    EXPECT_EQ(result.front(), "^");
    EXPECT_EQ(result.back(), "$");

    // Check "ch" is mapped
    bool found_ch_mapping = false;
    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == "ch" && result[i] != "ch") {
            found_ch_mapping = true;
            break;
        }
    }
    EXPECT_TRUE(found_ch_mapping);
}

// Test UTF-8 encoding of PUA characters
TEST(PhonemizeTest, PUAEncodingTest) {
    // Test that PUA characters are properly encoded
    std::vector<std::string> input = {"ch"};
    auto result = piper::mapPhonemes(input);

    ASSERT_EQ(result.size(), 1);

    // Check it's a 3-byte UTF-8 sequence (PUA is in range E000-F8FF)
    const std::string& pua_char = result[0];
    EXPECT_EQ(pua_char.length(), 3); // PUA chars are 3 bytes in UTF-8

    // Check first byte starts with 1110 (0xE0)
    EXPECT_EQ((unsigned char)pua_char[0] & 0xF0, 0xE0);
}

// Main function for test runner
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}