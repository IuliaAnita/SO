#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <pwd.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/wait.h>


struct BMPInfo {
    int size;
    int width;
    int height;
};

int getLineNumber(const char *filename);

int getLineNumber(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Eroare deschidere fisier");
        exit(EXIT_FAILURE);
    }

    int count = 0;
    int ch;

    while ((ch = fgetc(file)) != EOF) {
        if (ch == '\n') {
            count++;
        }
    }

    fclose(file);
    return count;
}

void showErrorAndExit(const char *errorMessage) {
    perror(errorMessage);
    exit(EXIT_FAILURE);
}

char *getUsername() {
    uid_t uid = geteuid();
    struct passwd *pw = getpwuid(uid);
    if (pw == NULL) {
        showErrorAndExit("Eroare obtinere nume utilizator");
    }
    return pw->pw_name;
}

void readBMPInfo(const char *filename, struct BMPInfo *bmp_info) {
    int bmp_file = open(filename, O_RDONLY);
    if (bmp_file == -1) {
        showErrorAndExit("Eroare deschidere fisier");
    }

    if (lseek(bmp_file, 18, SEEK_SET) == -1) {
        showErrorAndExit("Eroare la setarea pozitiei curente");
    }

    if (read(bmp_file, bmp_info, sizeof(struct BMPInfo)) != sizeof(struct BMPInfo)) {
        showErrorAndExit("Eroare la citirea informatiilor BMP");
    }

    close(bmp_file);
}

void getFileStat(const char *filename, struct stat *file_stat) {
    if (stat(filename, file_stat) == -1) {
        showErrorAndExit("Eroare obtinere informatii fisier");
    }
}

void formatPermissions(char *permissions, mode_t mode) {
    permissions[0] = (mode & S_IRUSR) ? 'R' : '-';
    permissions[1] = (mode & S_IWUSR) ? 'W' : '-';
    permissions[2] = (mode & S_IXUSR) ? 'X' : '-';
    permissions[3] = '\0';
}

