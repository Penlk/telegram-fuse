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
#include "filesystem.hpp"

namespace telegram_fs {

Filesystem::Filesystem(std::string chat_id)
    : chat_id_(std::move(chat_id)) {
    Node root;
    root.ino = 1;
    root.parent_ino = 1;
    root.nlink = 2;
    root.kind = Node::Kind::Dir;
    root.mode = S_IFDIR | 0755;
    nodes_.emplace(1, std::move(root));
}

void Filesystem::Lookup(fuse_req_t req, fuse_ino_t parent, const char* name) {
    Node* child = FindChild(parent, name);
    if (!child) {
        fuse_reply_err(req, ENOENT);
        return;
    }

    fuse_entry_param e{};
    e.ino = child->ino;
    e.generation = 0;
    e.attr_timeout = 1.0;
    e.entry_timeout = 1.0;
    e.attr = {};

    FillStat(*child, e.attr);
    fuse_reply_entry(req, &e);
}

void Filesystem::GetAttr(fuse_req_t req, fuse_ino_t ino, fuse_file_info *fi) {
    Node* node = GetNodeFromHandleOrIno(ino, fi);
    if (!node) {
        fuse_reply_err(req, ENOENT);
        return;
    }

    struct stat st;
    FillStat(*node, st);
    fuse_reply_attr(req, &st, 1.0);
}

void Filesystem::ReadDir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, fuse_file_info *fi) {
    Node* node = GetNodeFromHandleOrIno(ino, fi);
    if (!node) {
        fuse_reply_err(req, ENOENT);
        return;
    }

    if (node->kind != Node::Kind::Dir) {
        fuse_reply_err(req, ENOTDIR);
        return;
    }

    std::vector<std::pair<std::string, fuse_ino_t>> entries;
    entries.emplace_back(".", node->ino);
    entries.emplace_back("..", node->parent_ino);

    for (const auto& [name, child] : node->children) {
        entries.emplace_back(name, child);
    }

    std::sort(entries.begin(), entries.end());

    std::vector<char> buf(size);
    size_t pos = 0;

    for (size_t i = static_cast<size_t>(off); i < entries.size(); ++i) {
        struct stat st{};
        FillStat(nodes_.at(entries[i].second), st);

        size_t ent_size = fuse_add_direntry(
            req,
            nullptr,
            0,
            entries[i].first.c_str(),
            &st,
            static_cast<off_t>(i + 1)
        );

        if (pos + ent_size > size) break;

        fuse_add_direntry(
            req,
            buf.data() + pos,
            size - pos,
            entries[i].first.c_str(),
            &st,
            static_cast<off_t>(i + 1)
        );

        pos += ent_size;
    }

    fuse_reply_buf(req, buf.data(), pos);
}

void Filesystem::Open(fuse_req_t req, fuse_ino_t ino, fuse_file_info *fi) {
    Node* node = GetNodeFromHandleOrIno(ino, fi);
    if (!node) {
        fuse_reply_err(req, ENOENT);
        return;
    }

    if (node->kind != Node::Kind::File) {
        fuse_reply_err(req, EISDIR);
        return;
    }

    // fi->fh = reinterpret_cast<uint64_t>(node);
    fuse_reply_open(req, fi);
}

void Filesystem::Read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, fuse_file_info *fi) {
    Node* node = GetNodeFromHandleOrIno(ino, fi);
    if (!node) {
        fuse_reply_err(req, ENOENT);
        return;
    }

    if (node->kind != Node::Kind::File) {
        fuse_reply_err(req, EISDIR);
        return;
    }

    if (off >= static_cast<off_t>(node->size)) {
        fuse_reply_buf(req, nullptr, 0);
        return;
    }

    if (off < 0) {
        fuse_reply_err(req, EINVAL);
        return;
    }

    size_t bytes_to_read = std::min(size, node->size - static_cast<size_t>(off));
    fuse_reply_buf(req, node->tg_file.value().GetData().c_str() + off, bytes_to_read);
}

