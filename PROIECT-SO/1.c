#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

// Funcție pentru a obține extensia unui fișier
const char *ObtineExtensieFisier(const char *cale) {
    const char *rezultat = cale + strlen(cale);
    while (rezultat > cale && *rezultat != '.' && *rezultat != '/' && *rezultat != '\\') {
        rezultat--;
    }
    return (*rezultat == '.' && rezultat > cale) ? rezultat : cale + strlen(cale);
}

// Funcție pentru a scrie un șir de caractere într-un fișier
void ScrieInFisier(int fd, const char *mesaj) {
    write(fd, mesaj, strlen(mesaj));
}

// Funcție pentru a scrie un număr întreg într-un fișier cu o etichetă asociată
void ScrieIntInFisier(int fd, const char *eticheta, int valoare) {
    char buffer[1024];
    sprintf(buffer, "%s: %d\n", eticheta, valoare);
    ScrieInFisier(fd, buffer);
}

// Funcție pentru a scrie drepturile de acces într-un fișier cu o etichetă asociată
void ScrieDrepturiAccesInFisier(int fd, const char *eticheta, mode_t mod) {
    char buffer[1024];
    sprintf(buffer, "%s: %c%c%c\n", eticheta, (mod & S_IRUSR) ? 'R' : '-',
            (mod & S_IWUSR) ? 'W' : '-', (mod & S_IXUSR) ? 'X' : '-');
    ScrieInFisier(fd, buffer);
}

// Funcție pentru a scrie statisticile unui fișier într-un fișier
void ScrieStatisticiFisierInFisier(int fd, struct stat fileStat) {
    // Scrie informații despre dimensiune, user ID, numărul de legături și data ultimei modificări
    ScrieIntInFisier(fd, "Dimensiune", fileStat.st_size);
    ScrieIntInFisier(fd, "ID Utilizator", fileStat.st_uid);
    ScrieIntInFisier(fd, "Număr legături", fileStat.st_nlink);
    
    char timeBuffer[1024];
    strftime(timeBuffer, sizeof(timeBuffer), "Ultima modificare: %Y-%m-%d %H:%M:%S\n", localtime(&fileStat.st_mtime));
    ScrieInFisier(fd, timeBuffer);

    // Scrie drepturile de acces pentru utilizator, grup și alții
    ScrieDrepturiAccesInFisier(fd, "Drepturi acces utilizator", fileStat.st_mode);
    ScrieDrepturiAccesInFisier(fd, "Drepturi acces grup", fileStat.st_mode >> 3);
    ScrieDrepturiAccesInFisier(fd, "Drepturi acces alții", fileStat.st_mode >> 6);
}

// Funcție pentru a procesa un fișier
void ProceseazaFisier(const char *numeFisierIntrare) {
    // Deschide fișierul de intrare pentru citire
    int fd_input = open(numeFisierIntrare, O_RDONLY);
    if (fd_input == -1) {
        perror("Nu s-a putut deschide fișierul de intrare");
        exit(-1);
    }

    printf("Fișierul de intrare %s s-a deschis cu succes!\n", numeFisierIntrare);

    int latime, inaltime;
    // Citește dimensiunile imaginii din fișierul de intrare
    read(fd_input, &latime, sizeof(int));
    read(fd_input, &inaltime, sizeof(int));

    const char *numeFisierIesire = "statistici.txt";
    // Deschide sau creează fișierul de ieșire pentru scriere
    int fd_output = open(numeFisierIesire, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IXUSR);
    if (fd_output == -1) {
        perror("Fișierul de ieșire nu există și nu a putut fi creat");
        exit(-1);
    }

    printf("Fișierul de ieșire %s s-a deschis cu succes!\n", numeFisierIesire);

    // Scrie informații despre latime și inaltime în fișierul de ieșire
    ScrieInFisier(fd_output, "Statistici pentru fișierul:\n\n");
    ScrieIntInFisier(fd_output, "Latime", latime);
    ScrieIntInFisier(fd_output, "Inaltime", inaltime);

    struct stat fileStat;
    // Scrie statisticile fisierului in fisierul de iesire
    ScrieStatisticiFisierInFisier(fd_output, fileStat);

    // Închide fișierele
    if (close(fd_input) == -1) {
        printf("Fișierul de intrare %s nu s-a putut închide\n", numeFisierIntrare);
    } else {
        printf("Fișierul de intrare %s s-a închis cu succes\n", numeFisierIntrare);
    }

    if (close(fd_output) == -1) {
        printf("Fișierul de ieșire %s nu s-a putut închide\n", numeFisierIesire);
    } else {
        printf("Fișierul de ieșire %s s-a închis cu succes\n", numeFisierIesire);
    }
}

int main(int argc, char **argv) {
    // Verifică dacă numărul corect de argumente a fost furnizat
    if (argc != 2) {
        perror("Utilizare: ./program <nume_fisier_intrare>");
        exit(-1);
    }

    const char *numeFisierIntrare = argv[1];

    // Verifică dacă extensia fișierului de intrare este '.bmp'
    if (strcmp(ObtineExtensieFisier(numeFisierIntrare), ".bmp") != 0) {
        perror("Fișierul de intrare trebuie să aibă extensia '.bmp'");
        exit(-1);
    }

    // Proceseaza fisierul
    ProceseazaFisier(numeFisierIntrare);

    return 0;
}
