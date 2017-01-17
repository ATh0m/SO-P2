#include "fat16_fuse.h"
#include "fat16.h"

struct dirbuf {
    char *p;
    size_t size;
};

static void dirbuf_add(fuse_req_t req, struct dirbuf *b, const char *name, fuse_ino_t ino)
{
    struct stat stbuf;
    size_t oldsize = b->size;
    b->size += fuse_add_direntry(req, NULL, 0, name, NULL, 0);
    b->p = (char *) realloc(b->p, b->size);
    memset(&stbuf, 0, sizeof(stbuf));
    stbuf.st_ino = ino;
    fuse_add_direntry(req, b->p + oldsize, b->size - oldsize, name, &stbuf, b->size);
}

#define min(x, y) ((x) < (y) ? (x) : (y))

static int reply_buf_limited(fuse_req_t req, const char *buf, size_t bufsize, off_t off, size_t maxsize)
{
    if (off < bufsize)
        return fuse_reply_buf(req, buf + off, min(bufsize - off, maxsize));
    else
        return fuse_reply_buf(req, NULL, 0);
}



void fat16_fuse_open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {;}

void fat16_fuse_read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi) {;}

void fat16_fuse_release(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {;}

void fat16_fuse_getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) 
{
    struct fat16_super *super = fuse_req_userdata(req);
    struct fat16_inode *inode = fat16_inodes_get(super->inodes, ino);

    if (inode == NULL) {
        fuse_reply_err(req, ENOENT);
        return;
    }

    struct stat *stat = fat16_inode_get_stat(inode);

    syslog(LOG_INFO, "getattr %lu %s %s", ino, inode->entry.filename, inode->entry.ext);

    fuse_reply_attr(req, stat, 0);
}

void fat16_fuse_lookup(fuse_req_t req, fuse_ino_t parent, const char *name) 
{
    struct fat16_super *super = fuse_req_userdata(req);
    struct fat16_inode *parent_inode = fat16_inodes_get(super->inodes, parent);

    if (!parent_inode->attributes.is_directory) {
        fuse_reply_err(req, ENOENT);
        return;
    }

    struct fat16_inode *inode = fat16_lookup(super, parent_inode, name);

    if (!inode) {
        fuse_reply_err(req, ENOENT);
        return;
    }

    syslog(LOG_INFO, "lookup %lu", parent);

    struct fuse_entry_param *entry = calloc(1, sizeof(struct fuse_entry_param));

    entry->attr_timeout = 0;
    entry->entry_timeout = 0;
    entry->ino = inode->ino;
    entry->attr = *fat16_inode_get_stat(inode);

    fuse_reply_entry(req, entry);
}

void fat16_fuse_opendir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {;}

void fat16_fuse_readdir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi) 
{
    struct fat16_super *super = fuse_req_userdata(req);
    struct fat16_inode *parent_inode = fat16_inodes_get(super->inodes, ino);
    
    syslog(LOG_INFO, "readdir %lu", ino);

    if (!parent_inode->attributes.is_directory) {
        fuse_reply_err(req, ENOTDIR);
        return;
    }

    struct dirbuf *b = calloc(1, sizeof(struct dirbuf));
    struct fat16_inode *child_inode;

    // dirbuf_add(req, b, ".", ino);
    // dirbuf_add(req, b, "..", ino);

    struct fat16_inode_node *child_inodes = fat16_readdir(super, parent_inode);

    while (child_inodes) {
        child_inode = child_inodes->inode;
        dirbuf_add(req, b, child_inode->entry.filename, child_inode->ino);

        child_inodes = child_inodes->next;
    }

    reply_buf_limited(req, b->p, b->size, off, size);
    
    free(b->p);
    free(b);
}

void fat16_fuse_releasedir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {;}

void fat16_fuse_stafs(fuse_req_t req, fuse_ino_t ino) {;}