void Filesystem::MakeNode(fuse_req_t req, fuse_ino_t parent, const char *name, mode_t mode, dev_t rdev) {
    Node* p = FindNode(parent);
    if (!p || p->kind != Node::Kind::Dir) {
        fuse_reply_err(req, ENOTDIR);
        return;
    }

    if (p->children.contains(name)) {
        fuse_reply_err(req, EEXIST);
        return;
    }

    Node node;
    node.ino = next_ino_++;
    node.parent_ino = parent;
    node.kind = Node::Kind::File;
    node.mode = mode;
    node.nlink = 1;
    node.size = 0;

    nodes_.emplace(node.ino, node);
    p->children.emplace(name, node.ino);

    struct fuse_entry_param e{};
    struct stat st{};
    FillStat(nodes_.at(node.ino), st);

    e.ino = node.ino;
    e.attr = st;
    e.attr_timeout = 1.0;
    e.entry_timeout = 1.0;

    fuse_reply_entry(req, &e);
}

void Filesystem::Create(fuse_req_t req, fuse_ino_t parent, const char *name, mode_t mode, fuse_file_info *fi) {
    Node* p = FindNode(parent);
    if (!p || p->kind != Node::Kind::Dir) {
        fuse_reply_err(req, ENOTDIR);
        return;
    }

    if (p->children.contains(name)) {
        fuse_reply_err(req, EEXIST);
        return;
    }

    Node node;
    node.ino = next_ino_++;
    node.parent_ino = parent;
    node.kind = Node::Kind::File;
    node.mode = S_IFREG | (mode & 0777);
    node.nlink = 1;
    node.size = 0;
    
    if (parent != 1) {
        node.tg_file = tg::File::CreateFile("empty", chat_id_, name, FindNode(parent)->name);
    } else {
        node.tg_file = tg::File::CreateFile("empty", chat_id_, name);
    }

    auto [it, inserted] = nodes_.emplace(node.ino, std::move(node));
    if (!inserted) {
        fuse_reply_err(req, EIO);
        return;
    }

    p->children.emplace(name, it->first);

    struct fuse_entry_param e{};
    struct stat st{};
    FillStat(it->second, st);

    e.ino = it->second.ino;
    e.attr = st;
    e.attr_timeout = 1.0;
    e.entry_timeout = 1.0;

    fi->fh = it->second.ino;
    fuse_reply_create(req, &e, fi);
}

void Filesystem::MakeDir(fuse_req_t req, fuse_ino_t parent,
                         const char *name, mode_t mode) {
    Node* p = FindNode(parent);
    if (!p || p->kind != Node::Kind::Dir) {
        fuse_reply_err(req, ENOTDIR);
        return;
    }

    if (p->children.contains(name)) {
        fuse_reply_err(req, EEXIST);
        return;
    }

    Node node;
    node.ino = next_ino_++;
    node.parent_ino = parent;
    node.kind = Node::Kind::Dir;
    node.mode = S_IFDIR | (mode & 0777);
    node.nlink = 2;
    node.size = 0;
    node.name = name;

    auto [it, inserted] = nodes_.emplace(node.ino, std::move(node));
    if (!inserted) {
        fuse_reply_err(req, EIO);
        return;
    }

    p->children.emplace(name, it->first);

    struct stat st{};
    FillStat(it->second, st);

    fuse_entry_param e{};
    e.ino = it->second.ino;
    e.attr = st;
    e.attr_timeout = 1.0;
    e.entry_timeout = 1.0;

    fuse_reply_entry(req, &e);
}

void Filesystem::SetAttr(fuse_req_t req, fuse_ino_t ino, struct stat *attr, int to_set, fuse_file_info *fi) {
    Node* node = GetNodeFromHandleOrIno(ino, fi);
    if (!node) {
        fuse_reply_err(req, ENOENT);
        return;
    }

    if (to_set & FUSE_SET_ATTR_SIZE) {
        if (node->kind != Node::Kind::File) {
            fuse_reply_err(req, EISDIR);
            return;
        }
        node->size = static_cast<size_t>(attr->st_size);
    }

    if (to_set & FUSE_SET_ATTR_MODE) {
        if (node->kind == Node::Kind::Dir) {
            node->mode = S_IFDIR | (attr->st_mode & 0777);
        } else {
            node->mode = S_IFREG | (attr->st_mode & 0777);
        }
    }

    struct stat st{};
    FillStat(*node, st);
    fuse_reply_attr(req, &st, 1.0);
}

