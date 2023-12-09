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

// Structura pentru informațiile despre fișier BMP
struct BMPInfo {
    int size;
    int width;
    int height;
};

// Funcție pentru a afișa mesajul de eroare și a ieși din program
void showErrorAndExit(const char *errorMessage) {
    perror(errorMessage);
    exit(EXIT_FAILURE);
}

// Funcție pentru a obține numele utilizatorului
char *getUsername() {
    uid_t uid = geteuid();
    struct passwd *pw = getpwuid(uid);
    if (pw == NULL) {
        showErrorAndExit("Eroare obtinere nume utilizator");
    }
    return pw->pw_name;
}

// Funcție pentru a citi informațiile despre fișier BMP
void readBMPInfo(const char *filename, struct BMPInfo *bmp_info) {
    int bmp_file = open(filename, O_RDONLY);
    if (bmp_file == -1) {
        showErrorAndExit("Eroare deschidere fisier");
    }

    if (lseek(bmp_file, 14, SEEK_SET) == -1) {
        showErrorAndExit("Eroare la setarea poziției curente");
    }

    if (read(bmp_file, bmp_info, sizeof(struct BMPInfo)) != sizeof(struct BMPInfo)) {
        showErrorAndExit("Eroare la citirea informatiilor BMP");
    }

    close(bmp_file);
}

// Funcție pentru a obține informațiile despre fișier
void getFileStat(const char *filename, struct stat *file_stat) {
    if (stat(filename, file_stat) == -1) {
        showErrorAndExit("Eroare obtinere informatii fisier");
    }
}

// Funcție pentru a formata drepturile de acces
void formatPermissions(char *permissions, mode_t mode) {
    permissions[0] = (mode & S_IRUSR) ? 'R' : '-';
    permissions[1] = (mode & S_IWUSR) ? 'W' : '-';
    permissions[2] = (mode & S_IXUSR) ? 'X' : '-';
    permissions[3] = '\0';
}

// Funcție pentru a scrie statistici în fișier
void writeStatsToFile(const char *filename, const struct BMPInfo *bmp_info, const struct stat *file_stat) {
    int stat_file = open("statistica.txt", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (stat_file == -1) {
        showErrorAndExit("Eroare creare fisier de statistici");
    }

    char stats[1024];
    char permissions_user[4], permissions_group[4], permissions_other[4];

    formatPermissions(permissions_user, file_stat->st_mode & 0700);
    formatPermissions(permissions_group, file_stat->st_mode & 0070);
    formatPermissions(permissions_other, file_stat->st_mode & 0007);

    sprintf(stats, "nume fisier: %s\ninaltime: %u\nlungime: %u\ndimensiune: %lu\nidentificatorul utilizatorului: %s\ntimpul ultimei modificari: %s \ncontorul de legaturi: %ld\ndrepturi de acces user: %s\ndrepturi de acces grup: %s\ndrepturi de acces altii: %s\n",
            filename, bmp_info->height, bmp_info->width, file_stat->st_size, getUsername(), ctime(&(file_stat->st_mtime)), file_stat->st_nlink, permissions_user, permissions_group, permissions_other);

    if (write(stat_file, stats, strlen(stats)) == -1) {
        showErrorAndExit("Eroare scriere in fisier de statistici");
    }

    close(stat_file);
}

int main(int argc, char *argv[]) {
//verificare ca a primit un singur argument
    if (argc != 2) {
        write(STDERR_FILENO, "Usage: ./program <fisier_intrare>\n", 34);
        return 1;
    }

    struct BMPInfo bmp_info;
    struct stat file_stat;

    readBMPInfo(argv[1], &bmp_info);
    getFileStat(argv[1], &file_stat);
    writeStatsToFile(argv[1], &bmp_info, &file_stat);

    return 0;
}
