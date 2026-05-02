add_library(telegram file.cpp)
target_link_libraries(telegram PRIVATE cpr::cpr nlohmann_json::nlohmann_json)