void writeStatsToFile(const char *filename, const struct BMPInfo *bmp_info, const struct stat *file_stat) {
    char stats[1024];
    char permissions_user[4], permissions_group[4], permissions_other[4];
    char outputFileName[1024];
    // Adaugă declarația pentru outputDir aici
    const char *outputDir = "/calea/catre/director"; // Schimbă cu calea corespunzătoare
    sprintf(outputFileName, "%s/%s_statistica.txt", outputDir, basename((char *)filename));

    int stat_file = open(outputFileName, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (stat_file == -1) {
        showErrorAndExit("Eroare creare fisier de statistici");
    }

    formatPermissions(permissions_user, file_stat->st_mode & 0700);
    formatPermissions(permissions_group, file_stat->st_mode & 0070);
    formatPermissions(permissions_other, file_stat->st_mode & 0007);

    sprintf(stats, "nume fisier: %s\n", basename((char *)filename));

    if (S_ISREG(file_stat->st_mode)) {
        if (strstr(filename, ".bmp") != NULL) {
            sprintf(stats + strlen(stats), "inaltime: %u\nlungime: %u\n", bmp_info->height, bmp_info->width);
        }
        sprintf(stats + strlen(stats), "dimensiune: %lu\nidentificatorul utilizatorului: %u\ntimpul ultimei modificari: %scontorul de legaturi: %lu\ndrepturi de acces user: %s\ndrepturi de acces grup: %s\ndrepturi de acces altii: %s\n",
                file_stat->st_size, file_stat->st_uid, ctime(&file_stat->st_mtime), file_stat->st_nlink, permissions_user, permissions_group, permissions_other);
    } else if (S_ISDIR(file_stat->st_mode)) {
        sprintf(stats + strlen(stats), "nume director: %s\nidentificatorul utilizatorului: %s\ndrepturi de acces user: RWX\ndrepturi de acces grup: R--\ndrepturi de acces altii: ---\n",
                basename((char *)filename), getUsername());
    } else if (S_ISLNK(file_stat->st_mode)) {
        char target_path[1024];
        ssize_t len = readlink(filename, target_path, sizeof(target_path) - 1);
        if (len != -1) {
            target_path[len] = '\0';
            struct stat target_stat;
            getFileStat(target_path, &target_stat);
            sprintf(stats + strlen(stats), "nume legatura: %s\ndimensiune: %zd\ndimensiune fisier: %lu\ndrepturi de acces user: %s\ndrepturi de acces grup: %s\ndrepturi de acces altii: %s\n",
                    basename((char *)filename), len, target_stat.st_size, permissions_user, permissions_group, permissions_other);
        }
    }

    // Afișează numărul de linii scrise în fișierul statistic.txt
    printf("Procesul cu PID-ul %d a scris %d linii in fisierul statistic.txt\n", getpid(), getLineNumber("statistic.txt"));

    if (write(stat_file, stats, strlen(stats)) == -1) {
        showErrorAndExit("Eroare scriere in fisier de statistici");
    }

    close(stat_file);
}

void processImage(const char *filename, const struct BMPInfo *bmp_info) {
    int bmp_file = open(filename, O_RDWR);
    if (bmp_file == -1) {
        showErrorAndExit("Eroare deschidere fisier");
    }

    u_int8_t header[54];
    if (read(bmp_file, header, sizeof(header)) != sizeof(header)) {
        showErrorAndExit("Eroare la citirea antetului BMP");
    }

    if (bmp_info->width < 0 || bmp_info->height < 0) {
        showErrorAndExit("Dimensiuni invalide ale imaginii BMP");
    }

    // Calculăm dimensiunea unei linii în octeți, inclusiv padding-ul
    int row_size = (3 * bmp_info->width + 3) & ~3;

    // Alocăm memorie pentru o linie de pixeli
    u_int8_t *row_data = (u_int8_t *)malloc(row_size);
    if (row_data == NULL) {
        showErrorAndExit("Eroare alocare memorie pentru linie de pixeli");
    }

    // Setăm cursorul la începutul datelor pixelilor
    if (lseek(bmp_file, header[10], SEEK_SET) == -1) {
        showErrorAndExit("Eroare la setarea pozitiei cursorului");
    }

    // Procesăm fiecare linie de pixeli
    for (int y = 0; y < bmp_info->height; y++) {
        // Citim o linie de pixeli
        if (read(bmp_file, row_data, row_size) != row_size) {
            showErrorAndExit("Eroare la citirea unei linii de pixeli");
        }

        // Procesăm fiecare pixel din linie
        for (int x = 0; x < bmp_info->width; x++) {
            // Calculăm offset-ul pixelului în linie
            int offset = 3 * x;

            // Extragem culorile pixelului
            u_int8_t blue = row_data[offset];
            u_int8_t green = row_data[offset + 1];
            u_int8_t red = row_data[offset + 2];

            // Calculăm valoarea tonului de gri conform formulei specificate
            u_int8_t gray = (u_int8_t)(0.299 * red + 0.587 * green + 0.114 * blue);

            // Actualizăm culorile pixelului cu tonul de gri
            row_data[offset] = gray;
            row_data[offset + 1] = gray;
            row_data[offset + 2] = gray;
        }

        // Repozitionăm cursorul la începutul liniei și scriem linia modificată
        if (lseek(bmp_file, -row_size, SEEK_CUR) == -1) {
            showErrorAndExit("Eroare la repozitionarea cursorului");
        }

        if (write(bmp_file, row_data, row_size) != row_size) {
            showErrorAndExit("Eroare la scrierea liniei de pixeli procesată");
        }
    }

    // Eliberăm memoria alocată pentru linia de pixeli
    free(row_data);

    // Închide fișierul BMP
    close(bmp_file);
}


void processDirectory(const char *inputDir, const char *outputDir) {
    DIR *dir = opendir(inputDir);
    if (dir == NULL) {
        showErrorAndExit("Eroare deschidere director");
    }

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        char file_path[1024];
        sprintf(file_path, "%s/%s", inputDir, entry->d_name);

        struct BMPInfo bmp_info;
        struct stat file_stat;

        // Verificați dacă intrarea este un director sau un fișier
        if (entry->d_type == DT_REG || entry->d_type == DT_DIR) {
            if (strstr(entry->d_name, ".bmp") != NULL && entry->d_type == DT_REG) {
                // Creează un proces fiu pentru procesarea imaginii BMP
                pid_t child_pid = fork();

                if (child_pid == -1) {
                    showErrorAndExit("Eroare la crearea procesului fiu");
                }

                if (child_pid == 0) {
                    // Proces fiu
                    processImage(file_path, &bmp_info);
                    exit(EXIT_SUCCESS);
                } else {
                    // Proces părinte
                    int status;
                    waitpid(child_pid, &status, 0);
                    printf("S-a incheiat procesul cu PID-ul %d si codul %d\n", child_pid, WEXITSTATUS(status));
                }

            } else {
                // Creează un proces fiu pentru scrierea statisticilor în fișierul corespunzător
                pid_t child_pid = fork();

                if (child_pid == -1) {
                    showErrorAndExit("Eroare la crearea procesului fiu");
                }

                if (child_pid == 0) {
                    // Proces fiu
                    writeStatsToFile(file_path, &bmp_info, &file_stat);

                    // Afișează numărul de linii scrise în fișierul statistic.txt
                    printf("Procesul cu PID-ul %d a scris %d linii in fisierul statistic.txt\n", getpid(), getLineNumber("statistic.txt"));

                    exit(EXIT_SUCCESS);
                } else {
                    // Proces părinte
                    int status;
                    waitpid(child_pid, &status, 0);
                    printf("S-a incheiat procesul cu PID-ul %d si codul %d\n", child_pid, WEXITSTATUS(status));
                }
            }
        } else {
            // Aici poți adăuga orice altă logică specifică pentru alte tipuri de intrări
            // (legături simbolice, socket-uri, etc.)
            printf("Intrare neacceptata: %s\n", entry->d_name);
        }
    }

    closedir(dir);
}


int main(int argc, char *argv[]) {
    // Verifică dacă numărul de argumente este corect
    if (argc != 3) {
        write(STDERR_FILENO, "Usage: ./program <director_intrare> <director_iesire>\n", 54);
        return 1;
    }

    // Verifică existența și drepturile de citire pe directorul de intrare
    if (access(argv[1], R_OK) == -1) {
        perror("Eroare acces director de intrare");
        return 1;
    }

    // Verifică existența și drepturile de scriere pe directorul de ieșire
    if (access(argv[2], W_OK) == -1) {
        perror("Eroare acces director de iesire");
        return 1;
    }

    // Apelează funcția processDirectory pentru a parcurge directorul de intrare
    processDirectory(argv[1], argv[2]);

    // Afișează numărul de linii din fișierul "statistic.txt"
    int lineNumber = getLineNumber("statistic.txt");
    printf("Numarul total de linii in fisierul statistic.txt: %d\n", lineNumber);

    return 0;
}
