#ifndef OPENJTALK_API_H_
#define OPENJTALK_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

// Forward declarations
typedef struct _OpenJTalk OpenJTalk;
typedef struct _HTS_Label HTS_Label;

// OpenJTalk wrapper functions
OpenJTalk* openjtalk_initialize();
void openjtalk_finalize(OpenJTalk* oj);
HTS_Label* openjtalk_extract_fullcontext(OpenJTalk* oj, const char* text);

// HTS Label functions
size_t HTS_Label_get_size(HTS_Label* label);
const char* HTS_Label_get_string(HTS_Label* label, size_t index);
void HTS_Label_clear(HTS_Label* label);

#ifdef __cplusplus
}
#endif

#endif // OPENJTALK_API_H_