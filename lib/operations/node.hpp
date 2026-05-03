#pragma once

#define FUSE_USE_VERSION 34 

#include <fuse3/fuse_lowlevel.h>
#include <string>
#include <unordered_map>
#include "telegram/file.hpp"

namespace telegram_fs {

struct Node {
    fuse_ino_t ino;
    fuse_ino_t parent_ino;
    std::string name;
    enum class Kind { File, Dir } kind;
    mode_t mode;

    size_t size = 0;
    size_t nlink = 0;
    std::unordered_map<std::string, fuse_ino_t> children;
    std::optional<std::string> chat_id;
    std::optional<tg::File> tg_file;
};

}