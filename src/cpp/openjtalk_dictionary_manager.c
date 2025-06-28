#include "openjtalk_dictionary_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#define access(path, mode) _access(path, mode)
#define R_OK 4
#define strcasecmp _stricmp
#define popen _popen
#define pclose _pclose
#else
#include <unistd.h>
#include <pwd.h>
#include <strings.h>
#endif

// Dictionary information for multiple versions
static const OpenJTalkDictInfo DICT_VERSIONS[] = {
    {
        "1.11",  // dict_version (latest)
        "https://sourceforge.net/projects/open-jtalk/files/Dictionary/open_jtalk_dic-1.11/open_jtalk_dic_utf_8-1.11.tar.gz/download",  // dict_url
        "33e9cd251bc41aa2bd7ca36f57abbf61eae3543ca25ca892ae345e394cb10549",  // dict_sha256
        10305862,  // dict_size (approximate size in bytes)
        "open_jtalk_dic_utf_8-1.11.tar.gz"  // dict_filename
    },
    {
        "1.10",  // dict_version
        "https://sourceforge.net/projects/open-jtalk/files/Dictionary/open_jtalk_dic-1.10/open_jtalk_dic_utf_8-1.10.tar.gz/download",  // dict_url
        "",  // dict_sha256 (not available for older version)
        10301296,  // dict_size (approximate)
        "open_jtalk_dic_utf_8-1.10.tar.gz"  // dict_filename
    }
};

static const int DICT_VERSION_COUNT = sizeof(DICT_VERSIONS) / sizeof(DICT_VERSIONS[0]);
static const char* preferred_dict_version = NULL;

// Default to the latest version
static const OpenJTalkDictInfo* get_dict_info() {
    if (preferred_dict_version) {
        for (int i = 0; i < DICT_VERSION_COUNT; i++) {
            if (strcmp(DICT_VERSIONS[i].dict_version, preferred_dict_version) == 0) {
                return &DICT_VERSIONS[i];
            }
        }
    }
    return &DICT_VERSIONS[0];  // Default to latest
}

static int ensure_directory_exists(const char* path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode) ? 0 : -1;
    }
    
    // Create parent directories recursively
    char* path_copy = strdup(path);
    char* p = path_copy;
    
    while (*p) {
        if (*p == '/' && p != path_copy) {
            *p = '\0';
            if (mkdir(path_copy, 0755) != 0 && errno != EEXIST) {
                free(path_copy);
                return -1;
            }
            *p = '/';
        }
        p++;
    }
    
    int result = mkdir(path_copy, 0755);
    free(path_copy);
    return (result == 0 || errno == EEXIST) ? 0 : -1;
}

int openjtalk_check_dictionary(const char* dict_path) {
    if (!dict_path) return 0;
    
    struct stat st;
    if (stat(dict_path, &st) != 0) return 0;
    if (!S_ISDIR(st.st_mode)) return 0;
    
    // Check for essential dictionary files
    char filepath[1024];
    snprintf(filepath, sizeof(filepath), "%s/sys.dic", dict_path);
    if (access(filepath, R_OK) != 0) return 0;
    
    snprintf(filepath, sizeof(filepath), "%s/unk.dic", dict_path);
    if (access(filepath, R_OK) != 0) return 0;
    
    return 1;
}

int openjtalk_get_default_dict_path(char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) return -1;
    
    // Priority 1: Environment variable
    const char* env_path = getenv("OPENJTALK_DICTIONARY_DIR");
    if (env_path && openjtalk_check_dictionary(env_path)) {
        strncpy(buffer, env_path, buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
        return 0;
    }
    
    // Priority 2: User home directory
    const char* home_dir = NULL;
#ifdef _WIN32
    char win_path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, win_path))) {
        home_dir = win_path;
    }
#else
    home_dir = getenv("HOME");
    if (!home_dir) {
        struct passwd* pw = getpwuid(getuid());
        if (pw) home_dir = pw->pw_dir;
    }
