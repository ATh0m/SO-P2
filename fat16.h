#include <stdio.h>
#include <stdlib.h>

// Zestaw struktur opisujących ważne sektory systemu plików FAT16

// FAT16 Boot Sector - zawiera podstawowe informacje na temat wyglądu
// systemu plików
typedef struct {
    unsigned char jmp[3];                   // instrukcje wykorzystywane przy bootowaniu
    char oem[8];                            // określa sposób formatowania dysku
    unsigned short sector_size;             // rozmiar sektora w bajtach
    unsigned short sectors_per_cluster;     // liczba sektorów na klaster
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
} __attribute((packed)) FAT16BootSector;

// Wypisz informacje z FAT16 Boot Sector
void print_fat16_boot_sector_info(FAT16BootSector *bs);

// FAT16 Entry - zawiera informacje na temat obiektów tj. katalog, plik
typedef struct {
    unsigned char filename[8];              // nazwa
    unsigned char ext[3];                   // rozszerzenie
    unsigned char attributes;               // atrybuty
    unsigned char reserved[10];             
    unsigned short modify_time;             // czas modyfikacji, utworzenia
    unsigned short modify_date;             // data modyfikacji, utworzenia
    unsigned short starting_cluster;        // numer początkowego klastra z danymi
    unsigned int file_size;                 // rozmiar
} __attribute((packed)) FAT16Entry;

// Wypisz informacje z FAT16 Entry
void print_fat16_entry_info(FAT16Entry *entry);
