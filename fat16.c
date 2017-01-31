
/* Tomasz Nanowski (tomasz.nanowski@gmail.com) */

#include "fat16.h"

struct fat16_attributes convert_attributes(char fat16_raw_attributes)
{
    struct fat16_attributes attributes;

    attributes.is_read_only     = fat16_raw_attributes & 0x01;  // tylko do odczytu
    attributes.is_hidden        = fat16_raw_attributes & 0x02;  // ukryty
    attributes.is_system_file   = fat16_raw_attributes & 0x04;  // plik systemowy
    attributes.is_volume_label  = fat16_raw_attributes & 0x08;  // etykieta wolumenu
    attributes.is_directory     = fat16_raw_attributes & 0x10;  // katalog
    attributes.is_archive       = fat16_raw_attributes & 0x20;  // archiwum

    return attributes;
}

struct tm convert_time(unsigned short fat16_date, unsigned short fat16_time)
{
    struct tm time_;

    time_.tm_year   = (fat16_date >> 9) + 80;           // rok (+80 bo fat16 liczy datę od 1980, a tm od 1900)
    time_.tm_mon    = ((fat16_date >> 5) & 0xF) - 1;    // miesiąc (-1 bo fat16 trzyma miesiąc 1-12, a tm 0-11)
    time_.tm_mday   = fat16_date & 0x1F;                // dzień

    time_.tm_hour   = fat16_time >> 11;                 // godzina
    time_.tm_min    = (fat16_time >> 5) & 0x3F;         // minuta
    time_.tm_sec    = (fat16_time & 0x1F) * 2;          // sekunda

    mktime(&time_);
    return time_;
}

/* Definicja systemowej struktury stat (Ubuntu 16.10)
 *
 * struct stat {
 *     dev_t     st_dev;         // ID of device containing file
 *     ino_t     st_ino;         // inode number
 *     mode_t    st_mode;        // file type and mode
 *     nlink_t   st_nlink;       // number of hard links
 *     uid_t     st_uid;         // user ID of owner
 *     gid_t     st_gid;         // group ID of owner
 *     dev_t     st_rdev;        // device ID (if special file)
 *     off_t     st_size;        // total size, in bytes
 *     blksize_t st_blksize;     // blocksize for filesystem I/O
 *     blkcnt_t  st_blocks;      // number of 512B blocks allocated
 *
 *     // Since Linux 2.6, the kernel supports nanosecond
 *        precision for the following timestamp fields.
 *        For the details before Linux 2.6, see NOTES. //
 *
 *     struct timespec st_atim;  // time of last access
 *     struct timespec st_mtim;  // time of last modification
 *     struct timespec st_ctim;  // time of last status change
 *
 *     #define st_atime st_atim.tv_sec      // Backward compatibility
 *     #define st_mtime st_mtim.tv_sec
 *     #define st_ctime st_ctim.tv_sec
 * };
 */

struct stat * fat16_inode_get_stat(struct fat16_super *super, struct fat16_inode *inode)
{
    struct stat *stat = calloc(1, sizeof(struct stat));

    stat->st_ino = inode->ino;

    struct tm mtime = convert_time(inode->entry.modify_date, inode->entry.modify_time);
    struct timespec ts = {.tv_sec = mktime(&mtime)};

    stat->st_atim = ts;
    stat->st_mtim = ts;
    stat->st_ctim = ts;

    size_t sector_size = super->boot_sector.sectors_per_cluster * super->boot_sector.sector_size;
    stat->st_blksize = sector_size;
    stat->st_size = inode->entry.file_size;

    stat->st_uid = super->uid;
    stat->st_gid = super->gid;
        
    if (inode->attributes.is_directory) {
        stat->st_mode = S_IFDIR | 0755;     // Ustawienie uprawnień dr-xr-xr-x
        stat->st_nlink = 2;

        struct fat16_inode_node *tmp, *child_inodes = fat16_readdir(super, inode);
        struct fat16_inode *child_inode;

        while (child_inodes) {
            child_inode = child_inodes->inode;

            if (child_inode->attributes.is_directory) stat->st_nlink++;

            tmp = child_inodes;
            child_inodes = child_inodes->next;

            free(tmp);
        }

    } else {
        stat->st_mode = S_IFREG | 0444;     // Ustawienie uprawnień -r--r--r--
        stat->st_nlink = 1;
        stat->st_blocks = (inode->entry.file_size + sector_size / 2) / sector_size;
    }

    return stat;
}

struct fat16_inodes fat16_inodes_init(size_t size)
{
    struct fat16_inodes inodes = {
        .size = size,
        .container = malloc(sizeof(struct fat16_inode_node *) * size),
        .amount = 0,
    };

    return inodes;
}

void fat16_inodes_del(struct fat16_inodes inodes)
{
    struct fat16_inode_node *node;
    struct fat16_inode_node *next_node;

    for (int hash_val = 0; hash_val < (int) inodes.size; hash_val++) {
        node = inodes.container[hash_val];

        while(node) {
            next_node = node->next;
            
            free(node->inode);
            free(node);

            node = next_node;
        }
    }

    free(inodes.container);
    inodes.container = NULL;
}

static uint64_t fat16_ino_hash(struct fat16_inodes inodes, uint64_t ino)
{
    return (uint64_t) ino % inodes.size;
}

void fat16_inodes_add(struct fat16_inodes inodes, struct fat16_inode *inode)
{
    uint64_t hash = fat16_ino_hash(inodes, inode->ino);

    struct fat16_inode_node *node = malloc(sizeof(struct fat16_inode_node));
    node->inode = inode;

    node->next = inodes.container[hash];
    inodes.container[hash] = node;

    inodes.amount++;
}

