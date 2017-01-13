#include "fat16.h"

struct fat16_attributes init_attributes(char fat16_raw_attributes) 
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

struct tm init_time(unsigned short fat16_date, unsigned short fat16_time) 
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

struct fat16_inodes fat16_inodes_init(size_t size)
{
    struct fat16_inodes inodes = {
        .size = size,
        .container = malloc(sizeof(struct fat16_inode *) * size),
        .use = 0,
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
