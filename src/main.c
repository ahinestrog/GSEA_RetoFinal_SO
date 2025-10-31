#include <stdio.h>     // printf, fprintf
#include <string.h>    // strcmp
#include <stdlib.h>    // exit, EXIT_FAILURE, EXIT_SUCCESS

#include "cesar.h"
#include "huffman.h"

// Uso:
//   ./gsea -c archivo_original archivo_comprimido.huff
//   ./gsea -d archivo_comprimido.huff archivo_salida

static void print_usage(const char *prog) {
    fprintf(stderr,
        "Uso:\n"
        "  %s -c <input> <output.huff>    Comprimir\n"
        "  %s -d <input.huff> <output>    Descomprimir\n"
        "  %s -e <input> <output.sec> -k N    Encriptar César con llave N (0-255)\n"
        "  %s -u <input.sec> <output>   -k N    Desencriptar César con la misma llave\n",
        prog, prog, prog, prog
    );
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    const char *flag = argv[1];
    const char *in_path = argv[2];
    const char *out_path = argv[3];

    if (strcmp(flag, "-c") == 0) {
        if (argc != 4) {
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
        // Comprimir
        if (compress_file(in_path, out_path) != 0) {
            fprintf(stderr, "Error al comprimir %s\n", in_path);
            return EXIT_FAILURE;
        }
        printf("OK: %s -> %s (comprimido)\n", in_path, out_path);
    }
    else if (strcmp(flag, "-d") == 0) {
        if (argc != 4) {
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }

        // Descomprimir
        if (decompress_file(in_path, out_path) != 0) {
            fprintf(stderr, "Error al descomprimir %s\n", in_path);
            return EXIT_FAILURE;
        }
        printf("OK: %s -> %s (descomprimido)\n", in_path, out_path);
    }
    else if (strcmp(flag, "-e") == 0){
        if (argc != 6) {
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }

        if (strcmp(argv[4], "-k") != 0) {
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }

        unsigned long key_ul = strtoul(argv[5], NULL, 10);
        unsigned char key = (unsigned char)(key_ul & 0xFF);

        if (cesar_encrypt_file(in_path, out_path, key) != 0) {
            fprintf(stderr, "Error al encriptar %s\n", in_path);
            return EXIT_FAILURE;
        }

        printf("OK: %s -> %s (encriptado con César, key=%u)\n", in_path, out_path, (unsigned)key);
        return EXIT_SUCCESS;
    }
    else if (strcmp(flag, "-u") == 0){
        if (argc != 6) {
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }

        if (strcmp(argv[4], "-k") != 0) {
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
        
        unsigned long key_ul = strtoul(argv[5], NULL, 10);
        unsigned char key = (unsigned char)(key_ul & 0xFF);

        if (cesar_decrypt_file(in_path, out_path, key) != 0) {
            fprintf(stderr, "Error al encriptar %s\n", in_path);
            return EXIT_FAILURE;
        }

        printf("OK: %s -> %s (desencriptado con César, key=%u)\n", in_path, out_path, (unsigned)key);
        return EXIT_SUCCESS;
    }
    else {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
