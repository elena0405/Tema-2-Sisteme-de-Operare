#include <stdlib.h>
#include "so_stdio.h"
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>	/* open */
#include <sys/stat.h>	/* open */
#include <fcntl.h>	/* O_RDWR, O_CREAT, O_TRUNC, O_WRONLY */
#include <unistd.h>	/* close */

struct _so_file
{
    unsigned char *buffer;
    int offset;
    int dimensiune_buffer;
    int file_descriptor;
    int file_size;
    size_t element_size;
    int index;
    int caractere_procesate;
    int sfarsit_de_fisier;
    int eroare;
    int ultima_operatie;
};

SO_FILE *so_fopen(const char *pathname, const char *mode)
{
    int fd = -1;
    SO_FILE *f;
    f = NULL;

    if (strcmp(mode, "r") == 0) {
        // Deschid fisierul in modul "read". 
        fd = open(pathname, O_RDONLY);
    } else if (strcmp(mode, "r+") == 0) {
       // Deschid fisierul in modul "read" + "write".
        fd = open(pathname, O_RDWR);
    } else if (strcmp(mode, "w") == 0) {
        // Deschid fisierul in modul "write".
        fd = open(pathname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    } else if (strcmp(mode, "w+") == 0) {
        // Deschid fisierul in modul "write" + "read".
        fd = open(pathname, O_RDWR | O_CREAT | O_TRUNC, 0644);
    } else if (strcmp(mode, "a") == 0) {
        // Deschid fisierul in modul "append".
        fd = open(pathname, O_APPEND | O_CREAT | O_WRONLY, 0644);
    } else if (strcmp(mode, "a+") == 0) {
        // Deschid fisierul in modul "append" + "read".
        fd = open(pathname, O_APPEND | O_RDWR | O_CREAT, 0644);
    }

    if (fd != -1) {
        // Initializez campurile din structura si fac alocarile necesare.
        f = calloc(1, sizeof(SO_FILE));
        f->buffer = calloc(4096, sizeof(unsigned char));
        f->dimensiune_buffer = 0;
        f->offset = 0;
        f->file_descriptor = fd;
        f->index = 0;
        f->element_size = 1;
        f->caractere_procesate = -1;
        f->sfarsit_de_fisier = 0;
        f->eroare = 0;
        f->ultima_operatie = -1;
    }

    // Intorc elementul de tip SO_FILE creat.
    return f;
}

int so_fclose(SO_FILE *stream)
{
    int rc;
    
    // Apelez functia fflush.
    rc = so_fflush(stream);
    if (rc == SO_EOF) {
        // Eliberez memoria.
        free(stream->buffer);
        free(stream);
        // Intorc SO_EOF pentru a semnaliza o eroare.
        return SO_EOF;
    }
    
    // Inchid handle-ul fisierului.
    rc = close(stream->file_descriptor);

    // Daca nu dau de erori, intorc 0.
    if (rc == 0) {
        // Eliberez memoria.
        free(stream->buffer);
        free(stream);

        return 0;
    }

    // Eliberez memoria.
    free(stream->buffer);
    free(stream);
    // Intorc SO_EOF pentru a semnaliza o eroare.
    return SO_EOF;
}

int so_fileno(SO_FILE *stream)
{   
    // Intorc file descriptorul asociat elementului stream.
    return stream->file_descriptor;
}

int so_fflush(SO_FILE *stream)
{
    int nr;

    // Verific daca ultima operatie a fost de scriere.
    if (stream->ultima_operatie == 1) {
        // Verific daca am date in buffer.
        if (stream->dimensiune_buffer > 0) {
            // Daca buffer-ul nu e gol, scrie datele din buffer in fisier.
            nr = write(stream->file_descriptor, stream->buffer, 
                        stream->dimensiune_buffer);
            
            // Daca nu s-au scris caractere in fisier, inseamna ca am ajuns 
			// la finalul fisierului.
            if (nr == 0) {
                stream->sfarsit_de_fisier = 1;
            } else if (nr < 0) {
                // Daca intampin erori, actualizez campul "eroare" din structura 
			    // si intorc SO_EOF.
                stream->eroare = 1;
                return SO_EOF;
            }

            // Reinitializez campurile offst, index si dimensiune_buffer cu 0.
            stream->offset = 0;
            stream->index = 0;
            stream->dimensiune_buffer = 0;
        }
    }

    // Daca nu am erori, intorc 0.
    return 0;
}

int so_fseek(SO_FILE *stream, long offset, int whence)
{   
    // Verific daca ultima operatie a fost de scriere.
    if (stream->ultima_operatie == 1) {
        // Apelez fflush.
        int rc = so_fflush(stream);

        // Daca intampin vreo eroare, actualizez campul "eroare"
		// din variabila stream si intorc SO_EOF.
        if (rc == SO_EOF) {
            stream->eroare = 1;
            return SO_EOF;
        }
    } else if (stream->ultima_operatie == 0) {
        // Daca ultima operatie a fost de citire, fac 
		// actualizarile cerute in cerinta.
        memset(stream->buffer, 0, 4096);
        stream->dimensiune_buffer = 0;
        stream->ultima_operatie = -1;
        stream->index = 0;
        stream->offset = 0;
    }
    
    // Setez cursorul la pozitia ceruta in fisier.
    off_t nr = lseek(stream->file_descriptor, offset, whence);

    // Daca intampin vreo eroare, actualizez campul "eroare"
	// din variabila stream si intorc SO_EOF.
    if (nr == -1) {
        stream->eroare = 1;
        return SO_EOF;
    }

    stream->caractere_procesate = nr;

    // Daca nu intampin probleme, intorc 0.
    return 0;
}

long so_ftell(SO_FILE *stream)
{
    // Returnez pozitia curenta din fisier.
    return stream->caractere_procesate;
}

size_t so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
    int i = 0, index = 0, j, rest;
    int ret, minim;
    unsigned char* str = (unsigned char*) ptr;

    // Salvez dimensiunea unui element.
    stream->element_size = size;

    // Verific daca dimensiunea elementelor pe care vreau sa le citesc nu depaseste 
	// capacitatea buffer-ului.
    if (nmemb < 4096 / size) {
        ret = so_fgetc(stream);

        // Daca dupa apelul so_fgetc am erori, intorc dimensiunea fisierului si 
		// fac actualizarile necesare.
        if (ret == SO_EOF) {
            stream->sfarsit_de_fisier = 1;
            stream->eroare = 1;
            return stream->file_size;
        }

        // Retin indexul din capat.
        minim = stream->index + nmemb * size;
        // Verific daca aceasta valoare depaseste capacitatea maxima a buffer-ului.
        if (minim > 4096) {
            // Daca da, atunci, mai intai, copiez restul caracterelor pana la 4096.
            for (j = stream->index; j < 4096; j++) {
                str[index] = stream->buffer[j];
                index += 1;
                i++;
            }

            // In continuare, determin cate caractere au ramas necitite.
            rest = minim - 4096;
            stream->index = 4096;
            stream->dimensiune_buffer = 0;

            // Apelez din nou so_fgetc.
            ret = so_fgetc(stream);
            if (ret == SO_EOF) {
                stream->sfarsit_de_fisier = 1;
                stream->eroare = 1;
                return stream->file_size;
            }

            // Copiez restul caracterelor ce trebuie citite.
            for (j = stream->index; j < rest; j++) {
                str[index] = stream->buffer[j];
                // Incrementez numarul de caractere citite.
                index += 1;
                i++;
            }

            stream->index += rest;
        } else {
            // Daca indexul din capatul drept nu depaseste 4096, copiez 
			// datele in buffer.
            for (j = stream->index; j < minim; j++) {
                str[index] = stream->buffer[j];
                index++;
                // Incrementez numarul de caractere citite.
                i++;
            }

            stream->index += nmemb * size;
        }

        i = i / size;
        stream->file_size += index;
    } else {
        // Daca dimensiunea elementelor pe care vreau sa le citesc depaseste 
		// 4096, o sa fac citirile intr-un for.
        i = 0;

        while (i < nmemb) {
            ret = so_fgetc(stream);
            
            if (ret == SO_EOF) {
                stream->sfarsit_de_fisier = 1;
                stream->eroare = 1;
                return stream->file_size;
            }

            // Retin caracterul citit.
            str[index] = (unsigned char) ret;
            index += 1;
            
            // Incrementez numarul de elemente citite.
            i++;
            stream->file_size = index;
        }
    }

    // Retin numarul de elemente citite.
    stream->caractere_procesate = i;
    
    // Intorc numarul de elemente citite.
    return i;
}

