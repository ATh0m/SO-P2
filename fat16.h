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
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

/* ------- Zestaw struktur opisujących ważne sektory systemu plików FAT16 -------- */

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

/* --------------------- Atrybuty FAT16 -------------------------- */

// Struktura opisująca atrybuty plików
struct fat16_attributes {
    bool is_read_only;          // tylko do odczytu
    bool is_hidden;             // ukryty
    bool is_system_file;        // plik systemowy
    bool is_volume_label;       // etykieta woluminu
    bool is_directory;          // katalog
    bool is_archive;            // archiwum
};

/*  Konwertuje ciąg bitów reprezentujących atrybuty pliku fat16 na 
 *  strukture fat16_attributes
 *  
 *  @param [char] fat16_raw_attributes - ciąg bitów opisująchych atrybuty pliku fat16
 *
 *  @return - wypełniona struktura fat16_attributes
 */
struct fat16_attributes convert_attributes(char fat16_raw_attributes);

/* ----------------- Czas modyfikacji pliku FAT16 -------------------- */

/*  Konweruje ciąg bitów opisujących date i czas modyfikacji pliku fat 16
 *  na strukturę tm (time.h)
 *
 *  @param [unsigned short] fat16_date - ciąg bitów opisujących datę modyfikacji
 *  @param [unsigned short] fat16_time - ciąg bitów opisujących czas modyfikacji
 *
 *  @return - wypełniona struktura tm (time.h)
 */
struct tm convert_time(unsigned short fat16_date, unsigned short fat16_time);

/*  Formarowanie surowej nazwy pliku FAT16
 *  
 *  @param [struct fat16_entry] entry - surowy opis pliku FAT16
 *
 *  @return - sformatowana nazwa
 */
char * fat16_format_name(struct fat16_entry entry);

// Struktura opisująca inode FAT16 
struct fat16_inode {
    uint64_t ino;                           // index inode'a

    struct fat16_attributes attributes;     // atrybuty FAT16
    struct fat16_entry entry;
};

/* --------------------------- Systemowa struktura stat ------------------------ */

/* struct stat {
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

/*  Wypełnianie systemowej struktury stat (sys/stat.h) na podstawie informacji ze
 *  struktury fat16_inode
 *
 *  @param [struct fat16_inode *] inode - struktura opisująca inode fat16
 *
 *  @return - wypełniona struktura stat (sys/stat.h)
 */
struct stat * fat16_inode_get_stat(struct fat16_inode *inode);

/* ----------------- Struktura do przetrzymywania inode'ów FAT16 ---------------------- */

/* Lista inode'ów. */
struct fat16_inode_node {
    struct fat16_inode *inode;
    struct fat16_inode_node *next;
};


/*  Struktura do przetrzymywania inode'ów FAT16. Działa na zasadzie hashmap'y.
 *  Jest to tablica list, gdzie kluczem jest hash danego argumentu.
 */
struct fat16_inodes {
    struct fat16_inode_node **container;
    size_t size;
};

/* Inicjalizacja struktury fat16_inodes
 *
 * @param [size_t] size - rozmiar
 *
 * @return - przygotowana struktura fat16_inodes
 */
struct fat16_inodes fat16_inodes_init(size_t size);

/* Dealokacja struktury fat16_inodes
 *
 * @param [struct fat16_inodes] inodes
 *
 */
void fat16_inodes_del(struct fat16_inodes inodes);


/* Dodanie nowego elementu do kontenera
 *
 * @param [struct fat16_inodes] inodes - kontener
 * @param [struct fat16_inode*] inode - element do dodania
 *
 */
void fat16_inodes_add(struct fat16_inodes inodes, struct fat16_inode *inode);

/* Wyciąganie elementu z kontenera na podstawie index'u inode
 *
 * @param [struct fat16_inodes] inodes - kontener
 * @param [uint64_t] ino - index szukanego inode'a
 *
 * @return - struktura fat16_inode odpowiadająca index'owi 
 */
struct fat16_inode * fat16_inodes_get(struct fat16_inodes inodes, uint64_t ino);

/* Pobieranie elementu z kontenera jeśli taki istnieje, a w przeciwnym
 * razie tworzenie nowego i umieszczanie go w kontenerze
 *
 * @param [struct fat16_inodes] inodes - kontener
 * @param [uint64_t] ino - index szukanego inode'a
 * @param [struct fat16_entry] entry - surowy opis pliku FAT16
 *
 * @return - wskaźnik do szukanego elementu
 */
struct fat16_inode * fat16_inodes_find(struct fat16_inodes inodes, uint64_t ino, struct fat16_entry entry);

/* --------------------- Główna struktura systemu FAT16 ------------------- */

// Struktura opisująca główny wygląd systemu FAT16
struct fat16_super {
    struct fat16_boot_sector boot_sector;
    unsigned short *FAT;

    FILE *device;

    struct fat16_inodes inodes;
};

/* -------------------- Implementacja operacji fuse dla systemu FAT16 --------------------- */

/* Odpowiednik fuse_lookup
 * Wyszukiwanie inode'a w podanym katalogu na podstawie nazwy
 *
 * @param [struct fat16_super *] super - wskaźnik do głównej struktury FAT16
 * @param [struct fat16_inode *] parent - wskaźnik do katalogu, rodzica
 * @param [const char] name - nazwa szukanego elementu
 *
 * @return NULL - szukany element nie istnieje
 * @return - wskaźnik na szukany element
 */
struct fat16_inode * fat16_lookup(struct fat16_super *super, struct fat16_inode *parent, const char *name);

/*  Odpowiednik fuse_readdir
 *  Zwraca zawartość danego katalogu
 *
 *  @param [struct fat16_super *] super - wskaźnik do głównej struktury FAT16
 *  @param [struct fat16_inode *] parent - wskaźnik do przeglądanego katalogu
 *
 *  @return - lista elementów [struct fat16_inode *], będących zawartością katalogu
 */
struct fat16_inode_node * fat16_readdir(struct fat16_super *super, struct fat16_inode *parent);

/*  Odpowiednik fuse_read
 *  Zwraca zawartość danego pliku
 *
 *  @param [struct fat16_super *] super - wskaźnik do głównej struktury FAT16
 *  @param [struct fat16_inode *] inode - wskażnik do przeglądanego pliku
 *  @param [char *] buffer - wskaźnik do bufora
 *  @param [size_t] size - rozmiar bufora
 */
void fat16_read(struct fat16_super *super, struct fat16_inode *inode, char *buffer, size_t size, off_t off);

#endif
