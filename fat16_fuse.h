#ifndef FAT16_FUSE_H
#define FAT16_FUSE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <assert.h>

#define FUSE_USE_VERSION 30

#include <fuse_lowlevel.h>

void fat16_fuse_init(void *userdata, struct fuse_conn_info *conn);

void fat16_fuse_destroy(void *userdata);

void fat16_fuse_open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi);

void fat16_fuse_read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi);

void fat16_fuse_release(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi);

void fat16_fuse_getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi);

void fat16_fuse_lookup(fuse_req_t req, fuse_ino_t parent, const char *name);

void fat16_fuse_opendir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi);

void fat16_fuse_readdir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi);

void fat16_fuse_releasedir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi);

void fat16_fuse_stafs(fuse_req_t req, fuse_ino_t ino);

static struct fuse_lowlevel_ops fat16_fuse_oper = {
    .init           = fat16_fuse_init,
    .destroy        = fat16_fuse_destroy,
    .open           = fat16_fuse_open,
    .read           = fat16_fuse_read,
    .release        = fat16_fuse_release,
    .getattr        = fat16_fuse_getattr,
    .lookup         = fat16_fuse_lookup,
    .opendir        = fat16_fuse_opendir,
    .readdir        = fat16_fuse_readdir,
    .releasedir     = fat16_fuse_releasedir,
//    .stafs          = fat16_fuse_stafs,
};

#endif