size_t so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
    int i, j, ret;
    unsigned char* str = (unsigned char*) ptr;
    stream->element_size = size;

    // Verific daca numarul de caratere pe care vreau sa le scriu depaseste
	// dimensiunea buffer-ului.
    if (nmemb < 4096 / size) {
        // Daca nu, atunci scriu in buffer caracter cu caracter.
        for (i = 0; i < nmemb * size; i++) {
            ret = so_fputc(str[i], stream);

            // Daca am eroare, actualizez campurile aferente si intorc SO_EOF.
            if (ret == SO_EOF) {
                stream->sfarsit_de_fisier = 1;
                stream->eroare = 1;
                return stream->file_size;
            }
        }

        // Retin numarul de elemente scrise.
        i = i / size;
    } else {
        i = 0;

        while (i < nmemb * size) {
            // Retin in buffer elementele.
            ret = so_fputc(str[i], stream);

            // Daca intampin vreo eroare, intorc SO_EOF si fac
			// modificarile necesare.
            if (ret == SO_EOF) {
                stream->sfarsit_de_fisier = 1;
                stream->eroare = 1;
                return stream->file_size;
            }

            i++;
        }

        i = i / size;
    }

    stream->file_size += i;
    stream->caractere_procesate = i;

    // Intorc numarul de elemente scrise.
    return i;
}

