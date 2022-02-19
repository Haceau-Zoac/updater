#ifndef _WIN32
# error This project only supported Windows.
#endif // _WIN32

#include <cstring>
#include <iostream>
#include <string>
#include <optional>
#include <cctype>
#include <fstream>
#include <nlohmann/json.hpp>
using namespace nlohmann;
#include <Urlmon.h>
#pragma comment(lib, "Urlmon.lib")

enum class update_status {
  latest,
  available,
  success,
  failed
};

class downloader {
public:
  downloader(wchar_t const* url)
    : url_(url) {}
  ~downloader() { stream_->Release(); }

  auto connect() -> bool {
    if (auto ret{ URLOpenBlockingStreamW(nullptr,
        url_, &stream_, 0, nullptr) }; ret != S_OK) {
      return false;
    }
    return true;
  }

  auto response() -> std::string {
    std::string response;
    char buffer[1024]{0};
    DWORD read_bytes{0};
    do {
      stream_->Read(buffer, 1023, &read_bytes);
      buffer[read_bytes] = 0;
      if (read_bytes > 0) {
        response += buffer;
      }
    } while (read_bytes > 0);
    return response;
  }
private:
  IStream* stream_;
  wchar_t const* url_;
};

class updater {
public:

  updater(std::string ver)
    : version_(ver) {}

  auto check_update() -> update_status {
    downloader api(L"https://api.github.com/repos/Haceau-Zoac/updater/releases/latest");

    if (!api.connect()) {
      return update_status::failed;
    }
    
    response_ = json::parse(api.response());
    if (new_version() <= version_) {
      return update_status::latest;
    }
    return update_status::available;
  }

  auto new_version() -> std::string {
    return response_.value().at("tag_name").get<std::string>();
  }

  auto update() -> update_status {
    if (!response_.has_value()) {
      auto state{ check_update() };
      if (state != update_status::available) {
        return state;
      }
    }

    auto url{ response_.value().at("assets").at(0).at("browser_download_url").get<std::string>() };
    if (URLDownloadToFileA(nullptr, url.c_str(),
        "hello.exe", 0, nullptr) != S_OK) {
      return update_status::failed;
    }

    return update_status::success;
  }

private:
  std::string version_;
  std::optional<json> response_{std::nullopt};
};

auto main() -> int {
  std::ifstream is("./version.txt");
  std::string version;
  std::getline(is, version);
  is.close();
  std::cout << "Updater " << version << std::endl;
  updater updater(version);
  auto status{ updater.check_update() };
  if (status == update_status::available) {
    std::cout << "Update " << updater.new_version() << " is available. Do you want to update? (y/n) ";
    char ch;
    std::cin >> ch;
    if (std::tolower(ch) == 'y') {
      if (updater.update() != update_status::failed) {
        std::ofstream os("./version.txt");
        os << updater.new_version();
        std::cout << "Updated.\n";
      } else {
        std::cout << "Update failed.\n";
      }
    }
  } else if (status == update_status::failed) {
    std::cout << "Check update failed.\n";
  }
}