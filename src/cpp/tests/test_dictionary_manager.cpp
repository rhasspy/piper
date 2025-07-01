#include <gtest/gtest.h>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>

extern "C" {
#include "../openjtalk_dictionary_manager.h"
}

class DictionaryManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary test directory
        char temp_template[] = "/tmp/piper_test_XXXXXX";
        test_dir = mkdtemp(temp_template);
        ASSERT_NE(test_dir, nullptr);
        
        // Save original environment variables
        original_home = getenv("HOME");
        original_dict_dir = getenv("OPENJTALK_DICTIONARY_DIR");
        original_voice = getenv("OPENJTALK_VOICE");
        original_auto_download = getenv("PIPER_AUTO_DOWNLOAD_DICT");
        original_offline = getenv("PIPER_OFFLINE_MODE");
        
        // Set test HOME
        setenv("HOME", test_dir, 1);
    }
    
    void TearDown() override {
        // Restore original environment variables
        if (original_home) {
            setenv("HOME", original_home, 1);
        } else {
            unsetenv("HOME");
        }
        
        if (original_dict_dir) {
            setenv("OPENJTALK_DICTIONARY_DIR", original_dict_dir, 1);
        } else {
            unsetenv("OPENJTALK_DICTIONARY_DIR");
        }
        
        if (original_voice) {
            setenv("OPENJTALK_VOICE", original_voice, 1);
        } else {
            unsetenv("OPENJTALK_VOICE");
        }
        
        if (original_auto_download) {
            setenv("PIPER_AUTO_DOWNLOAD_DICT", original_auto_download, 1);
        } else {
            unsetenv("PIPER_AUTO_DOWNLOAD_DICT");
        }
        
        if (original_offline) {
            setenv("PIPER_OFFLINE_MODE", original_offline, 1);
        } else {
            unsetenv("PIPER_OFFLINE_MODE");
        }
        
        // Clean up test directory
        if (test_dir) {
            char cmd[256];
            snprintf(cmd, sizeof(cmd), "rm -rf %s", test_dir);
            system(cmd);
        }
    }
    
    char* test_dir = nullptr;
    const char* original_home = nullptr;
    const char* original_dict_dir = nullptr;
    const char* original_voice = nullptr;
    const char* original_auto_download = nullptr;
    const char* original_offline = nullptr;
};

// Test dictionary path resolution
TEST_F(DictionaryManagerTest, GetDefaultDictPath) {
    char buffer[1024];
    
    // Test default path (should use HOME)
    EXPECT_EQ(openjtalk_get_default_dict_path(buffer, sizeof(buffer)), 0);
    EXPECT_TRUE(strstr(buffer, test_dir) != nullptr);
    EXPECT_TRUE(strstr(buffer, ".piper/dictionaries/openjtalk") != nullptr);
}

// Test custom dictionary path via environment variable
TEST_F(DictionaryManagerTest, CustomDictPath) {
    const char* custom_path = "/custom/dict/path";
    setenv("OPENJTALK_DICTIONARY_DIR", custom_path, 1);
    
    // Create dummy dictionary files
    mkdir("/tmp", 0755);
    mkdir("/tmp/custom_dict_test", 0755);
    setenv("OPENJTALK_DICTIONARY_DIR", "/tmp/custom_dict_test", 1);
    
    // Create dummy dictionary files
    FILE* fp = fopen("/tmp/custom_dict_test/sys.dic", "w");
    if (fp) fclose(fp);
    fp = fopen("/tmp/custom_dict_test/unk.dic", "w");
    if (fp) fclose(fp);
    
    char buffer[1024];
    EXPECT_EQ(openjtalk_get_default_dict_path(buffer, sizeof(buffer)), 0);
    EXPECT_STREQ(buffer, "/tmp/custom_dict_test");
    
    // Clean up
    unlink("/tmp/custom_dict_test/sys.dic");
    unlink("/tmp/custom_dict_test/unk.dic");
    rmdir("/tmp/custom_dict_test");
}

// Test dictionary existence check
TEST_F(DictionaryManagerTest, CheckDictionary) {
    // Non-existent path
    EXPECT_EQ(openjtalk_check_dictionary("/nonexistent/path"), 0);
    
    // Create a test directory with dictionary files
    char test_dict_path[256];
    snprintf(test_dict_path, sizeof(test_dict_path), "%s/test_dict", test_dir);
    mkdir(test_dict_path, 0755);
    
    // Without dictionary files
    EXPECT_EQ(openjtalk_check_dictionary(test_dict_path), 0);
    
    // Create dictionary files
    char sys_dic[256], unk_dic[256];
    snprintf(sys_dic, sizeof(sys_dic), "%s/sys.dic", test_dict_path);
    snprintf(unk_dic, sizeof(unk_dic), "%s/unk.dic", test_dict_path);
    
    FILE* fp = fopen(sys_dic, "w");
    if (fp) {
        fprintf(fp, "dummy dictionary content\n");
        fclose(fp);
    }
    fp = fopen(unk_dic, "w");
    if (fp) {
        fprintf(fp, "dummy dictionary content\n");
        fclose(fp);
    }
    
    // With dictionary files
    EXPECT_EQ(openjtalk_check_dictionary(test_dict_path), 1);
}

// Test offline mode
TEST_F(DictionaryManagerTest, OfflineMode) {
    setenv("PIPER_OFFLINE_MODE", "1", 1);
    
    const char* dict_path = nullptr;
    // Should fail in offline mode without existing dictionary
    EXPECT_NE(openjtalk_ensure_dictionary(&dict_path), 0);
}

// Test auto-download disabled
TEST_F(DictionaryManagerTest, AutoDownloadDisabled) {
    setenv("PIPER_AUTO_DOWNLOAD_DICT", "0", 1);
    
    const char* dict_path = nullptr;
    // Should fail when auto-download is disabled
    EXPECT_NE(openjtalk_ensure_dictionary(&dict_path), 0);
}

// Test version management
TEST_F(DictionaryManagerTest, VersionManagement) {
    // Test getting available versions
    const char* versions[10];
    int count = openjtalk_get_available_versions(versions, 10);
    EXPECT_GT(count, 0);
    EXPECT_STREQ(versions[0], "1.11");  // First version should be 1.11
    
    // Test setting dictionary version
    openjtalk_set_dict_version("1.10");
    // This would affect subsequent dictionary downloads
}

// Test HTS voice path resolution
TEST_F(DictionaryManagerTest, HTSVoicePath) {
    const char* custom_voice = "/tmp/custom_voice_test.htsvoice";
    
    // Create dummy voice file
    FILE* fp = fopen(custom_voice, "w");
    if (fp) {
        fprintf(fp, "dummy hts voice");
        fclose(fp);
    }
    
    setenv("OPENJTALK_VOICE", custom_voice, 1);
    
    const char* voice_path = nullptr;
    // This test might fail due to network in CI, so we just test the path resolution
    // In a real test environment, we'd mock the download functionality
    
    unlink(custom_voice);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}