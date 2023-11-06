#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<unistd.h>
#include<sys/stat.h>
#include<fcntl.h>


typedef struct {
    unsigned int FileSize, Width, Height, DataOffset;
}HeaderBMP;

int main(int argc, char *argv[]) {

    if (argc!=2) 
    { 
        printf("Usage: %s <imagine.bmp>/n", argv[0] );
        return 1;
    }

    
    //DESCHIDERE FISIER INTRARE: fisBMP
    
    int fisBMP = open(argv[1], O_RDONLY);
    if(fisBMP < 0) 
    {
        printf("Eroare la deschiderea fisierului!\n");
        return 1;
    }
    
    //CREARE FISIER statistica.txt
    
    int statFile = open("statistica.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (statFile < 0) 
    {
            
            perror("Error opening stat file");
            close(fisBMP);
            return 1;
             
    }
}
