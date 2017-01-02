#include <stdio.h>
#include "fat16.h"

int main(int argc, char *argv[]) {

    // sprawdzić czy podano ścieżkę do urządzenia
    if (argc != 2) return 1;

    // otworzyć urządzenie
    FILE *device = fopen(argv[1], "rb");

    // uzupełnić informacja na temat Boot Sector'a
    FAT16BootSector bs;
    fread(&bs, sizeof(FAT16BootSector), 1, device);

    // wypisać informacje o Boot Sector
    print_fat16_boot_sector_info(&bs);

    // przesunąć się do miejsca z katalogiem głownym
    fseek(device, (bs.reserved_sectors-1 + bs.fat_size_sectors * bs.number_of_fats) * bs.sector_size, SEEK_CUR);

    // uzyskać informacje na temat wpisów
    FAT16Entry entry;

    for (int i = 0; i < bs.root_dir_entries; i++) {
        fread(&entry, sizeof(entry), 1, device);
        print_fat16_entry_info(&entry);
    }

    // zamknąć urządzenie
    fclose(device);

    return 0;
}