#endif
    
    if (home_dir) {
        const OpenJTalkDictInfo* dict_info = get_dict_info();
        snprintf(buffer, buffer_size, "%s/.piper/dictionaries/openjtalk/open_jtalk_dic_utf_8-%s", 
                 home_dir, dict_info->dict_version);
        if (openjtalk_check_dictionary(buffer)) return 0;
    }
    
    // Priority 3: XDG_DATA_HOME (Linux)
#ifndef _WIN32
    const char* xdg_data = getenv("XDG_DATA_HOME");
    if (xdg_data) {
        snprintf(buffer, buffer_size, "%s/piper/dictionaries/openjtalk/open_jtalk_dic_utf_8-1.11", xdg_data);
        if (openjtalk_check_dictionary(buffer)) return 0;
    } else if (home_dir) {
        snprintf(buffer, buffer_size, "%s/.local/share/piper/dictionaries/openjtalk/open_jtalk_dic_utf_8-1.11", home_dir);
        if (openjtalk_check_dictionary(buffer)) return 0;
    }
#endif
    
    // Priority 4: Relative to binary (compile-time default)
#ifdef OPENJTALK_DIC_PATH
    strncpy(buffer, OPENJTALK_DIC_PATH, buffer_size - 1);
    buffer[buffer_size - 1] = '\0';
    if (openjtalk_check_dictionary(buffer)) return 0;
#endif
    
    // If no dictionary found, return default download location
    if (home_dir) {
        const OpenJTalkDictInfo* dict_info = get_dict_info();
        snprintf(buffer, buffer_size, "%s/.piper/dictionaries/openjtalk/open_jtalk_dic_utf_8-%s", 
                 home_dir, dict_info->dict_version);
        return 0;
    }
    
    return -1;
}

static int should_auto_download() {
    const char* auto_download = getenv("PIPER_AUTO_DOWNLOAD_DICT");
    if (auto_download && strcmp(auto_download, "0") == 0) return 0;
    
    const char* offline_mode = getenv("PIPER_OFFLINE_MODE");
    if (offline_mode && strcmp(offline_mode, "1") == 0) return 0;
    
    return 1;
}

static int extract_tar_gz(const char* tar_gz_path, const char* target_dir) {
    char command[2048];
    
    // Create target directory
    if (ensure_directory_exists(target_dir) != 0) {
        fprintf(stderr, "Failed to create directory: %s\n", target_dir);
        return -1;
    }
    
    // Extract using tar command
    snprintf(command, sizeof(command), "tar -xzf \"%s\" -C \"%s\" --strip-components=0", tar_gz_path, target_dir);
    
    int result = system(command);
    if (result != 0) {
        fprintf(stderr, "Failed to extract dictionary: tar command failed\n");
        return -1;
    }
    
    return 0;
}

static int calculate_sha256(const char* filename, char* output_hash) {
    // Use system command to calculate SHA256
    char command[2048];
    
#ifdef __APPLE__
    // macOS uses shasum command
    snprintf(command, sizeof(command), "shasum -a 256 \"%s\" 2>/dev/null | cut -d' ' -f1", filename);
#else
    // Linux uses sha256sum command
    snprintf(command, sizeof(command), "sha256sum \"%s\" 2>/dev/null | cut -d' ' -f1", filename);
#endif
    
    FILE* fp = popen(command, "r");
    if (!fp) {
        return -1;
    }
    
    if (fgets(output_hash, 65, fp) == NULL) {
        pclose(fp);
        return -1;
    }
    
    // Remove newline
    size_t len = strlen(output_hash);
    if (len > 0 && output_hash[len-1] == '\n') {
        output_hash[len-1] = '\0';
    }
    
    pclose(fp);
    return 0;
}

