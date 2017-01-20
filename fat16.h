#ifndef FAT16_H
#define FAT16_H

#include <time.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <syslog.h>
#include <ctype.h>

// Zestaw struktur opisujących ważne sektory systemu plików FAT16

// FAT16 Boot Sector - zawiera podstawowe informacje na temat wyglądu
// systemu plików
struct fat16_boot_sector {
    unsigned char jmp[3];                   // instrukcje wykorzystywane przy bootowaniu
    char oem[8];                            // określa sposób formatowania dysku
    unsigned short sector_size;             // rozmiar sektora w bajtach
    unsigned char sectors_per_cluster;      // liczba sektorów na klaster
    unsigned short reserved_sectors;        // liczba zarezerwowanych sektorów
    unsigned char number_of_fats;           // liczba kopii FAT
    unsigned short root_dir_entries;        // liczba elementów katalogu root (włącznie z nieużywanymi)
    unsigned short total_sectors_short;     // łączna liczba sektorów
    unsigned char media_descriptor;         // rodzaj urządzenia
    unsigned short fat_size_sectors;        // liczba sektorów wykorzystywana przez FAT
    unsigned short sectors_per_track;       // liczba sektorówn na ścieżkę
    unsigned short number_of_heads;         // liczba powierzchni dysku
    unsigned int hidden_sectors;            // liczba ukrytych sektorów
    unsigned int total_sectors_long;        // łączna liczba sektorów
    unsigned char drive_number;             // numer dysku
    unsigned char current_head;
    unsigned char boot_signature;
    unsigned int volume_id;                 // numer seryjny woluminu
    char volume_label[11];                  // etykieta woluminu
    char fs_type[8];                        // typ systemu plików
    char boot_code[448];
    unsigned short boot_sector_signature;
} __attribute((packed));

// FAT16 Entry - zawiera informacje na temat obiektów tj. katalog, plik
struct fat16_entry {
    unsigned char filename[8];              // nazwa
    unsigned char ext[3];                   // rozszerzenie
    unsigned char attributes;               // atrybuty
    unsigned char reserved[10];
    unsigned short modify_time;             // czas modyfikacji, utworzenia
    unsigned short modify_date;             // data modyfikacji, utworzenia
    unsigned short starting_cluster;        // numer początkowego klastra z danymi
    unsigned int file_size;                 // rozmiar
} __attribute((packed));

// Struktura opisująca atrybuty plików
struct fat16_attributes {
    bool is_read_only;          // tylko do odczytu
    bool is_hidden;             // ukryty
    bool is_system_file;        // plik systemowy
    bool is_volume_label;       // etykieta woluminu
    bool is_directory;          // katalog
    bool is_archive;            // archiwum
};

// Inicjalizacja struktury FAT16 Attributes
struct fat16_attributes convert_attributes(char fat16_raw_attributes);

// Inicjalizacja struktury czasu
struct tm convert_time(unsigned short fat16_date, unsigned short fat16_time);

struct fat16_inode {
    uint64_t ino;

    struct fat16_attributes attributes;
    struct fat16_entry entry;

    struct fat16_super *super;
    struct fat16_inode *next;
};

struct stat * fat16_inode_get_stat(struct fat16_inode *inode);

#define FAT16_INODES_CONTAINER_SIZE 4096

struct fat16_inodes {
    struct fat16_inode **container;
    size_t use;
    size_t size;
};

struct fat16_inodes fat16_inodes_init(size_t size);

void fat16_inodes_del(struct fat16_inodes inodes);

void fat16_inodes_add(struct fat16_inodes inodes, struct fat16_inode *inode);

struct fat16_inode * fat16_inodes_get(struct fat16_inodes inodes, uint64_t ino);

struct fat16_inode * fat16_inodes_find(struct fat16_inodes inodes, uint64_t ino, struct fat16_entry entry);

struct fat16_super {
    struct fat16_boot_sector boot_sector;
    unsigned short *FAT;

    FILE *device;

    struct fat16_inodes inodes;
};

struct fat16_inode * fat16_lookup(struct fat16_super *super, struct fat16_inode *parent, const char *name);

struct fat16_inode_node {
    struct fat16_inode *inode;
    struct fat16_inode_node *next;
};

struct fat16_inode_node * fat16_readdir(struct fat16_super *super, struct fat16_inode *parent);

char * fat16_format_name(struct fat16_entry entry);

#endif
