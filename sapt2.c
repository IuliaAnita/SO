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

struct BMPInfo {
    int size;
    int width;
    int height;
};

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
    int stat_file = open("statistica.txt", O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (stat_file == -1) {
        showErrorAndExit("Eroare creare fisier de statistici");
    }

    char stats[1024];
    char permissions_user[4], permissions_group[4], permissions_other[4];

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

    if (write(stat_file, stats, strlen(stats)) == -1) {
        showErrorAndExit("Eroare scriere in fisier de statistici");
    }

    close(stat_file);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        write(STDERR_FILENO, "Usage: ./program <director_intrare>\n", 37);
        return 1;
    }

    DIR *dir = opendir(argv[1]);
    if (dir == NULL) {
        showErrorAndExit("Eroare deschidere director");
    }

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_DIR) {
            char file_path[1024];
            sprintf(file_path, "%s/%s", argv[1], entry->d_name);

            struct BMPInfo bmp_info;
            struct stat file_stat;

            if (strstr(entry->d_name, ".bmp") != NULL) {
                readBMPInfo(file_path, &bmp_info);
            }

            getFileStat(file_path, &file_stat);
            writeStatsToFile(file_path, &bmp_info, &file_stat);
        }
    }

    closedir(dir);

    return 0;
}
