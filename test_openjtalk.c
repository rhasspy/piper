#include <stdio.h>
#include <stdlib.h>
#include "src/cpp/openjtalk_api.h"

int main(int argc, char* argv[]) {
    const char* text = argc > 1 ? argv[1] : "こんにちは";
    
    printf("Initializing OpenJTalk...\n");
    OpenJTalk* oj = openjtalk_initialize();
    
    if (!oj) {
        fprintf(stderr, "Failed to initialize OpenJTalk\n");
        return 1;
    }
    
    printf("Extracting fullcontext for: %s\n", text);
    OJ_Label* label = openjtalk_extract_fullcontext(oj, text);
    
    if (!label) {
        fprintf(stderr, "Failed to extract fullcontext\n");
        openjtalk_finalize(oj);
        return 1;
    }
    
    size_t label_size = OJ_Label_get_size(label);
    printf("Generated %zu labels:\n", label_size);
    
    for (size_t i = 0; i < label_size && i < 10; i++) {
        const char* label_str = OJ_Label_get_string(label, i);
        if (label_str) {
            printf("  [%zu]: %s\n", i, label_str);
        }
    }
    
    if (label_size > 10) {
        printf("  ... (%zu more labels)\n", label_size - 10);
    }
    
    OJ_Label_clear(label);
    openjtalk_finalize(oj);
    
    printf("Test completed successfully!\n");
    return 0;
}