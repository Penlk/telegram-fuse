#include "telegram_ops.hpp"

#include "filesystem.hpp"

static std::string GetChatId() {
    const char* chat_id = std::getenv("CHAT_ID");
    if (!chat_id) {
        throw std::runtime_error("CHAT_ID environment variable is not set");
    }

    return chat_id;
}
static const std::string CHAT_ID = GetChatId();

static telegram_fs::Filesystem* g_fs = new telegram_fs::Filesystem(CHAT_ID);

static void ll_init(void* userdata, fuse_conn_info* conn) {
    (void)userdata;
    (void)conn;
}

static void ll_destroy(void* userdata) {
    (void)userdata;
}

// static void ll_forget(fuse_req_t req, fuse_ino_t ino, uint64_t nlookup) {
//     if (g_fs) g_fs->Forget(req, ino, nlookup);
//     fuse_reply_err(req, 0);
// }

static void ll_mknod(fuse_req_t req, fuse_ino_t parent, const char *name, mode_t mode, dev_t rdev) {
    if (!g_fs) { fuse_reply_err(req, EIO); return; }
    g_fs->MakeNode(req, parent, name, mode, rdev);
}

static void ll_getattr(fuse_req_t req, fuse_ino_t ino, fuse_file_info* fi) {
    if (!g_fs) { fuse_reply_err(req, EIO); return; }
    g_fs->GetAttr(req, ino, fi);
}

static void ll_lookup(fuse_req_t req, fuse_ino_t parent, const char* name) {
    if (!g_fs) { fuse_reply_err(req, EIO); return; }
    g_fs->Lookup(req, parent, name);
}

// static void ll_opendir(fuse_req_t req, fuse_ino_t ino, fuse_file_info* fi) {
//     if (!g_fs) { fuse_reply_err(req, EIO); return; }
//     g_fs->OpenDir(req, ino, fi);
// }

static void ll_readdir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, fuse_file_info* fi) {
    if (!g_fs) { fuse_reply_err(req, EIO); return; }
    g_fs->ReadDir(req, ino, size, off, fi);
}

static void ll_open(fuse_req_t req, fuse_ino_t ino, fuse_file_info* fi) {
    if (!g_fs) { fuse_reply_err(req, EIO); return; }
    g_fs->Open(req, ino, fi);
}

static void ll_read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, fuse_file_info* fi) {
    if (!g_fs) { fuse_reply_err(req, EIO); return; }
    g_fs->Read(req, ino, size, off, fi);
}

static void ll_write(fuse_req_t req, fuse_ino_t ino, const char* buf, size_t size, off_t off, fuse_file_info* fi) {
    if (!g_fs) { fuse_reply_err(req, EIO); return; }
    g_fs->Write(req, ino, buf, size, off, fi);
}

static void ll_create(fuse_req_t req, fuse_ino_t parent, const char* name, mode_t mode, fuse_file_info* fi) {
    if (!g_fs) { fuse_reply_err(req, EIO); return; }
    g_fs->Create(req, parent, name, mode, fi);
}

static void ll_setattr(fuse_req_t req, fuse_ino_t ino, struct stat *attr, int to_set, fuse_file_info *fi) {
    if (!g_fs) { fuse_reply_err(req, EIO); return; }
    g_fs->SetAttr(req, ino, attr, to_set, fi);
}

static void ll_unlink(fuse_req_t req, fuse_ino_t parent, const char* name) {
    if (!g_fs) { fuse_reply_err(req, EIO); return; }
    g_fs->Unlink(req, parent, name);
}

static void ll_mkdir(fuse_req_t req, fuse_ino_t parent, const char* name, mode_t mode) {
    if (!g_fs) { fuse_reply_err(req, EIO); return; }
    g_fs->MakeDir(req, parent, name, mode);
}

// static void ll_rmdir(fuse_req_t req, fuse_ino_t parent, const char* name) {
//     if (!g_fs) { fuse_reply_err(req, EIO); return; }
//     g_fs->Rmdir(req, parent, name);
// }

// static void ll_rename(fuse_req_t req, fuse_ino_t parent, const char* name,
//                       fuse_ino_t newparent, const char* newname, unsigned int flags) {
//     if (!g_fs) { fuse_reply_err(req, EIO); return; }
//     g_fs->Rename(req, parent, name, newparent, newname, flags);
// }

// static void ll_release(fuse_req_t req, fuse_ino_t ino, fuse_file_info* fi) {
//     (void)ino;
//     (void)fi;
//     if (!g_fs) { fuse_reply_err(req, EIO); return; }
//     g_fs->Release(req, ino, fi);
// }

// static void ll_releasedir(fuse_req_t req, fuse_ino_t ino, fuse_file_info* fi) {
//     (void)ino;
//     (void)fi;
//     if (!g_fs) { fuse_reply_err(req, EIO); return; }
//     g_fs->ReleaseDir(req, ino, fi);
// }

fuse_lowlevel_ops BuildOps() {
    struct fuse_lowlevel_ops ops {};
    ops.init = ll_init;
    ops.destroy = ll_destroy;
    // ops.forget = ll_forget;
    ops.getattr = ll_getattr;
    ops.lookup = ll_lookup;
    // ops.opendir = ll_opendir;
    ops.readdir = ll_readdir;
    ops.open = ll_open;
    ops.read = ll_read;
    ops.mknod = ll_mknod;
    ops.setattr = ll_setattr;
    ops.write = ll_write;
    ops.create = ll_create;
    ops.unlink = ll_unlink;
    ops.mkdir = ll_mkdir;
    // ops.rmdir = ll_rmdir;
    // ops.rename = ll_rename;
    // ops.release = ll_release;
    // ops.releasedir = ll_releasedir;
    return ops;
}

fuse_lowlevel_ops telegram_ops = BuildOps();