void Filesystem::Unlink(fuse_req_t req, fuse_ino_t parent, const char *name) {
    Node* p = FindNode(parent);
    if (!p || p->kind != Node::Kind::Dir) {
        fuse_reply_err(req, ENOTDIR);
        return;
    }

    auto it = p->children.find(name);
    if (it == p->children.end()) {
        fuse_reply_err(req, ENOENT);
        return;
    }

    Node* node = FindNode(it->second);
    if (!node || node->kind == Node::Kind::Dir) {
        fuse_reply_err(req, EISDIR);
        return;
    }

    node->nlink--;
    if (node->nlink == 0) {
        nodes_.erase(it->second);
    }

    p->children.erase(it);

    fuse_reply_err(req, 0);
}

void Filesystem::Write(fuse_req_t req, fuse_ino_t ino, const char *buf, size_t size, off_t off, fuse_file_info *fi) {
    Node* node = GetNodeFromHandleOrIno(ino, fi);
    if (!node) {
        fuse_reply_err(req, ENOENT);
        return;
    }

    if (node->kind != Node::Kind::File) {
        fuse_reply_err(req, EISDIR);
        return;
    }

    if (off < 0) {
        fuse_reply_err(req, EINVAL);
        return;
    }

    std::string new_data(buf, size);
    if (off > static_cast<off_t>(node->size)) {
        new_data = std::string(off, '\0') + new_data;
    } else {
        new_data = node->tg_file.value().GetData().substr(0, off) + new_data;
    }

    node->tg_file.value().EditData(new_data);
    node->size = node->tg_file.value().GetData().size();

    fuse_reply_write(req, size);
}

Filesystem::~Filesystem() {
    for (auto& [ino, node] : nodes_) {
        if (node.kind == Node::Kind::File && node.tg_file.has_value()) {
            node.tg_file->Delete();
        }
    }
}

Node* Filesystem::FindNode(fuse_ino_t ino) {
    auto it = nodes_.find(ino);
    if (it == nodes_.end()) return nullptr;
    return &it->second;
}

Node* Filesystem::FindChild(fuse_ino_t parent, std::string_view name) {
    Node* p = FindNode(parent);
    if (!p || p->kind != Node::Kind::Dir) return nullptr;

    if (name == ".") return p;
    if (name == "..") return FindNode(p->parent_ino);

    auto it = p->children.find(std::string(name));
    if (it == p->children.end()) return nullptr;
    return FindNode(it->second);
}

Node* Filesystem::GetNodeFromHandleOrIno(fuse_ino_t ino, fuse_file_info* fi) {
    // if (fi && fi->fh != 0) {
    //     auto* node = reinterpret_cast<Node*>(fi->fh);
    //     if (node) return node;
    // }
    return FindNode(ino);
}

// void Filesystem::EnsureLoaded(Node& node) {
//     if (node.kind != Node::Kind::File) return;
//     if (node.loaded) return;
//     if (!node.remote) return;

//     node.data = node.remote->GetData();
//     node.size = node.data.size();
//     node.loaded = true;
// }

void Filesystem::FillStat(const Node& node, struct stat& st) {
    std::memset(&st, 0, sizeof(st));

    st.st_ino = node.ino;
    st.st_mode = node.mode;
    st.st_nlink = (node.kind == Node::Kind::Dir) ? 2 : 1;
    st.st_uid = getuid();
    st.st_gid = getgid();
    st.st_size = static_cast<off_t>(node.kind == Node::Kind::File ? node.size : 4096);
    st.st_blksize = 4096;
    st.st_blocks = (st.st_size + 511) / 512;

    const auto now = std::time(nullptr);
    st.st_atime = now;
    st.st_mtime = now;
    st.st_ctime = now;
}

}