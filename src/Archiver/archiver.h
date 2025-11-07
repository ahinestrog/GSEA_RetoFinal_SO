// archiver.h - Soporte para comprimir/descomprimir carpetas con hilos

// Comprime una carpeta completa usando hilos:
// - input_path: ruta de la carpeta a comprimir
// - output_path: archivo .har de salida (Huffman ARchive)
// - num_threads: cantidad de hilos para comprimir archivos en paralelo
// Devuelve 0 si OK, -1 si error
int compress_directory(const char *input_path, const char *output_path, int num_threads);

// Descomprime un archivo .har a una carpeta:
// - input_path: archivo .har a descomprimir
// - output_path: carpeta destino donde se extraerán los archivos
// Devuelve 0 si OK, -1 si error
int decompress_directory(const char *input_path, const char *output_path);

// Verifica si un archivo es un .har válido
// Devuelve 1 si es .har, 0 si no lo es, -1 si error
int is_har_archive(const char *path);
