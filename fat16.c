#include "fat16.h"

void print_fat16_boot_sector_info(FAT16BootSector *bs) {
    printf("  Jump code: %02X:%02X:%02X\n", bs->jmp[0], bs->jmp[1], bs->jmp[2]);
    printf("  OEM code: [%.8s]\n", bs->oem);
    printf("  sector_size: %d\n", bs->sector_size);
    printf("  sectors_per_cluster: %d\n", bs->sectors_per_cluster);
    printf("  reserved_sectors: %d\n", bs->reserved_sectors);
    printf("  number_of_fats: %d\n", bs->number_of_fats);
    printf("  root_dir_entries: %d\n", bs->root_dir_entries);
    printf("  total_sectors_short: %d\n", bs->total_sectors_short);
    printf("  media_descriptor: 0x%02X\n", bs->media_descriptor);
    printf("  fat_size_sectors: %d\n", bs->fat_size_sectors);
    printf("  sectors_per_track: %d\n", bs->sectors_per_track);
    printf("  number_of_heads: %d\n", bs->number_of_heads);
    printf("  hidden_sectors: %d\n", bs->hidden_sectors);
    printf("  total_sectors_long: %d\n", bs->total_sectors_long);
    printf("  drive_number: 0x%02X\n", bs->drive_number);
    printf("  current_head: 0x%02X\n", bs->current_head);
    printf("  boot_signature: 0x%02X\n", bs->boot_signature);
    printf("  volume_id: 0x%08X\n", bs->volume_id);
    printf("  Volume label: [%.11s]\n", bs->volume_label);
    printf("  Filesystem type: [%.8s]\n", bs->fs_type);
    printf("  Boot sector signature: 0x%04X\n", bs->boot_sector_signature);
}

void print_fat16_entry_info(FAT16Entry *entry) {
    
    switch(entry->filename[0]) {
    // nieużywany
    case 0x00:
        return;
    // usunięty
    case 0xE5:
        printf("Deleted file: [?%.7s.%.3s]\n", entry->filename+1, entry->ext);
        return;
    // nazwa zaczyna się od 0xE5
    case 0x05:
        printf("File starting with 0xE5: [%c%.7s.%.3s]\n", 0xE5, entry->filename+1, entry->ext);
        break;
    // kataolg
    case 0x2E:
        printf("Directory: [%.8s.%.3s]\n", entry->filename, entry->ext);
        break;
    // plik
    default:
        printf("File: [%.8s.%.3s]\n", entry->filename, entry->ext);
    }

    // wypisz poczatkowy klaster z danymi i rozmiar
    printf("  Start: [%04X]\n  Size: %d\n", entry->starting_cluster, entry->file_size);

    // wypisz datę modyfikacji
    struct tm modify_time = init_time(entry->modify_date, entry->modify_time);
    printf("  Modified: %s", asctime(&modify_time));

    // wypisz atrybuty
    Attributes attributes = init_attributes(entry->attributes);
    printf("  Attributes: ");
    print_attributes(&attributes);
    printf("\n");
}
