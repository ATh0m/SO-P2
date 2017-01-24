# System plików z użyciem FUSE

Tomasz Nanowski

## Opis problemu

Implementacja sterownika obsługi systemu plików FAT16 z użyciem interfejsu FUSE. Funkcja sterownika zostanie ograniczona wyłącznie do odczytu danych z obrazu systemu plików.

Głównym problemem będzie implementacja następujących operacji: *open, read, release, getattr, lookup, opendir, readdir, releasedir, statfs* włącznie ze zwracaniem odpowiednich kodów błędów oraz ustalona wielkość sektorów w blokach (512 bajtów).

## Wymagania

- [libfuse3](https://github.com/libfuse/libfuse/)
- gcc 5.4.0

## Uruchomienie

```Bash
make
mkdir -p fs_root
./mount.fat16 fs_root
```