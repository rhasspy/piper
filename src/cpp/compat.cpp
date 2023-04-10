#include <filesystem>
#include <string>

#if _MSC_VER
#include <windows.h>

namespace {
  std::string wstringToString(const std::wstring& ws) {
    char tmpMb[4096];
    const int len = WideCharToMultiByte(CP_ACP, 0, ws.c_str(), -1, tmpMb, (int) sizeof(tmpMb), NULL, NULL);
    tmpMb[len] = 0;
    return std::string(tmpMb);
  }
}

std::string compat_filesystem_path_to_string(std::filesystem::path path) {
  return wstringToString(path);
}
#else
std::string compat_filesystem_path_to_string(std::filesystem::path path) {
  return path;
}
#endif
