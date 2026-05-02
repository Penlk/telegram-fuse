#include "telegram-ops.hpp"

void my_getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
}

fuse_lowlevel_ops telegram_ops = {
    .getattr = my_getattr,
};
