#include "openjtalk_wrapper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#ifndef _WIN32

// Real OpenJTalk implementation for Unix platforms using binary execution
struct OpenJTalk_impl {
    char* dic_path;
    char* openjtalk_bin;
    int initialized;
};

struct HTS_Label_Wrapper_impl {
    char** labels;
    size_t size;
    size_t capacity;
};

OpenJTalk* openjtalk_initialize() {
    struct OpenJTalk_impl* oj = (struct OpenJTalk_impl*)calloc(1, sizeof(struct OpenJTalk_impl));
    if (!oj) return NULL;
    
    // Check if dictionary exists - prefer environment variable
    const char* dic_path = getenv("OPENJTALK_DICTIONARY_DIR");
    if (!dic_path) {
        dic_path = OPENJTALK_DIC_PATH;
    }
    
    if (access(dic_path, R_OK) != 0) {
        fprintf(stderr, "OpenJTalk dictionary not found at: %s\n", dic_path);
        fprintf(stderr, "Please set OPENJTALK_DICTIONARY_DIR environment variable or install dictionary at the default location\n");
        free(oj);
        return NULL;
    }
    
    // Find open_jtalk binary
    const char* possible_paths[] = {
        "./oj/bin/open_jtalk",
        "../oj/bin/open_jtalk",
        "../../build/oj/bin/open_jtalk",
        "/usr/local/bin/open_jtalk",
        "/usr/bin/open_jtalk",
        NULL
    };
    
    const char* found_bin = NULL;
    for (int i = 0; possible_paths[i] != NULL; i++) {
        if (access(possible_paths[i], X_OK) == 0) {
            found_bin = possible_paths[i];
            break;
        }
    }
    
    if (!found_bin) {
        // Try to find it relative to build directory
        char build_path[512];
        snprintf(build_path, sizeof(build_path), "%s/../oj/bin/open_jtalk", dic_path);
        if (access(build_path, X_OK) == 0) {
            found_bin = build_path;
        }
    }
    
    if (!found_bin) {
        fprintf(stderr, "open_jtalk binary not found\n");
        free(oj);
        return NULL;
    }
    
    oj->dic_path = strdup(dic_path);
    oj->openjtalk_bin = strdup(found_bin);
    oj->initialized = 1;
    
    return (OpenJTalk*)oj;
}

void openjtalk_finalize(OpenJTalk* oj) {
    if (!oj) return;
    
    struct OpenJTalk_impl* impl = (struct OpenJTalk_impl*)oj;
    
    if (impl->dic_path) free(impl->dic_path);
    if (impl->openjtalk_bin) free(impl->openjtalk_bin);
    
    free(oj);
}

// Parse phoneme from OpenJTalk label format
static char* extract_phoneme_from_label(const char* label) {
    // OpenJTalk label format includes phoneme after "-" and before "+"
    // Example: "xx^xx-sil+xx=xx/A:xx..."
    const char* start = strchr(label, '-');
    if (!start) return NULL;
    start++; // Skip '-'
    
    const char* end = strchr(start, '+');
    if (!end) return NULL;
    
    size_t len = end - start;
    if (len == 0) return NULL;
    
    char* phoneme = (char*)malloc(len + 1);
    if (!phoneme) return NULL;
    
    strncpy(phoneme, start, len);
    phoneme[len] = '\0';
    
    return phoneme;
}

