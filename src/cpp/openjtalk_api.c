#include "openjtalk_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include the individual OpenJTalk component headers
#include "text2mecab.h"
#include "mecab.h"
#include "njd.h"  
#include "jpcommon.h"
#include "mecab2njd.h"
#include "njd2jpcommon.h"
#include "njd_set_pronunciation.h"
#include "njd_set_digit.h"
#include "njd_set_accent_phrase.h"
#include "njd_set_accent_type.h"
#include "njd_set_long_vowel.h"
#include "njd_set_unvoiced_vowel.h"

// OpenJTalk wrapper structure
struct _OpenJTalk {
    Mecab mecab;
    NJD njd;
    JPCommon jpcommon;
    int initialized;
};

// HTS Label wrapper structure that holds JPCommon labels
struct _HTS_Label {
    JPCommon* jpcommon;
    int size;
};

OpenJTalk* openjtalk_initialize() {
    OpenJTalk* oj = (OpenJTalk*)malloc(sizeof(OpenJTalk));
    if (!oj) return NULL;
    
    oj->initialized = 0;
    
    // Initialize MeCab
    Mecab_initialize(&oj->mecab);
    
    // Initialize NJD
    NJD_initialize(&oj->njd);
    
    // Initialize JPCommon
    JPCommon_initialize(&oj->jpcommon);
    
    oj->initialized = 1;
    return oj;
}

void openjtalk_finalize(OpenJTalk* oj) {
    if (!oj) return;
    
    if (oj->initialized) {
        JPCommon_clear(&oj->jpcommon);
        NJD_clear(&oj->njd);
        Mecab_clear(&oj->mecab);
    }
    
    free(oj);
}

HTS_Label* openjtalk_extract_fullcontext(OpenJTalk* oj, const char* text) {
    if (!oj || !oj->initialized || !text) return NULL;
    
    // Allocate buffer for MeCab output (estimate size)
    size_t text_len = strlen(text);
    size_t buffer_size = text_len * 10; // Generous estimate for MeCab output
    char* mecab_output = (char*)malloc(buffer_size);
    if (!mecab_output) return NULL;
    
    // Convert text to MeCab format
    text2mecab(mecab_output, text);
    
    // Clear previous analysis
    NJD_clear(&oj->njd);
    NJD_initialize(&oj->njd);
    
    // Analyze with MeCab
    Mecab_analysis(&oj->mecab, mecab_output);
    
    // Convert MeCab output to NJD
    mecab2njd(&oj->njd, Mecab_get_feature(&oj->mecab), Mecab_get_size(&oj->mecab));
    
    // Process through NJD stages
    njd_set_pronunciation(&oj->njd);
    njd_set_digit(&oj->njd);
    njd_set_accent_phrase(&oj->njd);
    njd_set_accent_type(&oj->njd);
    njd_set_unvoiced_vowel(&oj->njd);
    njd_set_long_vowel(&oj->njd);
    
    // Clear previous JPCommon analysis
    JPCommon_refresh(&oj->jpcommon);
    
    // Convert to JPCommon
    njd2jpcommon(&oj->jpcommon, &oj->njd);
    
    // Make full-context labels
    JPCommon_make_label(&oj->jpcommon);
    
    // Create HTS_Label wrapper
    HTS_Label* label = (HTS_Label*)malloc(sizeof(HTS_Label));
    if (!label) {
        free(mecab_output);
        return NULL;
    }
    
    label->jpcommon = &oj->jpcommon;
    label->size = JPCommon_get_label_size(&oj->jpcommon);
    
    free(mecab_output);
    return label;
}

size_t HTS_Label_get_size(HTS_Label* label) {
    if (!label) return 0;
    return label->size;
}

const char* HTS_Label_get_string(HTS_Label* label, size_t index) {
    if (!label || !label->jpcommon || index >= label->size) return NULL;
    
    char** features = JPCommon_get_label_feature(label->jpcommon);
    if (!features) return NULL;
    
    return features[index];
}

void HTS_Label_clear(HTS_Label* label) {
    if (!label) return;
    // JPCommon cleanup is handled by openjtalk_finalize
    // Don't free the JPCommon here as it's owned by OpenJTalk
    free(label);
}