#define FUSE_USE_VERSION 30

#include <fuse_lowlevel.h>

#include <stdio.h>
#include <stdlib.h>
#include "fat16.h"
#include "fat16_fuse.h"

int main(int argc, char *argv[]) {

    /*
    // sprawdzić czy podano ścieżkę do urządzenia
    if (argc != 2) return 1;

    // otworzyć urządzenie
    FILE *device = fopen(argv[1], "rb");

    // uzupełnić informacja na temat Boot Sector'a
    struct fat16_boot_sector bs;
    fread(&bs, sizeof(struct fat16_boot_sector), 1, device);

    // przesunąć się do początku FAT
    fseek(device, bs.reserved_sectors * bs.sector_size, SEEK_SET);

    // przeczytać sekcje FAT
    unsigned short *FAT = malloc(bs.fat_size_sectors * bs.sector_size);
    fread(FAT, sizeof(unsigned short), bs.fat_size_sectors * bs.sector_size / 2, device);

    // przesunąć się do miejsca z katalogiem głownym
    fseek(device, (bs.fat_size_sectors * bs.number_of_fats + 1) * bs.sector_size, SEEK_SET);

    // uzyskać informacje na temat wpisów
    struct fat16_entry entry;

    for (int i = 0; i < bs.root_dir_entries; i++) {
        fread(&entry, sizeof(entry), 1, device);
    }

    printf("Root directory end: 0x%08X\n", (int)ftell(device));

    // zamknąć urządzenie
    fclose(device);

    struct fat16_inodes inodes = fat16_inodes_init(FAT16_INODES_CONTAINER_SIZE);

    struct fat16_inode *inode0 = malloc(sizeof(struct fat16_inode));
    inode0->ino = 3;
    fat16_inodes_add(inodes, inode0);

    struct fat16_inode *inode1 = malloc(sizeof(struct fat16_inode));
    inode1->ino = 4099;
    fat16_inodes_add(inodes, inode1);

    printf("%p %p\n", inode0, fat16_inodes_get(inodes, 3));
    printf("%p %p\n", inode1, fat16_inodes_get(inodes, 4099));

    fat16_inodes_del(inodes);

    */

    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    struct fuse_session *se;
    struct fuse_cmdline_opts opts;
    int ret = -1;
    if (fuse_parse_cmdline(&args, &opts) != 0)
        return 1;
    if (opts.show_help) {
        printf("usage: %s [options] <mountpoint>\n\n", argv[0]);
        fuse_cmdline_help();
        fuse_lowlevel_help();
        ret = 0;
        goto err_out1;
    } else if (opts.show_version) {
        printf("FUSE library version %s\n", fuse_pkgversion());
        fuse_lowlevel_version();
        ret = 0;
        goto err_out1;
    }
    se = fuse_session_new(&args, &fat16_fuse_oper, sizeof(fat16_fuse_oper), NULL);
    if (se == NULL)
        goto err_out1;
    if (fuse_set_signal_handlers(se) != 0)
        goto err_out2;
    if (fuse_session_mount(se, opts.mountpoint) != 0)
        goto err_out3;
    fuse_daemonize(opts.foreground);
    
    if (opts.singlethread)
        ret = fuse_session_loop(se);
    else
        ret = fuse_session_loop_mt(se, opts.clone_fd);
    fuse_session_unmount(se);
err_out3:
    fuse_remove_signal_handlers(se);
err_out2:
    fuse_session_destroy(se);
err_out1:
    free(opts.mountpoint);
    fuse_opt_free_args(&args);
    
    return ret ? 1 : 0;

    return 0;
}
