#define FUSE_USE_VERSION 30

#include <fuse_lowlevel.h>

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include "fat16.h"
#include "fat16_fuse.h"

int main(int argc, char *argv[]) {

    openlog("slog", LOG_PID|LOG_CONS, LOG_USER);

    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    struct fuse_session *se;
    struct fuse_cmdline_opts opts;
    
    if (fuse_parse_cmdline(&args, &opts) != 0) return 1;

    struct fat16_super fat16;
    fat16.inodes = fat16_inodes_init(4096);

    //////////////////////////

    fat16.device = fopen("fs_image.raw", "rb");

    fread(&fat16.boot_sector, sizeof(struct fat16_boot_sector), 1, fat16.device);

    fseek(fat16.device, fat16.boot_sector.reserved_sectors * fat16.boot_sector.sector_size, SEEK_SET);
    fat16.FAT = malloc(fat16.boot_sector.fat_size_sectors * fat16.boot_sector.sector_size);
    fread(fat16.FAT, sizeof(unsigned short), fat16.boot_sector.fat_size_sectors * fat16.boot_sector.sector_size / 2, fat16.device);

    struct fat16_inode *root = malloc(sizeof(struct fat16_inode));
    root->ino = 1;
    root->attributes.is_directory = true;
    root->entry.starting_cluster = 0;

    fat16_inodes_add(fat16.inodes, root);
    //////////////////////////

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
