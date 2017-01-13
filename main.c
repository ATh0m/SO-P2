#include <stdio.h>
#include <stdlib.h>
#include "fat16.h"

int main(int argc, char *argv[]) {

    // sprawdzić czy podano ścieżkę do urządzenia
    if (argc != 2) return 1;

    // otworzyć urządzenie
    FILE *device = fopen(argv[1], "rb");

    // uzupełnić informacja na temat Boot Sector'a
    struct fat16_boot_sector bs;
    fread(&bs, sizeof(struct fat16_boot_sector), 1, device);

    // przesunąć się do początku FAT
    fseek(device, bs.reserved_sectors * bs.sector_size, SEEK_SET);

    // przeczytać sekcje FAT
    unsigned short *FAT = malloc(bs.fat_size_sectors * bs.sector_size);
    fread(FAT, sizeof(unsigned short), bs.fat_size_sectors * bs.sector_size / 2, device);

    // przesunąć się do miejsca z katalogiem głownym
    fseek(device, (bs.fat_size_sectors * bs.number_of_fats + 1) * bs.sector_size, SEEK_SET);

    // uzyskać informacje na temat wpisów
    struct fat16_entry entry;

    for (int i = 0; i < bs.root_dir_entries; i++) {
        fread(&entry, sizeof(entry), 1, device);
    }

    printf("Root directory end: 0x%08X\n", (int)ftell(device));

    // zamknąć urządzenie
    fclose(device);

    return 0;
}