static int verify_checksum(const char* filename, const char* expected_sha256) {
    char actual_hash[65] = {0};
    
    if (calculate_sha256(filename, actual_hash) != 0) {
        fprintf(stderr, "Failed to calculate SHA256 checksum\n");
        return -1;
    }
    
    if (strcasecmp(actual_hash, expected_sha256) != 0) {
        fprintf(stderr, "Checksum mismatch!\n");
        fprintf(stderr, "Expected: %s\n", expected_sha256);
        fprintf(stderr, "Actual:   %s\n", actual_hash);
        return -1;
    }
    
    fprintf(stdout, "Checksum verified successfully\n");
    return 0;
}

static int download_file_with_curl(const char* url, const char* output_path, int support_resume) {
    char command[2048];
    
    // Check if we're in a CI environment
    const char* ci_env = getenv("CI");
    const char* progress_opt = (ci_env && strcmp(ci_env, "true") == 0) ? "-s" : "--progress-bar";
    
    // Check if partial file exists for resume
    struct stat st;
    int partial_exists = (stat(output_path, &st) == 0);
    
    // Use curl with follow redirects and optional resume
    if (support_resume && partial_exists) {
        fprintf(stdout, "Resuming download from %ld bytes...\n", (long)st.st_size);
        snprintf(command, sizeof(command), 
                 "curl -L -C - %s -o \"%s\" \"%s\"", 
                 progress_opt, output_path, url);
    } else {
        snprintf(command, sizeof(command), 
                 "curl -L %s -o \"%s\" \"%s\"", 
                 progress_opt, output_path, url);
    }
    
    fprintf(stdout, "Downloading file...\n");
    fprintf(stdout, "URL: %s\n", url);
    fprintf(stdout, "Destination: %s\n", output_path);
    
    int result = system(command);
    if (result != 0) {
        fprintf(stderr, "Failed to download: curl command failed\n");
        return -1;
    }
    
    return 0;
}

int openjtalk_download_dictionary(const char* target_dir) {
    if (!target_dir) return -1;
    
    // Create parent directory for dictionary
    char parent_dir[1024];
    snprintf(parent_dir, sizeof(parent_dir), "%s", target_dir);
    char* last_slash = strrchr(parent_dir, '/');
    if (last_slash) {
        *last_slash = '\0';
        if (ensure_directory_exists(parent_dir) != 0) {
            fprintf(stderr, "Failed to create parent directory: %s\n", parent_dir);
            return -1;
        }
    }
    
    // Download dictionary
    char download_path[1024];
    snprintf(download_path, sizeof(download_path), "%s.tar.gz", target_dir);
    
    const OpenJTalkDictInfo* dict_info = get_dict_info();
    fprintf(stdout, "Downloading OpenJTalk dictionary (version %s)...\n", dict_info->dict_version);
    if (download_file_with_curl(dict_info->dict_url, download_path, 1) != 0) {
        return -1;
    }
    
    // Verify checksum if SHA256 is provided
    if (dict_info->dict_sha256 && strlen(dict_info->dict_sha256) > 0) {
        fprintf(stdout, "Verifying checksum...\n");
        if (verify_checksum(download_path, dict_info->dict_sha256) != 0) {
            unlink(download_path);
            fprintf(stderr, "Checksum verification failed. File may be corrupted.\n");
            return -1;
        }
    }
    
    // Extract dictionary
    if (extract_tar_gz(download_path, parent_dir) != 0) {
        unlink(download_path);
        return -1;
    }
    
    // Clean up downloaded archive
    unlink(download_path);
    
    // Verify extraction
    if (!openjtalk_check_dictionary(target_dir)) {
        fprintf(stderr, "Dictionary extraction verification failed\n");
        return -1;
    }
    
    fprintf(stdout, "Successfully downloaded and extracted OpenJTalk dictionary to: %s\n", target_dir);
    return 0;
}

