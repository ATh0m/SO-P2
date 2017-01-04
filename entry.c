#include "entry.h"

Attributes init_attributes(char fat16_attributes) {
    Attributes attributes;

    attributes.is_read_only     = fat16_attributes & 0x01;
    attributes.is_hidden        = fat16_attributes & 0x02;
    attributes.is_system_file   = fat16_attributes & 0x04;
    attributes.is_volume_label  = fat16_attributes & 0x08;
    attributes.is_directory     = fat16_attributes & 0x10;
    attributes.is_archive       = fat16_attributes & 0x20;

    return attributes;
}

void print_attributes(Attributes *attributes) {
    printf("[ ");

    if (attributes->is_read_only) printf("read_only, ");
    if (attributes->is_hidden) printf("hidden, ");
    if (attributes->is_system_file) printf("system_file, ");
    if (attributes->is_volume_label) printf("volume_label, ");
    if (attributes->is_directory) printf("directory, ");
    if (attributes->is_archive) printf("archive, ");

    printf("]");
}

struct tm init_time(unsigned short fat16_date, unsigned short fat16_time) {
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
