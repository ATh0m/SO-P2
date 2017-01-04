#include <stdbool.h>
#include <stdio.h>

// Struktura opisująca atrybuty plików
typedef struct {
    bool is_read_only;          // tylko do odczytu
    bool is_hidden;             // ukryty
    bool is_system_file;        // plik systemowy
    bool is_volume_label;       // etykieta woluminu
    bool is_directory;          // katalog
    bool is_archive;            // archiwum
} Attributes;

// Inicjalizacja struktury Attributes
Attributes init_attributes(char fat16_attributes);

// Wypisz atrybuty pliku
void print_attributes(Attributes *attributes);
