#include "cesar.h"
#include <unistd.h>
#include <fcntl.h> // read, write, close
#include <errno.h> // open
#include <stdio.h> //Solo para perror y printf
#include <stdlib.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

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

#define MAX_FILES_CESAR 512
static const char MAGIC_CSAR[8] = "CSAR1000";

typedef struct { char path[1024]; char temp[64]; long size; } CesarFile;
static CesarFile cesar_files[MAX_FILES_CESAR];
static int cesar_file_count = 0;
static int cesar_next_job = 0;
static unsigned char cesar_key_global = 0;
static pthread_mutex_t cesar_lock = PTHREAD_MUTEX_INITIALIZER;

// Worker de cada hilo para encriptar
static void* cesar_worker(void *arg) {
    char *base = (char*)arg;
    while (1) {
        pthread_mutex_lock(&cesar_lock);
        if (cesar_next_job >= cesar_file_count) { pthread_mutex_unlock(&cesar_lock); break; }
        int idx = cesar_next_job++;
        pthread_mutex_unlock(&cesar_lock);
        
        sprintf(cesar_files[idx].temp, ".ctmp_%d.ces", idx);
        char full[2048];
        sprintf(full, "%s/%s", base, cesar_files[idx].path);
        cesar_encrypt_file(full, cesar_files[idx].temp, cesar_key_global);
        struct stat st;
        if (stat(cesar_files[idx].temp, &st) == 0) cesar_files[idx].size = st.st_size;
    }
    return NULL;
}

// Escaneo recursivo de carpetas
static void cesar_scan_recursive(const char *base, const char *rel);

static void cesar_scan(const char *dir) {
    cesar_scan_recursive(dir, "");
}

static void cesar_scan_recursive(const char *base, const char *rel) {
    char path[2048];
    sprintf(path, "%s%s%s", base, rel[0] ? "/" : "", rel);
    
    DIR *d = opendir(path);
    if (!d) return;
    struct dirent *e;
    while ((e = readdir(d))) {
        if (e->d_name[0] == '.' || cesar_file_count >= MAX_FILES_CESAR) continue;
        
        char full[2048], relpath[1024];
        sprintf(full, "%s/%s", path, e->d_name);
        sprintf(relpath, "%s%s%s", rel, rel[0] ? "/" : "", e->d_name);
        
        struct stat st;
        if (stat(full, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                cesar_scan_recursive(base, relpath);
            } else if (S_ISREG(st.st_mode)) {
                strncpy(cesar_files[cesar_file_count].path, relpath, sizeof(cesar_files[0].path)-1);
                cesar_files[cesar_file_count].path[sizeof(cesar_files[0].path)-1] = 0;
                cesar_files[cesar_file_count].size = st.st_size;
                cesar_file_count++;
            }
        }
    }
    closedir(d);
}

int cesar_encrypt_directory(const char *input_path, const char *output_path, unsigned char key, int num_threads) {
    cesar_file_count = 0; cesar_next_job = 0; cesar_key_global = key;
    cesar_scan(input_path);
    if (cesar_file_count == 0) { printf("Carpeta vacía\n"); return -1; }
    printf("Encriptando %d archivos con César (clave=%u) usando %d hilos...\n", 
           cesar_file_count, (unsigned)key, num_threads);
    
    pthread_t threads[32];
    int nt = num_threads > 32 ? 32 : num_threads;
    for (int i = 0; i < nt; i++) pthread_create(&threads[i], NULL, cesar_worker, (void*)input_path);
    for (int i = 0; i < nt; i++) pthread_join(threads[i], NULL);
    
    // Empaquetar en .csar
    int fd = open(output_path, O_CREAT|O_TRUNC|O_WRONLY, 0644);
    if (fd < 0) return -1;
    write(fd, MAGIC_CSAR, 8);
    write(fd, &key, 1);
    unsigned int count = cesar_file_count;
    write(fd, &count, 4);
    
    char buf[8192];
    for (int i = 0; i < cesar_file_count; i++) {
        unsigned short plen = strlen(cesar_files[i].path);
        unsigned long size = cesar_files[i].size;
        write(fd, &plen, 2);
        write(fd, cesar_files[i].path, plen);
        write(fd, &size, 8);
        
        int tf = open(cesar_files[i].temp, O_RDONLY);
        if (tf >= 0) {
            ssize_t n;
            while ((n = read(tf, buf, sizeof(buf))) > 0) write(fd, buf, n);
            close(tf);
            unlink(cesar_files[i].temp);
        }
    }
    close(fd);
    printf("OK: %s\n", output_path);
    return 0;
}

int is_csar_archive(const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return 0;
    char m[8];
    int r = read(fd, m, 8) == 8 && memcmp(m, MAGIC_CSAR, 8) == 0;
    close(fd);
    return r;
}

static void cesar_mkdirs(const char *path) {
    char tmp[1024];
    strncpy(tmp, path, sizeof(tmp)-1);
    tmp[sizeof(tmp)-1] = 0;
    for (char *p = tmp; *p; p++) {
        if (*p == '/' && p != tmp) {
            *p = 0;
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    mkdir(tmp, 0755);  // crear el último directorio también
}

int cesar_decrypt_directory(const char *input_path, const char *output_path, unsigned char key) {
    int fd = open(input_path, O_RDONLY);
    if (fd < 0) return -1;
    char m[8];
    unsigned char stored_key;
    unsigned int count;
    if (read(fd, m, 8) != 8 || memcmp(m, MAGIC_CSAR, 8) != 0) { close(fd); return -1; }
    if (read(fd, &stored_key, 1) != 1) { close(fd); return -1; }
    if (stored_key != key) {
        fprintf(stderr, "Error: clave incorrecta (archivo=%u, provista=%u)\n", 
                (unsigned)stored_key, (unsigned)key);
        close(fd); return -1;
    }
    if (read(fd, &count, 4) != 4) { close(fd); return -1; }
    
    mkdir(output_path, 0755);
    printf("Desencriptando %u archivos con César (clave=%u)...\n", count, (unsigned)key);
    
    char buf[8192];
    for (unsigned int i = 0; i < count; i++) {
        unsigned short plen;
        unsigned long size;
        if (read(fd, &plen, 2) != 2) break;
        
        char path[1024];
        if (read(fd, path, plen) != plen) break;
        path[plen] = 0;
        if (read(fd, &size, 8) != 8) break;
        
        char out[2048], tmp[64];
        sprintf(out, "%s/%s", output_path, path);
        sprintf(tmp, ".dctmp_%u.ces", i);
        
        // Crear directorios intermedios incluyendo output_path
        char dir_path[2048];
        strncpy(dir_path, out, sizeof(dir_path)-1);
        dir_path[sizeof(dir_path)-1] = 0;
        char *slash = strrchr(dir_path, '/');
        if (slash) {
            *slash = 0;
            cesar_mkdirs(dir_path);
        }
        
        int tf = open(tmp, O_CREAT|O_TRUNC|O_WRONLY, 0644);
        unsigned long left = size;
        while (left > 0) {
            size_t n = left > sizeof(buf) ? sizeof(buf) : left;
            if (read(fd, buf, n) != (ssize_t)n) break;
            write(tf, buf, n);
            left -= n;
        }
        close(tf);
        cesar_decrypt_file(tmp, out, key);
        unlink(tmp);
    }
    close(fd);
    printf("OK: %s\n", output_path);
    return 0;
}
