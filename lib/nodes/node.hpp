#pragma once

#define FUSE_USE_VERSION 34 

#include <fuse3/fuse_lowlevel.h>
#include <string>

namespace telegram_fs {

class Node {
public:
    virtual ~Node() = default;
private:
    fuse_ino_t inode;
    std::string chat_id;
};

}