#include "fat16_fuse.h"

/* --------------------- Implementacja z example/hello_ll.c ---------------------- */

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

/* -------------------------------------------------------------------------------- */


void fat16_fuse_init(void *userdata, struct fuse_conn_info *conn) 
{
    struct fat16_super *fat16 = (struct fat16_super *)userdata;

    fat16->inodes = fat16_inodes_init(4096);

    fread(&fat16->boot_sector, sizeof(struct fat16_boot_sector), 1, fat16->device);

    fseek(fat16->device, fat16->boot_sector.reserved_sectors * fat16->boot_sector.sector_size, SEEK_SET);
    fat16->FAT = malloc(fat16->boot_sector.fat_size_sectors * fat16->boot_sector.sector_size);
    fread(fat16->FAT, sizeof(unsigned short), fat16->boot_sector.fat_size_sectors * fat16->boot_sector.sector_size / 2, fat16->device);

    struct fat16_inode *root = malloc(sizeof(struct fat16_inode));
    root->ino = 1;
    root->attributes.is_directory = true;
    root->entry.starting_cluster = 0;

    fat16_inodes_add(fat16->inodes, root);
}

void fat16_fuse_destroy(void *userdata)
{
    struct fat16_super *fat16 = (struct fat16_super *)userdata;

    fat16_inodes_del(fat16->inodes);
    fclose(fat16->device);
}

void fat16_fuse_open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) 
{
    struct fat16_super *super = fuse_req_userdata(req);
    struct fat16_inode *inode = fat16_inodes_get(super->inodes, ino);

    if (inode == NULL) {
        fuse_reply_err(req, ENOENT);
        return;
    }

    if (inode->attributes.is_directory) {
        fuse_reply_err(req, EISDIR);
        return;
    }

    if (fi->flags & 3 != O_RDONLY) {
        fuse_reply_err(req, EACCES);
        return;
    }
    
    fuse_reply_open(req, fi);
}

void fat16_fuse_read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi) 
{
    struct fat16_super *super = fuse_req_userdata(req);
    struct fat16_inode *inode = fat16_inodes_get(super->inodes, ino);

    if (inode == NULL) {
        fuse_reply_err(req, ENOENT);
        return;
    }

    char *buffer = malloc(size * sizeof(char));

    fat16_read(super, inode, buffer, size, off);

    reply_buf_limited(req, buffer, size, 0, size);

    free(buffer);
}

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

    fuse_reply_attr(req, stat, 3600.0);
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

    struct fuse_entry_param *entry = calloc(1, sizeof(struct fuse_entry_param));

    entry->attr_timeout = 0;
    entry->entry_timeout = 0;
    entry->ino = inode->ino;
    entry->attr = *fat16_inode_get_stat(inode);

    fuse_reply_entry(req, entry);
}

void fat16_fuse_opendir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) 
{
    fuse_reply_open(req, fi);
}

void fat16_fuse_readdir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off, struct fuse_file_info *fi)
{
    struct fat16_super *super = fuse_req_userdata(req);
    struct fat16_inode *parent_inode = fat16_inodes_get(super->inodes, ino);

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

        char *filename = fat16_format_name(child_inode->entry);
        dirbuf_add(req, b, filename, child_inode->ino);
        free(filename);

        child_inodes = child_inodes->next;
    }

    reply_buf_limited(req, b->p, b->size, off, size);

    free(b->p);
    free(b);
}

void fat16_fuse_releasedir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {;}

void fat16_fuse_stafs(fuse_req_t req, fuse_ino_t ino) {;}

int main(int argc, char *argv[]) {

    struct fuse_args args = FUSE_ARGS_INIT(argc-1, &argv[1]);
    struct fuse_session *se;
    struct fuse_cmdline_opts opts;
    
    if (fuse_parse_cmdline(&args, &opts) != 0) return 1;

    struct fat16_super fat16 = {.device = fopen(argv[1], "rb")};
    se = fuse_session_new(&args, &fat16_fuse_oper, sizeof(fat16_fuse_oper), &fat16);

    int ret = -1;

    if (se != NULL) {
        if (fuse_set_signal_handlers(se) == 0) {
            if (fuse_session_mount(se, opts.mountpoint) == 0) {
                fuse_daemonize(opts.foreground);
                ret = fuse_session_loop(se);
                fuse_session_unmount(se);
            }
            fuse_remove_signal_handlers(se);
        }
        fuse_session_destroy(se);
    }

    free(opts.mountpoint);
    fuse_opt_free_args(&args);

    return ret ? 1 : 0;
}
