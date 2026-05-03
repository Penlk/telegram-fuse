#include <format>
#include <nlohmann/json.hpp>
#include "file.hpp"
#include "cpr/cpr.h"

namespace tg {

namespace {
    static std::string GetBotToken() {
        const char* token = std::getenv("BOT_TOKEN");
        if (!token) {
            throw std::runtime_error("BOT_TOKEN environment variable is not set");
        }
    
        return token;
    }
    
    static const std::string BOT_TOKEN = GetBotToken();
}


File::File(std::string file_id, std::string chat_id, long long message_id, std::string name_file, std::optional<std::string> tag) : 
    dirty(false),
    deleted(false),
    data(std::nullopt),
    file_id(std::move(file_id)), 
    chat_id(std::move(chat_id)),
    name_file(std::move(name_file)),
    tag(tag),
    message_id(message_id) {}

std::string& File::GetData() {
    if (dirty) {
        Delete();
        File edited_file = CreateFile(data.value_or(""), chat_id, name_file, tag);
        swap(edited_file);
    }

    if (!data.has_value()) {
        auto response = cpr::Get(
            cpr::Url(std::format("https://api.telegram.org/file/bot{}/{}", BOT_TOKEN, file_path.GetPath(file_id)))
        );

        data = response.text;
    }

    return data.value();
}

void File::EditData(const std::string& new_data) {
    data = new_data;
    dirty = true;
}

void File::swap(File &other) noexcept {
    std::swap(file_id, other.file_id);
    std::swap(chat_id, other.chat_id);
    std::swap(name_file, other.name_file);
    std::swap(data, other.data);
    std::swap(message_id, other.message_id);
    std::swap(file_path, other.file_path);
    std::swap(tag, other.tag);
    std::swap(dirty, other.dirty);
    std::swap(deleted, other.deleted);
}

void File::Delete() {
    if (deleted) return;
    deleted = true;
    cpr::Post(
        cpr::Url(std::format("https://api.telegram.org/bot{}/deleteMessage", BOT_TOKEN)),
        cpr::Payload {
            {"chat_id", chat_id},
            {"message_id", std::to_string(message_id)}
        }
    );
}

File File::CreateFile(const std::string &data, const std::string& chat_id, 
    const std::string& name_file, std::optional<std::string> tag) {

    auto response = cpr::Post(
        cpr::Url(std::format("https://api.telegram.org/bot{}/sendDocument", BOT_TOKEN)),
        cpr::Multipart{
            {"chat_id", chat_id},
            {"caption", tag.has_value() ? std::format("#{}", tag.value()) : ""},
            {
                "document",
                cpr::Buffer {
                    data.begin(),
                    data.end(),
                    "text/" + name_file
                }
            }
        }
    );

    nlohmann::json json_response = nlohmann::json::parse(response.text);
    std::string file_id = json_response["result"]["document"]["file_id"].get<std::string>();
    long long message_id = json_response["result"]["message_id"].get<long long>();

    return File(file_id, chat_id, message_id, name_file, tag);
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