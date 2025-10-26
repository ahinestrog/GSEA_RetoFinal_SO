
#include <stdio.h>
#include <unistd.h>   
#include <fcntl.h>    
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Compresión RLE
int compress_rle(const char *input, const char *output) {
    int fd_in = open(input, O_RDONLY);
    if (fd_in < 0) {
        perror("open input");
        return -1;
    }

    int fd_out = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_out < 0) {
        perror("open output");
        close(fd_in);
        return -1;
    }

    unsigned char prev, curr;
    unsigned char count = 0;
    int have_prev = 0;
    unsigned char buf[4096];
    ssize_t r;

    while ((r = read(fd_in, buf, sizeof(buf))) > 0) {
        for (ssize_t i = 0; i < r; ++i) {
            curr = buf[i];
            if (!have_prev) {
                prev = curr;
                count = 1;
                have_prev = 1;
            } else if (curr == prev && count < 255) {
                count++;
            } else {
                unsigned char pair[2] = {count, prev};
                if (write(fd_out, pair, 2) != 2) {
                    perror("write");
                    close(fd_in);
                    close(fd_out);
                    return -1;
                }
                prev = curr;
                count = 1;
            }
        }
    }

    if (r < 0) {
        perror("read");
    }

    // escribir el último bloque
    if (have_prev) {
        unsigned char pair[2] = {count, prev};
        write(fd_out, pair, 2);
    }

    close(fd_in);
    close(fd_out);
    return 0;
}

// Descompresión RLE
int decompress_rle(const char *input, const char *output) {
    int fd_in = open(input, O_RDONLY);
    if (fd_in < 0) {
        perror("open input");
        return -1;
    }

    int fd_out = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_out < 0) {
        perror("open output");
        close(fd_in);
        return -1;
    }

    unsigned char pair[2];
    unsigned char block[256];
    ssize_t r;

    while ((r = read(fd_in, pair, 2)) == 2) {
        unsigned char count = pair[0];
        unsigned char value = pair[1];
        memset(block, value, count);
        if (write(fd_out, block, count) != count) {
            perror("write");
            close(fd_in);
            close(fd_out);
            return -1;
        }
    }

    if (r < 0) {
        perror("read");
    }

    close(fd_in);
    close(fd_out);
    return 0;
}


int main(int argc, char *argv[]) {
    if (argc < 5) {
        printf("Uso: %s [-c | -d] -i <input> -o <output>\n", argv[0]);
        return 1;
    }

    int compress = 0, decompress = 0;
    const char *in = NULL, *out = NULL;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-c") == 0) compress = 1;
        else if (strcmp(argv[i], "-d") == 0) decompress = 1;
        else if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) in = argv[++i];
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) out = argv[++i];
    }

    if (!in || !out) {
        fprintf(stderr, "Debes especificar archivos con -i y -o.\n");
        return 1;
    }

    if (compress && decompress) {
        fprintf(stderr, "Solo una opción (-c o -d).\n");
        return 1;
    }

    int res = 0;
    if (compress) {
        printf("🔹 Comprimiendo %s → %s\n", in, out);
        res = compress_rle(in, out);
    } else if (decompress) {
        printf("🔹 Descomprimiendo %s → %s\n", in, out);
        res = decompress_rle(in, out);
    } else {
        fprintf(stderr, "Debes indicar -c o -d.\n");
        return 1;
    }

    if (res == 0) printf("Operación completada correctamente.\n");
    else printf("Error en la operación.\n");

    return res;
}
