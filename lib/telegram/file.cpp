#include <nlohmann/json.hpp>
#include "file.hpp"

namespace tg {

static const std::string BOT_TOKEN = std::getenv("BOT_TOKEN") ? std::getenv("BOT_TOKEN") : "";

File::File(std::string file_id, std::string chat_id) : 
    enabled_data(false),
    file_id(std::move(file_id)), 
    chat_id(std::move(chat_id)) {}

std::string& File::GetData() {
    if (!enabled_data) {
        auto response = cpr::Get(
            cpr::Url(std::format("https://api.telegram.org/file/bot{}/documents/{}", BOT_TOKEN, file_path.GetPath(file_id)))
        );

        data = response.text;
        enabled_data = true;
    }

    return data;
}

File File::CreateFile(const std::string &data, const std::string& chat_id, 
    const std::string& name_file, std::optional<std::string> tag) {

    auto response = cpr::Post(
        cpr::Url(std::format("https://api.telegram.org/bot{}/sendDocument", BOT_TOKEN)),
        cpr::Multipart{
            {"chat_id", chat_id},
            {"caption", "dir=work"},
            {
                "document",
                cpr::Buffer {
                    data.begin(),
                    data.end(),
                    "text/plain"
                }
            }
        }
    );

    nlohmann::json json_response = nlohmann::json::parse(response.text);
    std::string file_id = json_response["result"]["document"]["file_id"].get<std::string>();
    long long file_size = json_response["result"]["document"]["file_size"].get<long long>();
    
    return File(file_id, chat_id);
}

std::string FilePath::GetPath(const std::string& file_id) {
    auto now = std::chrono::system_clock::now();
    
    if (now > deadline) {
        auto response = cpr::Get(
            cpr::Url(std::format("https://api.telegram.org/bot{}/getFile", BOT_TOKEN)),
            cpr::Parameters{{"file_id", file_id}}
        );

        nlohmann::json json_response = nlohmann::json::parse(response.text);
        path = json_response["result"]["file_path"].get<std::string>();

        deadline = now + std::chrono::minutes(59);
    }

    return path;
}

}