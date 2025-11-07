#define _XOPEN_SOURCE 700
#include "archiver.h"
#include "../Huffman/huffman.h"
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_FILES 512
static const char MAGIC[8] = "GSHAR100";

typedef struct { char path[1024]; char temp[64]; long size; } File;
static File files[MAX_FILES];
static int file_count = 0;
static int next_job = 0;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static void scan_recursive(const char *base, const char *rel);

// Función worker de cada hilo
static void* worker(void *arg) {
    char *base = (char*)arg;
    while (1) {
        pthread_mutex_lock(&lock);
        if (next_job >= file_count) { pthread_mutex_unlock(&lock); break; }
        int idx = next_job++;
        pthread_mutex_unlock(&lock);
        
        sprintf(files[idx].temp, ".tmp_%d.huf", idx);
        char full[2048];
        sprintf(full, "%s/%s", base, files[idx].path);
        compress_file(full, files[idx].temp);
        struct stat st;
        if (stat(files[idx].temp, &st) == 0) files[idx].size = st.st_size;
    }
    return NULL;
}

// Escaneo recursivo de carpetas
static void scan(const char *dir) {
    scan_recursive(dir, "");
}

static void scan_recursive(const char *base, const char *rel) {
    char path[2048];
    sprintf(path, "%s%s%s", base, rel[0] ? "/" : "", rel);
    
    DIR *d = opendir(path);
    if (!d) return;
    struct dirent *e;
    while ((e = readdir(d))) {
        if (e->d_name[0] == '.' || file_count >= MAX_FILES) continue;
        
        char full[2048], relpath[1024];
        sprintf(full, "%s/%s", path, e->d_name);
        sprintf(relpath, "%s%s%s", rel, rel[0] ? "/" : "", e->d_name);
        
        struct stat st;
        if (stat(full, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                scan_recursive(base, relpath);
            } else if (S_ISREG(st.st_mode)) {
                strncpy(files[file_count].path, relpath, sizeof(files[0].path)-1);
                files[file_count].path[sizeof(files[0].path)-1] = 0;
                files[file_count].size = st.st_size;
                file_count++;
            }
        }
    }
    closedir(d);
}

int compress_directory(const char *input_path, const char *output_path, int num_threads) {
    file_count = 0; next_job = 0;
    scan(input_path);
    if (file_count == 0) { printf("Carpeta vacía\n"); return -1; }
    printf("Comprimiendo %d archivos con %d hilos...\n", file_count, num_threads);
    
    pthread_t threads[32];
    int nt = num_threads > 32 ? 32 : num_threads;
    for (int i = 0; i < nt; i++) pthread_create(&threads[i], NULL, worker, (void*)input_path);
    for (int i = 0; i < nt; i++) pthread_join(threads[i], NULL);
    
    int fd = open(output_path, O_CREAT|O_TRUNC|O_WRONLY, 0644);
    if (fd < 0) return -1;
    write(fd, MAGIC, 8);
    unsigned int count = file_count;
    write(fd, &count, 4);
    
    char buf[8192];
    for (int i = 0; i < file_count; i++) {
        unsigned char type = 0;
        unsigned short plen = strlen(files[i].path);
        unsigned long size = files[i].size;
        write(fd, &type, 1);
        write(fd, &plen, 2);
        write(fd, files[i].path, plen);
        write(fd, &size, 8);
        
        int tf = open(files[i].temp, O_RDONLY);
        if (tf >= 0) {
            ssize_t n;
            while ((n = read(tf, buf, sizeof(buf))) > 0) write(fd, buf, n);
            close(tf);
            unlink(files[i].temp);
        }
    }
    close(fd);
    printf("OK: %s\n", output_path);
    return 0;
}

int is_har_archive(const char *path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return 0;
    char m[8];
    int r = read(fd, m, 8) == 8 && memcmp(m, MAGIC, 8) == 0;
    close(fd);
    return r;
}

static void mkdirs(const char *path) {
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

int decompress_directory(const char *input_path, const char *output_path) {
    int fd = open(input_path, O_RDONLY);
    if (fd < 0) return -1;
    char m[8];
    unsigned int count;
    if (read(fd, m, 8) != 8 || memcmp(m, MAGIC, 8) != 0 || read(fd, &count, 4) != 4) {
        close(fd); return -1;
    }
    mkdir(output_path, 0755);
    printf("Extrayendo %u archivos...\n", count);
    
    char buf[8192];
    for (unsigned int i = 0; i < count; i++) {
        unsigned char type;
        unsigned short plen;
        unsigned long size;
        if (read(fd, &type, 1) != 1 || read(fd, &plen, 2) != 2) break;
        
        char path[1024];
        if (read(fd, path, plen) != plen) break;
        path[plen] = 0;
        if (read(fd, &size, 8) != 8) break;
        
        char out[2048], tmp[64];
        sprintf(out, "%s/%s", output_path, path);
        sprintf(tmp, ".ext_%u.huf", i);
        
        // Crear directorios intermedios incluyendo output_path
        char dir_path[2048];
        strncpy(dir_path, out, sizeof(dir_path)-1);
        dir_path[sizeof(dir_path)-1] = 0;
        char *slash = strrchr(dir_path, '/');
        if (slash) {
            *slash = 0;
            mkdirs(dir_path);
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
        decompress_file(tmp, out);
        unlink(tmp);
    }
    close(fd);
    printf("OK: %s\n", output_path);
    return 0;
}
