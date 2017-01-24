#define FUSE_USE_VERSION 30

#include <fuse_lowlevel.h>

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include "fat16.h"
#include "fat16_fuse.h"

int main(int argc, char *argv[]) {

    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    struct fuse_session *se;
    struct fuse_cmdline_opts opts;
    
    if (fuse_parse_cmdline(&args, &opts) != 0) return 1;

    struct fat16_super fat16 = {.device = fopen("fs_image.raw", "rb")};
    se = fuse_session_new(&args, &fat16_fuse_oper, sizeof(fat16_fuse_oper), &fat16);

    if (se == NULL) printf("error 1\n");
    if (fuse_set_signal_handlers(se) != 0) printf("error 2\n");
    if (fuse_session_mount(se, opts.mountpoint) != 0) printf("error 3\n");
    
    fuse_daemonize(opts.foreground);
    
    int ret = -1;
    
    if (opts.singlethread) ret = fuse_session_loop(se);
    else ret = fuse_session_loop_mt(se, opts.clone_fd);

    fuse_session_unmount(se);
    fuse_remove_signal_handlers(se);
    fuse_session_destroy(se);
    
    fat16_inodes_del(fat16.inodes);
    free(opts.mountpoint);
    fclose(fat16.device);

    fuse_opt_free_args(&args);

    closelog();

    return ret ? 1 : 0;
}
