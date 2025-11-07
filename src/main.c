#include <stdio.h>     // printf, fprintf
#include <string.h>    // strcmp
#include <stdlib.h>    // exit, EXIT_FAILURE, EXIT_SUCCESS
#include <sys/stat.h>  // stat, S_ISDIR

#include "cesar.h"
#include "huffman.h"
#include "archiver.h"  // para comprimir/descomprimir carpetas

// Uso:
//   ./gsea -c <archivo_o_carpeta> <salida>       Comprimir archivo o carpeta
//   ./gsea -d <archivo.huff_o_.har> <salida>     Descomprimir
//   ./gsea -c <carpeta> <salida.har> -t N        Comprimir carpeta con N hilos
//   ./gsea -e <input> <output.sec> -k N          Encriptar César
//   ./gsea -u <input.sec> <output> -k N          Desencriptar César

// Helper: verifica si un path es directorio
static int es_directorio(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return S_ISDIR(st.st_mode);
}

static void print_usage(const char *prog) {
    fprintf(stderr,
        "Uso:\n"
        "  %s -c <input> <output> [-t N]      Comprimir archivo o carpeta\n"
        "  %s -d <input> <output>             Descomprimir\n"
        "  %s -e <input> <output> -k K [-t N] Encriptar César (carpeta o archivo)\n"
        "  %s -u <input> <output> -k K        Desencriptar César\n",
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
        // Comprimir: archivo o carpeta
        // Detectar si hay opción -t para hilos
        int num_hilos = 4;  // valor por defecto
        if (argc >= 6 && strcmp(argv[4], "-t") == 0) {
            num_hilos = atoi(argv[5]);
            if (num_hilos < 1) num_hilos = 1;
        }

        // Si es directorio, usar archivador con hilos
        if (es_directorio(in_path)) {
            if (compress_directory(in_path, out_path, num_hilos) != 0) {
                fprintf(stderr, "Error al comprimir carpeta %s\n", in_path);
                return EXIT_FAILURE;
            }
            printf("OK: carpeta %s -> %s (comprimida con %d hilos)\n", in_path, out_path, num_hilos);
        } else {
            // Es archivo regular, usar Huffman normal
            if (compress_file(in_path, out_path) != 0) {
                fprintf(stderr, "Error al comprimir %s\n", in_path);
                return EXIT_FAILURE;
            }
            printf("OK: %s -> %s (comprimido)\n", in_path, out_path);
        }
    }
    else if (strcmp(flag, "-d") == 0) {
        if (argc != 4) {
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }

        // Descomprimir: detectar si es .har o .huff
        int es_har = is_har_archive(in_path);
        if (es_har < 0) {
            fprintf(stderr, "Error al verificar archivo %s\n", in_path);
            return EXIT_FAILURE;
        }

        if (es_har) {
            // Es un archivo .har (carpeta comprimida)
            if (decompress_directory(in_path, out_path) != 0) {
                fprintf(stderr, "Error al descomprimir carpeta %s\n", in_path);
                return EXIT_FAILURE;
            }
            printf("OK: %s -> %s (carpeta descomprimida)\n", in_path, out_path);
        } else {
            // Es archivo .huff normal
            if (decompress_file(in_path, out_path) != 0) {
                fprintf(stderr, "Error al descomprimir %s\n", in_path);
                return EXIT_FAILURE;
            }
            printf("OK: %s -> %s (descomprimido)\n", in_path, out_path);
        }
    }
    else if (strcmp(flag, "-e") == 0){
        if (argc < 6 || strcmp(argv[4], "-k") != 0) {
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }

        unsigned long key_ul = strtoul(argv[5], NULL, 10);
        unsigned char key = (unsigned char)(key_ul & 0xFF);

        int num_hilos = 4;
        if (argc >= 8 && strcmp(argv[6], "-t") == 0) {
            num_hilos = atoi(argv[7]);
            if (num_hilos < 1) num_hilos = 1;
        }

        if (es_directorio(in_path)) {
            if (cesar_encrypt_directory(in_path, out_path, key, num_hilos) != 0) {
                fprintf(stderr, "Error al encriptar carpeta %s\n", in_path);
                return EXIT_FAILURE;
            }
            printf("OK: %s -> %s (carpeta encriptada con César, key=%u, %d hilos)\n", 
                   in_path, out_path, (unsigned)key, num_hilos);
        } else {
            if (cesar_encrypt_file(in_path, out_path, key) != 0) {
                fprintf(stderr, "Error al encriptar %s\n", in_path);
                return EXIT_FAILURE;
            }
            printf("OK: %s -> %s (encriptado con César, key=%u)\n", in_path, out_path, (unsigned)key);
        }
        return EXIT_SUCCESS;
    }
    else if (strcmp(flag, "-u") == 0){
        if (argc < 6 || strcmp(argv[4], "-k") != 0) {
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
        
        unsigned long key_ul = strtoul(argv[5], NULL, 10);
        unsigned char key = (unsigned char)(key_ul & 0xFF);

        int es_csar = is_csar_archive(in_path);
        if (es_csar < 0) {
            fprintf(stderr, "Error al verificar archivo %s\n", in_path);
            return EXIT_FAILURE;
        }

        if (es_csar) {
            if (cesar_decrypt_directory(in_path, out_path, key) != 0) {
                fprintf(stderr, "Error al desencriptar carpeta %s\n", in_path);
                return EXIT_FAILURE;
            }
            printf("OK: %s -> %s (carpeta desencriptada con César, key=%u)\n", 
                   in_path, out_path, (unsigned)key);
        } else {
            if (cesar_decrypt_file(in_path, out_path, key) != 0) {
                fprintf(stderr, "Error al desencriptar %s\n", in_path);
                return EXIT_FAILURE;
            }
            printf("OK: %s -> %s (desencriptado con César, key=%u)\n", in_path, out_path, (unsigned)key);
        }
        return EXIT_SUCCESS;
    }
    else {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
