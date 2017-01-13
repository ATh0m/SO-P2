#include "fat16_fuse.h"

static const char *hello_str = "Hello World!\n";
static const char *hello_name = "hello";

static int hello_stat(fuse_ino_t ino, struct stat *stbuf)
{
        stbuf->st_ino = ino;
        switch (ino) {
        case 1:
                stbuf->st_mode = S_IFDIR | 0755;
                stbuf->st_nlink = 2;
                break;
        case 2:
                stbuf->st_mode = S_IFREG | 0444;
                stbuf->st_nlink = 1;
                stbuf->st_size = strlen(hello_str);
                break;
        default:
                return -1;
        }
        return 0;
}

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

static int reply_buf_limited(fuse_req_t req, const char *buf, size_t bufsize,
                             off_t off, size_t maxsize)
{
        if (off < bufsize)
                return fuse_reply_buf(req, buf + off,
                                      min(bufsize - off, maxsize));
        else
                return fuse_reply_buf(req, NULL, 0);
}



void fat16_fuse_open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {;}

void fat16_fuse_read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi) {;}

void fat16_fuse_release(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {;}

void fat16_fuse_getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) 
{
    struct stat stbuf;
    (void) fi;
    memset(&stbuf, 0, sizeof(stbuf));
    if (hello_stat(ino, &stbuf) == -1)
        fuse_reply_err(req, ENOENT);
    else
        fuse_reply_attr(req, &stbuf, 1.0);
}

void fat16_fuse_lookup(fuse_req_t req, fuse_ino_t parent, const char *name) 
{
    struct fuse_entry_param e;
    if (parent != 1 || strcmp(name, hello_name) != 0)
        fuse_reply_err(req, ENOENT);
    else {
        memset(&e, 0, sizeof(e));
        e.ino = 2;
        e.attr_timeout = 1.0;
        e.entry_timeout = 1.0;
        hello_stat(e.ino, &e.attr);
        fuse_reply_entry(req, &e);
    }
}

void fat16_fuse_opendir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {;}

void fat16_fuse_readdir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi) 
{
    (void) fi;
    if (ino != 1)
        fuse_reply_err(req, ENOTDIR);
    else {
        struct dirbuf b;
        memset(&b, 0, sizeof(b));
        dirbuf_add(req, &b, ".", 1);
        dirbuf_add(req, &b, "..", 1);
        dirbuf_add(req, &b, hello_name, 2);
        reply_buf_limited(req, b.p, b.size, off, size);
        free(b.p);
    }
}

void fat16_fuse_releasedir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {;}

void fat16_fuse_stafs(fuse_req_t req, fuse_ino_t ino) {;}