int openjtalk_ensure_dictionary(const char** dict_path_out) {
    static char dict_path_buffer[1024];
    
    // Get default dictionary path
    if (openjtalk_get_default_dict_path(dict_path_buffer, sizeof(dict_path_buffer)) != 0) {
        fprintf(stderr, "Failed to determine dictionary path\n");
        return -1;
    }
    
    // Check if dictionary already exists
    if (openjtalk_check_dictionary(dict_path_buffer)) {
        if (dict_path_out) *dict_path_out = dict_path_buffer;
        return 0;
    }
    
    // Check if auto-download is enabled
    if (!should_auto_download()) {
        fprintf(stderr, "OpenJTalk dictionary not found at: %s\n", dict_path_buffer);
        fprintf(stderr, "Please download the dictionary manually or set PIPER_AUTO_DOWNLOAD_DICT=1\n");
        return -1;
    }
    
    // Attempt to download dictionary
    fprintf(stdout, "OpenJTalk dictionary not found. Attempting to download...\n");
    if (openjtalk_download_dictionary(dict_path_buffer) != 0) {
        const OpenJTalkDictInfo* dict_info = get_dict_info();
        fprintf(stderr, "Failed to download OpenJTalk dictionary\n");
        fprintf(stderr, "Please download manually from: %s\n", dict_info->dict_url);
        return -1;
    }
    
    if (dict_path_out) *dict_path_out = dict_path_buffer;
    return 0;
}

// HTS Voice model information
static const HTSVoiceInfo VOICE_INFO = {
    "nitech_jp_atr503_m001",  // voice_name
    "https://sourceforge.net/projects/open-jtalk/files/HTS%20voice/hts_voice_nitech_jp_atr503_m001-1.05/hts_voice_nitech_jp_atr503_m001-1.05.tar.gz/download",  // voice_url
    "2e555c88482267b2931c7dbc7ecc0e3df140d6f68fc913aa4822f336c9e0adfc",  // voice_sha256
    1911781,  // voice_size (approximate size in bytes)
    "hts_voice_nitech_jp_atr503_m001-1.05.tar.gz"  // voice_filename
};

static int check_hts_voice(const char* voice_path) {
    if (!voice_path) return 0;
    
    struct stat st;
    if (stat(voice_path, &st) != 0) return 0;
    if (!S_ISREG(st.st_mode)) return 0;
    
    // Check if it's a valid .htsvoice file
    const char* ext = strrchr(voice_path, '.');
    if (!ext || strcmp(ext, ".htsvoice") != 0) return 0;
    
    return 1;
}

static int get_default_hts_voice_path(char* buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) return -1;
    
    // Priority 1: Environment variable
    const char* env_path = getenv("OPENJTALK_VOICE");
    if (env_path && check_hts_voice(env_path)) {
        strncpy(buffer, env_path, buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
        return 0;
    }
    
    // Priority 2: User home directory
    const char* home_dir = NULL;
#ifdef _WIN32
    char win_path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, win_path))) {
        home_dir = win_path;
    }
#else
    home_dir = getenv("HOME");
    if (!home_dir) {
        struct passwd* pw = getpwuid(getuid());
        if (pw) home_dir = pw->pw_dir;
    }
#endif
    
    if (home_dir) {
        snprintf(buffer, buffer_size, "%s/.piper/voices/hts/hts_voice_nitech_jp_atr503_m001-1.05/nitech_jp_atr503_m001.htsvoice", home_dir);
        if (check_hts_voice(buffer)) return 0;
    }
    
    // If no voice found, return default download location
    if (home_dir) {
        snprintf(buffer, buffer_size, "%s/.piper/voices/hts/hts_voice_nitech_jp_atr503_m001-1.05/nitech_jp_atr503_m001.htsvoice", home_dir);
        return 0;
    }
    
    return -1;
}

