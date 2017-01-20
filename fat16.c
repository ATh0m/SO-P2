#include "fat16.h"

struct fat16_attributes convert_attributes(char fat16_raw_attributes)
{
    struct fat16_attributes attributes;

    attributes.is_read_only     = fat16_raw_attributes & 0x01;
    attributes.is_hidden        = fat16_raw_attributes & 0x02;
    attributes.is_system_file   = fat16_raw_attributes & 0x04;
    attributes.is_volume_label  = fat16_raw_attributes & 0x08;
    attributes.is_directory     = fat16_raw_attributes & 0x10;
    attributes.is_archive       = fat16_raw_attributes & 0x20;

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
    time_.tm_sec    = fat16_time & 0x1F;                // sekunda

    mktime(&time_);
    return time_;
}

struct stat * fat16_inode_get_stat(struct fat16_inode *inode)
{
    struct stat *stat = calloc(1, sizeof(struct stat));

    stat->st_ino = inode->ino;

    struct tm mtime = convert_time(inode->entry.modify_date, inode->entry.modify_time);
    struct timespec ts = {.tv_sec = mktime(&mtime)};

    stat->st_atim = ts;
    stat->st_mtim = ts;
    stat->st_ctim = ts;

    if (inode->attributes.is_directory) {
        stat->st_mode = S_IFDIR | 0755;
        stat->st_nlink = 2;
    } else {
        stat->st_mode = S_IFREG | 0444;
        stat->st_nlink = 1;
        stat->st_size = inode->entry.file_size;
    }

    return stat;
}

struct fat16_inodes fat16_inodes_init(size_t size)
{
    struct fat16_inodes inodes = {
        .size = size,
        .container = malloc(sizeof(struct fat16_inode *) * size),
    };

    return inodes;
}

void fat16_inodes_del(struct fat16_inodes inodes)
{
    struct fat16_inode *inode;
    struct fat16_inode *next_inode;

    for (int hash_val = 0; hash_val < (int) inodes.size; hash_val++) {
        inode = inodes.container[hash_val];

        while(inode) {
            next_inode = inode->next;
            // Wyczyścić tutaj dynamiczną zawartość inode
            free(inode);

            inode = next_inode;
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

    inode->next = inodes.container[hash];
    inodes.container[hash] = inode;
}

struct fat16_inode * fat16_inodes_get(struct fat16_inodes inodes, uint64_t ino)
{
    uint64_t hash = fat16_ino_hash(inodes, ino);

    struct fat16_inode *inode = inodes.container[hash];

    while (inode) {
        if (inode->ino == ino) return inode;
        inode = inode->next;
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

void __set_device_position_on_entry(struct fat16_super *super, struct fat16_inode *inode)
{
    struct fat16_boot_sector bs = super->boot_sector;

    long root_directory_region_start = (bs.reserved_sectors + bs.number_of_fats * bs.fat_size_sectors) * bs.sector_size;
    long data_region_start = root_directory_region_start + bs.root_dir_entries * sizeof(struct fat16_entry);

    if (inode->ino == 1) {
        fseek(super->device, root_directory_region_start, SEEK_SET);
    } else {
        fseek(super->device, data_region_start + (inode->entry.starting_cluster - 2) * bs.sectors_per_cluster * bs.sector_size, SEEK_SET);
    }
}

struct fat16_inode * fat16_lookup(struct fat16_super *super, struct fat16_inode *parent, const char *name)
{
    struct fat16_boot_sector bs = super->boot_sector;

    __set_device_position_on_entry(super, parent);

    struct fat16_entry entry;

    int entries_amount = bs.sectors_per_cluster * bs.sector_size / sizeof(struct fat16_entry);

    for (int i = 0; i < entries_amount; i++) {
        uint64_t ino = ftell(super->device);
        fread(&entry, sizeof(struct fat16_entry), 1, super->device);

        char *filename = fat16_format_name(entry);
        // syslog(LOG_INFO, "%s %s", name, filename);
        if (strcmp(name, filename) == 0) {
            free(filename);
            return fat16_inodes_get(super->inodes, ino);
        }
        free(filename);
    }

    return NULL;
}

struct fat16_inode_node * fat16_readdir(struct fat16_super *super, struct fat16_inode *parent)
{
    struct fat16_boot_sector bs = super->boot_sector;

    __set_device_position_on_entry(super, parent);

    struct fat16_entry entry;

    int entries_amount = bs.sectors_per_cluster * bs.sector_size / sizeof(struct fat16_entry);

    struct fat16_inode_node *child_inodes = NULL;
    struct fat16_inode *child_inode;

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

    return child_inodes;
}

char * fat16_format_name(struct fat16_entry entry)
{
    char *name = calloc(20, sizeof(char));
    struct fat16_attributes attributes = convert_attributes(entry.attributes);

    if (attributes.is_hidden)
        strcat(name, ".");

    int i = 0;
    for (i; i < 8; i++)
        if (entry.filename[i] == ' ') break;

    strncat(name, entry.filename, i);

    if (!attributes.is_directory) {
        strcat(name, ".");

        i = 0;
        for (i; i < 3; i++)
            if (entry.ext[i] == ' ') break;

        strncat(name, entry.ext, i);
    }

    for(i = 0; name[i]; i++) name[i] = tolower(name[i]);

    return name;
}

void fat16_read(struct fat16_super *super, struct fat16_inode *inode, char *buffer, size_t size)
{
    __set_device_position_on_entry(super, inode);

    fread(buffer, size, 1, super->device);
}
