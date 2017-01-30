# System plików z użyciem FUSE

Tomasz Nanowski

## Opis problemu

Implementacja sterownika obsługi systemu plików FAT16 z użyciem niskopoziomowego interfejsu FUSE. Funkcje sterownika zostaną ograniczone wyłącznie do odczytu danych z obrazu systemu plików.

Głównym problemem będzie implementacja następujących operacji: `open, read, release, getattr, lookup, opendir, readdir, releasedir, statfs` włącznie ze zwracaniem odpowiednich kodów błędów oraz ustalona wielkość sektorów w blokach.

## Wymagania

- [libfuse3](https://github.com/libfuse/libfuse/)
- gcc 5.4.0

Sterownik korzysta ze struktur systemowych **Ubuntu 16.10**

## Uruchomienie

```Bash
make
./fat16 fs_image.raw fs_root
```
