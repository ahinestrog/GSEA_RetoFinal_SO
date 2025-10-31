#include <stdio.h>     // printf, fprintf
#include <string.h>    // strcmp
#include <stdlib.h>    // exit, EXIT_FAILURE, EXIT_SUCCESS

#include "huffman.h"

// Uso:
//   ./gsea -c archivo_original archivo_comprimido.huff
//   ./gsea -d archivo_comprimido.huff archivo_salida

static void print_usage(const char *prog) {
    fprintf(stderr,
        "Uso:\n"
        "  %s -c <input> <output.huff>    Comprimir\n"
        "  %s -d <input.huff> <output>    Descomprimir\n",
        prog, prog
    );
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    const char *flag = argv[1];
    const char *in_path = argv[2];
    const char *out_path = argv[3];

    if (strcmp(flag, "-c") == 0) {
        // Comprimir
        if (compress_file(in_path, out_path) != 0) {
            fprintf(stderr, "Error al comprimir %s\n", in_path);
            return EXIT_FAILURE;
        }
        printf("OK: %s -> %s (comprimido)\n", in_path, out_path);
    }
    else if (strcmp(flag, "-d") == 0) {
        // Descomprimir
        if (decompress_file(in_path, out_path) != 0) {
            fprintf(stderr, "Error al descomprimir %s\n", in_path);
            return EXIT_FAILURE;
        }
        printf("OK: %s -> %s (descomprimido)\n", in_path, out_path);
    }
    else {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
