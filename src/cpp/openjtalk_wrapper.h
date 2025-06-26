#ifndef OPENJTALK_WRAPPER_H_
#define OPENJTALK_WRAPPER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

// Forward declarations
typedef struct OpenJTalk_impl OpenJTalk;
typedef struct HTS_Label_Wrapper_impl HTS_Label_Wrapper;

// Main API functions
OpenJTalk* openjtalk_initialize();
void openjtalk_finalize(OpenJTalk* oj);
HTS_Label_Wrapper* openjtalk_extract_fullcontext(OpenJTalk* oj, const char* text);

// Label access functions
size_t HTS_Label_get_size(HTS_Label_Wrapper* label);
const char* HTS_Label_get_string(HTS_Label_Wrapper* label, size_t index);
void HTS_Label_clear(HTS_Label_Wrapper* label);

#ifdef __cplusplus
}
#endif

#endif // OPENJTALK_WRAPPER_H_