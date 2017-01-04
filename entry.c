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