int so_fgetc(SO_FILE *stream)
{   
    ssize_t nr;
    int cod_ascii;
    unsigned char caracter;

    // Verific daca trebuie sa fac un apel de sistem.
    if (stream->dimensiune_buffer == 0 ||
        ((stream->offset >= stream->dimensiune_buffer ||
            stream->index >= stream->dimensiune_buffer) &&
         stream->dimensiune_buffer != 0)) {
            // Daca da, apelez ReadFile ca sa citesc datele din fisier si sa le 
			// pun in buffer.
            nr = read(stream->file_descriptor, stream->buffer, 4096);
            if (nr <= 0) {
                stream->sfarsit_de_fisier = 1;
                stream->eroare = 1;
                return SO_EOF;
            }

            stream->offset = 0;
            stream->index = 0;
            stream->dimensiune_buffer = nr;
    }

    // Retin caracaterul citit.
    caracter = stream->buffer[stream->offset];
    stream->offset += 1;
    // Marchez faptul ca ultima operatie a fost una de citire.
    stream->ultima_operatie = 0;

    cod_ascii = (int)caracter;

    return cod_ascii;
}

int so_fputc(int c, SO_FILE *stream)
{
    ssize_t nr;
    int cod_ascii;
    unsigned char caracter;

    // Verific daca buffer-ul e plin.
    if (stream->dimensiune_buffer == 4096) {
        // Daca da, atunci scriu datele in fisier.
        nr = write(stream->file_descriptor, stream->buffer, 
                    stream->dimensiune_buffer);
        if (nr <= 0) {
            stream->sfarsit_de_fisier = 1;
            stream->eroare = 1;
            return SO_EOF;
        }

        // Reinitializez valorile cu 0.
        stream->offset = 0;
        stream->index = 0;
        stream->dimensiune_buffer = 0;
    } 
    
    // Scriu in buffer caracterul dat ca parametru.
    stream->buffer[stream->offset] = (unsigned char)c;
    caracter = stream->buffer[stream->offset];
    stream->offset += 1;
    stream->index += 1;
    stream->dimensiune_buffer += 1;
    stream->file_size += 1;
    cod_ascii = (int)caracter;
    // Marchez ca ultima operatie a fost una de scriere.
    stream->ultima_operatie = 1;

    return cod_ascii;
}

int so_feof(SO_FILE *stream)
{
    // Daca nu am ajuns la finalul fisierului, intorc 0.
    if (stream->sfarsit_de_fisier == 0)
        return 0;

    // Daca am ajuns la finalul fisierului, intorc 1.
    return 1;
}

int so_ferror(SO_FILE *stream)
{
    // Daca intampin vreo eroare, introc 1.
    if (stream->eroare == 1)
        return 1;

    // Daca nu intampin nicio eroare, intorc 0.
    return 0;
}

SO_FILE *so_popen(const char *command, const char *type)
{
    return NULL;
}

int so_pclose(SO_FILE *stream)
{
    return 0;
}