int openjtalk_download_hts_voice(const char* target_dir) {
    if (!target_dir) return -1;
    
    // Create parent directory for voice
    char parent_dir[1024];
    snprintf(parent_dir, sizeof(parent_dir), "%s", target_dir);
    char* last_slash = strrchr(parent_dir, '/');
    if (last_slash) {
        *last_slash = '\0';
        if (ensure_directory_exists(parent_dir) != 0) {
            fprintf(stderr, "Failed to create parent directory: %s\n", parent_dir);
            return -1;
        }
    }
    
    // Download voice model
    char download_path[1024];
    snprintf(download_path, sizeof(download_path), "%s.tar.gz", target_dir);
    
    fprintf(stdout, "Downloading HTS voice model...\n");
    if (download_file_with_curl(VOICE_INFO.voice_url, download_path, 1) != 0) {
        return -1;
    }
    
    // Verify checksum if SHA256 is provided
    if (VOICE_INFO.voice_sha256 && strlen(VOICE_INFO.voice_sha256) > 0) {
        fprintf(stdout, "Verifying checksum...\n");
        if (verify_checksum(download_path, VOICE_INFO.voice_sha256) != 0) {
            unlink(download_path);
            fprintf(stderr, "Checksum verification failed. File may be corrupted.\n");
            return -1;
        }
    }
    
    // Extract voice model to parent directory
    if (extract_tar_gz(download_path, parent_dir) != 0) {
        unlink(download_path);
        return -1;
    }
    
    // Clean up downloaded archive
    unlink(download_path);
    
    // Verify extraction - the actual voice file path is in the extracted directory
    char voice_file_path[1024];
    snprintf(voice_file_path, sizeof(voice_file_path), "%s/nitech_jp_atr503_m001.htsvoice", target_dir);
    if (!check_hts_voice(voice_file_path)) {
        fprintf(stderr, "HTS voice extraction verification failed\n");
        return -1;
    }
    
    fprintf(stdout, "Successfully downloaded and extracted HTS voice to: %s\n", voice_file_path);
    return 0;
}

int openjtalk_ensure_hts_voice(const char** voice_path_out) {
    static char voice_path_buffer[1024];
    
    // Get default voice path
    if (get_default_hts_voice_path(voice_path_buffer, sizeof(voice_path_buffer)) != 0) {
        fprintf(stderr, "Failed to determine HTS voice path\n");
        return -1;
    }
    
    // Check if voice already exists
    if (check_hts_voice(voice_path_buffer)) {
        if (voice_path_out) *voice_path_out = voice_path_buffer;
        return 0;
    }
    
    // Check if auto-download is enabled
    if (!should_auto_download()) {
        fprintf(stderr, "HTS voice model not found at: %s\n", voice_path_buffer);
        fprintf(stderr, "Please download the voice model manually or set PIPER_AUTO_DOWNLOAD_DICT=1\n");
        return -1;
    }
    
    // Attempt to download voice
    fprintf(stdout, "HTS voice model not found. Attempting to download...\n");
    
    // Extract directory path from full file path
    char voice_dir[1024];
    strncpy(voice_dir, voice_path_buffer, sizeof(voice_dir) - 1);
    voice_dir[sizeof(voice_dir) - 1] = '\0';
    char* last_slash = strrchr(voice_dir, '/');
    if (last_slash) *last_slash = '\0';
    
    if (openjtalk_download_hts_voice(voice_dir) != 0) {
        fprintf(stderr, "Failed to download HTS voice model\n");
        fprintf(stderr, "Please download manually from: %s\n", VOICE_INFO.voice_url);
        return -1;
    }
    
    if (voice_path_out) *voice_path_out = voice_path_buffer;
    return 0;
}

void openjtalk_set_dict_version(const char* version) {
    preferred_dict_version = version;
}

int openjtalk_get_available_versions(const char** versions, int max_versions) {
    int count = 0;
    for (int i = 0; i < DICT_VERSION_COUNT && count < max_versions; i++) {
        versions[count++] = DICT_VERSIONS[i].dict_version;
    }
    return count;
}