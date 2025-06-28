#ifndef OPENJTALK_DICTIONARY_MANAGER_H
#define OPENJTALK_DICTIONARY_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

// OpenJTalk dictionary information
typedef struct {
    const char* dict_version;
    const char* dict_url;
    const char* dict_sha256;
    size_t dict_size;
    const char* dict_filename;
} OpenJTalkDictInfo;

// HTS voice model information
typedef struct {
    const char* voice_name;
    const char* voice_url;
    const char* voice_sha256;
    size_t voice_size;
    const char* voice_filename;
} HTSVoiceInfo;

// Dictionary manager functions
/**
 * Ensure OpenJTalk dictionary is available
 * @param dict_path_out Output parameter for dictionary path
 * @return 0 on success, non-zero on error
 */
int openjtalk_ensure_dictionary(const char** dict_path_out);

/**
 * Download OpenJTalk dictionary to specified directory
 * @param target_dir Target directory for download
 * @return 0 on success, non-zero on error
 */
int openjtalk_download_dictionary(const char* target_dir);

/**
 * Check if dictionary exists at specified path
 * @param dict_path Path to check
 * @return 1 if exists, 0 if not exists
 */
int openjtalk_check_dictionary(const char* dict_path);

/**
 * Get default dictionary path
 * @param buffer Output buffer for path
 * @param buffer_size Size of output buffer
 * @return 0 on success, non-zero on error
 */
int openjtalk_get_default_dict_path(char* buffer, size_t buffer_size);

/**
 * Set preferred dictionary version
 * @param version Version string (e.g., "1.11", "1.10")
 */
void openjtalk_set_dict_version(const char* version);

/**
 * Get available dictionary versions
 * @param versions Output array of version strings
 * @param max_versions Maximum number of versions to return
 * @return Number of available versions
 */
int openjtalk_get_available_versions(const char** versions, int max_versions);

// HTS voice functions
/**
 * Ensure HTS voice model is available
 * @param voice_path_out Output parameter for voice model path
 * @return 0 on success, non-zero on error
 */
int openjtalk_ensure_hts_voice(const char** voice_path_out);

/**
 * Download HTS voice model to specified directory
 * @param target_dir Target directory for download
 * @return 0 on success, non-zero on error
 */
int openjtalk_download_hts_voice(const char* target_dir);

#ifdef __cplusplus
}
#endif

#endif // OPENJTALK_DICTIONARY_MANAGER_H