struct fat16_inode * fat16_inodes_get(struct fat16_inodes inodes, uint64_t ino)
{
    uint64_t hash = fat16_ino_hash(inodes, ino);

    struct fat16_inode_node *node = inodes.container[hash];

    while (node) {
        if (node->inode->ino == ino) return node->inode;
        node = node->next;
    }

    return NULL;
}

struct fat16_inode * fat16_inodes_find(struct fat16_inodes inodes, uint64_t ino, struct fat16_entry entry)
{
    struct fat16_inode *inode = fat16_inodes_get(inodes, ino);

    if (!inode) {
        inode = malloc(sizeof(struct fat16_inode));
        inode->ino = ino;

        inode->entry = entry;
        inode->attributes = convert_attributes(entry.attributes);

        fat16_inodes_add(inodes, inode);
    }

    return inode;
}

void __set_device_position_on_cluster(struct fat16_super *super, int cluster_number)
{
    struct fat16_boot_sector bs = super->boot_sector;

    long root_directory_region_start = (bs.reserved_sectors + bs.number_of_fats * bs.fat_size_sectors) * bs.sector_size;
    long data_region_start = root_directory_region_start + bs.root_dir_entries * sizeof(struct fat16_entry);

    if (cluster_number == 0) {
        fseek(super->device, root_directory_region_start, SEEK_SET);
        return;
    }
    
    fseek(super->device, data_region_start + (cluster_number - 2) * bs.sectors_per_cluster * bs.sector_size, SEEK_SET);
}

struct fat16_inode * fat16_lookup(struct fat16_super *super, struct fat16_inode *parent, const char *name)
{
    struct fat16_boot_sector bs = super->boot_sector;
    struct fat16_entry entry;

    int entries_amount = bs.sectors_per_cluster * bs.sector_size / sizeof(struct fat16_entry);
    if (parent->ino == 1) entries_amount = super->boot_sector.root_dir_entries;

    unsigned short cluster = parent->entry.starting_cluster;

    while (cluster != 0xFFFF) {
        
        __set_device_position_on_cluster(super, cluster);

        for (int i = 0; i < entries_amount; i++) {
            uint64_t ino = ftell(super->device);
            fread(&entry, sizeof(struct fat16_entry), 1, super->device);

            char *filename = fat16_format_name(entry);
            if (strcmp(name, filename) == 0) {
                free(filename);
                return fat16_inodes_get(super->inodes, ino);
            }
            free(filename);
        }

        if (parent->ino == 1) break;
        cluster = super->FAT[cluster];
    }

    return NULL;
}

struct fat16_inode_node * fat16_readdir(struct fat16_super *super, struct fat16_inode *parent)
{
    struct fat16_boot_sector bs = super->boot_sector;
    struct fat16_entry entry;

    int entries_amount = bs.sectors_per_cluster * bs.sector_size / sizeof(struct fat16_entry);
    if (parent->ino == 1) entries_amount = super->boot_sector.root_dir_entries;

    unsigned short cluster = parent->entry.starting_cluster;

    struct fat16_inode_node *child_inodes = NULL;
    struct fat16_inode *child_inode;

    while (cluster != 0xFFFF) {

        __set_device_position_on_cluster(super, cluster);

        for (int i = 0; i < entries_amount; i++) {
            uint64_t ino = ftell(super->device);
            fread(&entry, sizeof(struct fat16_entry), 1, super->device);

            if (entry.filename[0] == 0x00) continue;
            if (entry.filename[0] == 0xE5) continue;
            if (convert_attributes(entry.attributes).is_volume_label) continue;

            child_inode = fat16_inodes_find(super->inodes, ino, entry);

            if (child_inode) {
                struct fat16_inode_node *tmp = malloc(sizeof(struct fat16_inode_node));
                tmp->inode = child_inode;
                tmp->next = child_inodes;

                child_inodes = tmp;
            }
        }

        if (parent->ino == 1) break;
        cluster = super->FAT[cluster];
    }

    return child_inodes;
}

char * fat16_format_name(struct fat16_entry entry)
{
    char *name = calloc(20, sizeof(char));
    struct fat16_attributes attributes = convert_attributes(entry.attributes);

    if (attributes.is_hidden) strcat(name, ".");

    int i = 0; while (i < 8 && entry.filename[i] != ' ') i++;

    strncat(name, (char *) entry.filename, i);

    if (!attributes.is_directory) {
        i = 0; while (i < 3 && entry.ext[i] != ' ') i++;

        if (i != 0) strcat(name, ".");
        strncat(name, (char *) entry.ext, i);
    }

    for(i = 0; name[i]; i++) name[i] = tolower(name[i]);

    return name;
}

#define min(x, y) ((x) < (y) ? (x) : (y))

void fat16_read(struct fat16_super *super, struct fat16_inode *inode, char *buffer, size_t size, off_t off)
{
    unsigned short cluster = inode->entry.starting_cluster;
    if (cluster == 0xFFFF) return;

    struct fat16_boot_sector bs = super->boot_sector;

    for (int i = 0; i < off / (bs.sectors_per_cluster * bs.sector_size); i++) cluster = super->FAT[cluster];

    __set_device_position_on_cluster(super, cluster);

    size_t _off = off % (bs.sectors_per_cluster * bs.sector_size);
    fseek(super->device, _off, SEEK_CUR);

    size_t _size = min((bs.sectors_per_cluster * bs.sector_size) - _off, size);
    fread(buffer, _size, 1, super->device);

    while (_size < size) {
        cluster = super->FAT[cluster];
        if (cluster == 0xFFFF) return;

        __set_device_position_on_cluster(super, cluster);

        fread(buffer + _size, min(bs.sectors_per_cluster * bs.sector_size, size - _size), 1, super->device);
        _size += min(bs.sectors_per_cluster * bs.sector_size, size - _size);
    }
}
