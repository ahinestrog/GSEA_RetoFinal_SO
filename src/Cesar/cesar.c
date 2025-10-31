#include "cesar.h"
#include <unistd.h>
#include <fcntl.h> // read, write, close
#include <errno.h> // open
#include <stdio.h> //Solo para perror y printf
#include <stdlib.h>

// Funcion que aplica la transformacion César a un buffer
static void cesar_transform_buffer(unsigned char *buf, ssize_t len, unsigned char key, int decrypt){
    for (ssize_t i = 0; i < len; i++){
        if (!decrypt){
            // Se usa & 0xFF para asegurar que el resultado se mantenga en el rango de un byte (0-255.
            // Es equivalente a hacer un modulo 256 pero más eficiente ya que no realiza divisiones.
            buf[i] = (unsigned char)((buf[i] + key) & 0xFF);
        }
        else{
            buf[i] = (unsigned char)((buf[i] - key) & 0xFF);
        }
    }
}


static int cesar_do(const char *input_path, const char *output_path, unsigned char key, int decrypt){
    int fd_in = open(input_path, O_RDONLY);
    if (fd_in < 0){
        perror("open input");
        return -1;
    }

    int fd_out = open(output_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_out < 0){
        perror("open output");
        close(fd_in);
        return -1;
    }

    unsigned char buf[4096];
    while (1){
        ssize_t r = read(fd_in, buf, sizeof(buf));
        if (r < 0){
            perror("read input");
            close(fd_in);
            close(fd_out);
            return -1;
        }

        if (r == 0){
            break;
        }

        cesar_transform_buffer(buf, r, key, decrypt);

        // Asegurarse de escribir todos los bytes leídos, si en una llamada a write() falla en escribir todos los bytes se suma a la variable written la cantidad de lineas que logro escribir y se intenta escribir el resto hasta completar r bytes (bytes leidos)
        ssize_t written = 0;
        while (written < r){
            ssize_t w = write(fd_out, buf + written, r - written);
            if (w < 0){
                if (errno == EINTR){
                    continue;
                }
                perror("write output");
                close(fd_in);
                close(fd_out);
                return -1;
            }
            written += w;
        }
    }
    
    close(fd_in);
    close(fd_out);
    return 0;
}

int cesar_encrypt_file(const char *input_path, const char *output_path, unsigned char key){
    return cesar_do(input_path, output_path, key, 0);
}

int cesar_decrypt_file(const char *input_path, const char *output_path, unsigned char key){
    return cesar_do(input_path, output_path, key, 1);
}
