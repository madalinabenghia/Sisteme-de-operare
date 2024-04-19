#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#define MP 256
#define MM 512

// Structura pentru stocarea metadatelor fiecărei intrări din director
typedef struct {
    char path[MP];
    char metadata[MM];
}Metadata;

// Funcție pentru crearea metadatelor pentru o intrare
void createMetadata(const char *path, char *metadata) {
    struct stat fileStat;
    if (stat(path, &fileStat) < 0) {
        printf("Eroare la obținerea metadatelor pentru %s\n", path);
        exit(EXIT_FAILURE);
    }
    sprintf(metadata, "Size: %lld, Modificat la: %ld", (long long)fileStat.st_size, (long)fileStat.st_mtime);
}

//Citire director si creare snapshot
void createSnapshot(const char *dirP) {
    DIR *directory;
    struct dirent *entry;
    Metadata metadata;
    char snapshotP[MP];
    FILE *snapshotFile;

    directory = opendir(dirP);

    if (directory == NULL) {
        printf("Eroare la deschiderea directorului %s\n", dirP);
        exit(EXIT_FAILURE);
    }

    sprintf(snapshotP, "%s/Snapshot.txt", dirP);
    snapshotFile = fopen(snapshotP, "w");

    if (snapshotFile == NULL) {
        printf("Eroare la crearea fișierului de snapshot pentru directorul %s\n", dirP);
        closedir(directory);
        exit(EXIT_FAILURE);
    }

    // Parcurgere fișiere și subdirectoare
    while ((entry = readdir(directory)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            sprintf(metadata.path, "%s/%s", dirP, entry->d_name);
            createMetadata(metadata.path, metadata.metadata);
            fprintf(snapshotFile, "%s\n", metadata.metadata);
        }
    }

    closedir(directory);
    fclose(snapshotFile);

    printf("Snapshot-ul pentru directorul %s a fost creat cu succes.\n", dirP);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Eroare %s \n", argv[0]);
        return 1;
    }

    const char *dirP = argv[1];

    createSnapshot(dirP);

    return 0;
}