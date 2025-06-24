#include <iostream>
#include <filesystem>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

std::string findEspeakDataPath() {
    // First, check environment variable
    const char* env_path = getenv("ESPEAK_DATA_PATH");
    if (env_path && access(env_path, F_OK) == 0) {
        std::cout << "Found via ESPEAK_DATA_PATH env: " << env_path << std::endl;
        return env_path;
    }
    
    // Try to find data relative to executable
    char exe_path[4096] = {0};
    
#ifdef _WIN32
    DWORD size = GetModuleFileNameA(NULL, exe_path, sizeof(exe_path));
    if (size == 0 || size >= sizeof(exe_path)) {
        exe_path[0] = '\0';
    }
#elif defined(__APPLE__)
    uint32_t size = sizeof(exe_path);
    if (_NSGetExecutablePath(exe_path, &size) != 0) {
        exe_path[0] = '\0';
    }
#elif defined(__linux__)
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len > 0) {
        exe_path[len] = '\0';
    } else {
        exe_path[0] = '\0';
    }
#endif
    
    if (exe_path[0] != '\0') {
        std::cout << "Executable path: " << exe_path << std::endl;
        std::filesystem::path exePath(exe_path);
        std::filesystem::path exeDir = exePath.parent_path();
        std::cout << "Executable directory: " << exeDir << std::endl;
        
        // Try different relative locations
        std::vector<std::filesystem::path> candidates = {
            exeDir / "espeak-ng-data",                    // Same directory as exe
            exeDir / ".." / "share" / "espeak-ng-data",   // Installed location
            exeDir / ".." / "espeak-ng-data",             // Alternative location
            exeDir / ".." / "lib" / "espeak-ng-data"      // Another alternative
        };
        
        for (const auto& candidate : candidates) {
            auto absPath = std::filesystem::absolute(candidate);
            std::cout << "Checking: " << absPath << " ... ";
            if (std::filesystem::exists(absPath)) {
                std::cout << "EXISTS!" << std::endl;
                return absPath.string();
            }
            std::cout << "not found" << std::endl;
        }
    }
    
    std::cout << "Could not find espeak-ng-data directory" << std::endl;
    return "";
}

int main() {
    std::cout << "Testing espeak-ng data path discovery..." << std::endl;
    std::string path = findEspeakDataPath();
    if (!path.empty()) {
        std::cout << "Final result: " << path << std::endl;
        return 0;
    } else {
        std::cout << "Final result: NOT FOUND" << std::endl;
        return 1;
    }
}