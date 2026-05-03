#pragma once

#define FUSE_USE_VERSION 34

#include <fuse3/fuse_lowlevel.h>

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "telegram/file.hpp"
#include "node.hpp"

namespace telegram_fs {

class Filesystem {
public:
    explicit Filesystem(std::string chat_id);

    void Lookup(fuse_req_t req, fuse_ino_t parent, const char* name);

    void GetAttr(fuse_req_t req, fuse_ino_t ino, fuse_file_info* fi);

    void ReadDir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, fuse_file_info* fi);

    void Open(fuse_req_t req, fuse_ino_t ino, fuse_file_info* fi);

    void Read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, fuse_file_info* fi);

    void MakeNode(fuse_req_t req, fuse_ino_t parent, const char *name, mode_t mode, dev_t rdev);

    void Create(fuse_req_t req, fuse_ino_t parent, const char* name, mode_t mode, fuse_file_info* fi);

    void MakeDir(fuse_req_t req, fuse_ino_t parent, const char *name, mode_t mode);

    void SetAttr(fuse_req_t req, fuse_ino_t ino, struct stat *attr, int to_set, struct fuse_file_info *fi);

    void Unlink(fuse_req_t req, fuse_ino_t parent, const char* name);

    void Write(fuse_req_t req, fuse_ino_t ino, const char* buf, size_t size, off_t off, fuse_file_info* fi);

    ~Filesystem();
private:
    Node* FindNode(fuse_ino_t ino);

    Node* FindChild(fuse_ino_t parent, std::string_view name);

    Node* GetNodeFromHandleOrIno(fuse_ino_t ino, fuse_file_info* fi);

    void EnsureLoaded(Node& node);

    void FillStat(const Node& node, struct stat& st);

private:
    std::string chat_id_;
    std::unordered_map<fuse_ino_t, Node> nodes_;
    fuse_ino_t next_ino_ = 2;
};

}