HTS_Label_Wrapper* openjtalk_extract_fullcontext(OpenJTalk* oj, const char* text) {
    if (!oj || !text || strlen(text) == 0) return NULL;
    
    struct OpenJTalk_impl* impl = (struct OpenJTalk_impl*)oj;
    if (!impl->initialized) return NULL;
    
    // Create temporary files
    char input_file[] = "/tmp/openjtalk_input_XXXXXX";
    char output_file[] = "/tmp/openjtalk_output_XXXXXX";
    char trace_file[] = "/tmp/openjtalk_trace_XXXXXX";
    
    int input_fd = mkstemp(input_file);
    if (input_fd < 0) return NULL;
    
    int output_fd = mkstemp(output_file);
    if (output_fd < 0) {
        close(input_fd);
        unlink(input_file);
        return NULL;
    }
    
    int trace_fd = mkstemp(trace_file);
    if (trace_fd < 0) {
        close(input_fd);
        close(output_fd);
        unlink(input_file);
        unlink(output_file);
        return NULL;
    }
    
    // Write input text
    FILE* fp = fdopen(input_fd, "w");
    if (!fp) {
        close(input_fd);
        close(output_fd);
        close(trace_fd);
        unlink(input_file);
        unlink(output_file);
        unlink(trace_file);
        return NULL;
    }
    
    fprintf(fp, "%s\n", text);
    fclose(fp);
    close(output_fd);
    close(trace_fd);
    
    // Run open_jtalk with trace output
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        execl(impl->openjtalk_bin, "open_jtalk",
              "-x", impl->dic_path,
              "-ow", output_file,
              "-ot", trace_file,
              input_file,
              NULL);
        _exit(1);
    } else if (pid < 0) {
        // Fork failed
        unlink(input_file);
        unlink(output_file);
        unlink(trace_file);
        return NULL;
    }
    
    // Wait for process
    int status;
    waitpid(pid, &status, 0);
    
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        unlink(input_file);
        unlink(output_file);
        unlink(trace_file);
        return NULL;
    }
    
    // Read trace file for labels
    fp = fopen(trace_file, "r");
    if (!fp) {
        unlink(input_file);
        unlink(output_file);
        unlink(trace_file);
        return NULL;
    }
    
    // Create wrapper
    struct HTS_Label_Wrapper_impl* wrapper = (struct HTS_Label_Wrapper_impl*)calloc(1, sizeof(struct HTS_Label_Wrapper_impl));
    if (!wrapper) {
        fclose(fp);
        unlink(input_file);
        unlink(output_file);
        unlink(trace_file);
        return NULL;
    }
    
    // Read labels
    char line[4096];
    size_t capacity = 64;
    wrapper->labels = (char**)calloc(capacity, sizeof(char*));
    if (!wrapper->labels) {
        free(wrapper);
        fclose(fp);
        unlink(input_file);
        unlink(output_file);
        unlink(trace_file);
        return NULL;
    }
    
    wrapper->size = 0;
    wrapper->capacity = capacity;
    
    while (fgets(line, sizeof(line), fp)) {
        // Remove newline
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        
        // Skip empty lines
        if (strlen(line) == 0) continue;
        
        // Check if this is a label line (contains phoneme information)
        if (strstr(line, "-") && strstr(line, "+") && strstr(line, "/")) {
            // Grow array if needed
            if (wrapper->size >= wrapper->capacity) {
                size_t new_capacity = wrapper->capacity * 2;
                char** new_labels = (char**)realloc(wrapper->labels, new_capacity * sizeof(char*));
                if (!new_labels) {
                    // Cleanup on error
                    for (size_t i = 0; i < wrapper->size; i++) {
                        free(wrapper->labels[i]);
                    }
                    free(wrapper->labels);
                    free(wrapper);
                    fclose(fp);
                    unlink(input_file);
                    unlink(output_file);
                    unlink(trace_file);
                    return NULL;
                }
                wrapper->labels = new_labels;
                wrapper->capacity = new_capacity;
            }
            
            // Store the full label
            wrapper->labels[wrapper->size] = strdup(line);
            if (!wrapper->labels[wrapper->size]) {
                // Cleanup on error
                for (size_t i = 0; i < wrapper->size; i++) {
                    free(wrapper->labels[i]);
                }
                free(wrapper->labels);
                free(wrapper);
                fclose(fp);
                unlink(input_file);
                unlink(output_file);
                unlink(trace_file);
                return NULL;
            }
            wrapper->size++;
        }
    }
    
    fclose(fp);
    
    // Cleanup temp files
    unlink(input_file);
    unlink(output_file);
    unlink(trace_file);
    
    if (wrapper->size == 0) {
        free(wrapper->labels);
        free(wrapper);
        return NULL;
    }
    
    return (HTS_Label_Wrapper*)wrapper;
}

size_t HTS_Label_get_size(HTS_Label_Wrapper* label) {
    if (!label) return 0;
    struct HTS_Label_Wrapper_impl* impl = (struct HTS_Label_Wrapper_impl*)label;
    return impl->size;
}

const char* HTS_Label_get_string(HTS_Label_Wrapper* label, size_t index) {
    if (!label) return NULL;
    struct HTS_Label_Wrapper_impl* impl = (struct HTS_Label_Wrapper_impl*)label;
    if (index >= impl->size) return NULL;
    return impl->labels[index];
}

void HTS_Label_clear(HTS_Label_Wrapper* label) {
    if (!label) return;
    struct HTS_Label_Wrapper_impl* impl = (struct HTS_Label_Wrapper_impl*)label;
    
    if (impl->labels) {
        for (size_t i = 0; i < impl->size; i++) {
            free(impl->labels[i]);
        }
        free(impl->labels);
    }
    
    free(label);
}

#else

// Windows stub implementation
OpenJTalk* openjtalk_initialize() {
    return NULL;
}

void openjtalk_finalize(OpenJTalk* oj) {
    (void)oj;
}

HTS_Label_Wrapper* openjtalk_extract_fullcontext(OpenJTalk* oj, const char* text) {
    (void)oj;
    (void)text;
    return NULL;
}

size_t HTS_Label_get_size(HTS_Label_Wrapper* label) {
    (void)label;
    return 0;
}

const char* HTS_Label_get_string(HTS_Label_Wrapper* label, size_t index) {
    (void)label;
    (void)index;
    return NULL;
}

void HTS_Label_clear(HTS_Label_Wrapper* label) {
    (void)label;
}

#endif // _WIN32