// cesar.h - Cifrado César
#ifndef CESAR_H
#define CESAR_H

// Encriptar/desencriptar archivo individual
int cesar_encrypt_file(const char *input_path, const char *output_path, unsigned char key);
int cesar_decrypt_file(const char *input_path, const char *output_path, unsigned char key);

// Encriptar/desencriptar carpeta con hilos (crea .csar = César Archive)
int cesar_encrypt_directory(const char *input_path, const char *output_path, unsigned char key, int num_threads);
int cesar_decrypt_directory(const char *input_path, const char *output_path, unsigned char key);

// Detectar si es un archivo .csar
int is_csar_archive(const char *path);

#endif // CESAR_H