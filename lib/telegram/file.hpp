#pragma once

#include <string>
#include <chrono>
#include <format>
#include "cpr/cpr.h"

namespace tg {

class FilePath {
public:
    std::string GetPath(const std::string& file_id);
private:
    std::string path;
    std::chrono::system_clock::time_point deadline;
};

class File {
public:
    File(std::string file_id, std::string chat_id);

    std::string& GetData();

    static File CreateFile(const std::string& data, const std::string& chat_id, 
        const std::string& name_file, std::optional<std::string> tag=std::nullopt);
private:
    bool enabled_data;
    std::string file_id;
    std::string chat_id;
    std::string data;
    FilePath file_path;
};

}
