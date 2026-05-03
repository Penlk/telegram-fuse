#include "cpr/cpr.h"

#include "operations/telegram_ops.hpp"
#include "telegram/file.hpp"

#include <fuse3/fuse_opt.h>

#include <fuse3/fuse_lowlevel.h>
#include <chrono>
#include <iostream>

int main(int argc, char *argv[]) {
    if (argc == 1) {
        std::cerr << "Mountpoint is required." << std::endl;
        exit(EXIT_FAILURE);
    }

    struct fuse_args args = FUSE_ARGS_INIT(0, NULL);
    fuse_opt_add_arg(&args, "telegram-fs");
    fuse_opt_add_arg(&args, "-odefault_permissions");
    fuse_opt_add_arg(&args, "-oauto_unmount");
    fuse_opt_add_arg(&args, "-odebug");
    
    struct fuse_conn_info_opts *opts = fuse_parse_conn_info_opts(&args);
    if (!opts)
        exit(EXIT_FAILURE);

    
    struct fuse_session *se = fuse_session_new(&args, &telegram_ops, sizeof(telegram_ops), NULL);
    if (!se)
        exit(EXIT_FAILURE);
    
    int err = fuse_session_mount(se, argv[1]);
    if (err) 
        exit(EXIT_FAILURE);
    
    fuse_session_loop(se);
}