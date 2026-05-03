#pragma once

#include <string>
#include <chrono>

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
    File(std::string file_id, std::string chat_id, long long message_id, std::string name_file, std::optional<std::string> tag = std::nullopt);

    std::string& GetData();

    void EditData(const std::string& new_data);

    void swap(File& other) noexcept;

    void Delete();

    static File CreateFile(const std::string& data, const std::string& chat_id, 
        const std::string& name_file, std::optional<std::string> tag=std::nullopt);
private:
    bool dirty;
    bool deleted;
    std::string file_id;
    std::string chat_id;
    std::string name_file;
    std::optional<std::string> tag;
    std::optional<std::string> data;
    long long message_id;
    FilePath file_path;
};

}
