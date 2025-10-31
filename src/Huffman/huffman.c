#include "huffman.h"

#include <unistd.h>   // read, write, close, lseek
#include <fcntl.h>    // open, O_RDONLY, O_WRONLY, O_CREAT...
#include <stdlib.h>   // malloc, free, exit
#include <string.h>   // memset, memcpy
#include <stdio.h>    // solo para pse utiliza para imprimir por consola

// Inicializa el heap vacío
static void heap_init(MinHeap *h) {
    h->size = 0;
}

// Intercambia dos punteros dentro del heap
static void heap_swap(Node **a, Node **b) {
    Node *tmp = *a;
    *a = *b;
    *b = tmp;
}

// Inserta un nodo en el heap manteniendo orden por frecuencia (min-heap), siempre los de menos frecuencia
static void heap_insert(MinHeap *h, Node *node) {
    int i = h->size;
    h->data[i] = node;
    h->size++;

    // mientras el nodo sea más pequeño que su padre, sube
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (h->data[parent]->freq <= h->data[i]->freq) {
            break;
        }
        heap_swap(&h->data[parent], &h->data[i]);
        i = parent;
    }
}

// Saca el nodo con menor frecuencia del heap
static Node* heap_extract_min(MinHeap *h) {
    if (h->size == 0) return NULL;

    Node *min = h->data[0];          // raíz del heap = mínimo
    h->size--;
    h->data[0] = h->data[h->size];   // movemos el último a la raíz

    // baja el nodo hasta que cumpla propiedad de heap
    int i = 0;
    while (1) {
        int left  = 2*i + 1;
        int right = 2*i + 2;
        int smallest = i;

        if (left < h->size && h->data[left]->freq < h->data[smallest]->freq) {
            smallest = left;
        }
        if (right < h->size && h->data[right]->freq < h->data[smallest]->freq) {
            smallest = right;
        }
        if (smallest == i) {
            break;
        }
        heap_swap(&h->data[i], &h->data[smallest]);
        i = smallest;
    }

    return min;
}


// Crea un nodo
static Node* make_node(unsigned char byte, uint64_t freq, Node *left, Node *right) {
    Node *n = (Node*) malloc(sizeof(Node));
    if (!n) {
        perror("malloc");
        exit(1);
    }
    n->byte = byte;
    n->freq = freq;
    n->left = left;
    n->right = right;
    return n;
}

// Libera recursivamente el árbol
static void free_tree(Node *root) {
    if (!root) return;
    free_tree(root->left);
    free_tree(root->right);
    free(root);
}


// Se construye el árbol con respecto a sus frecuencias

static Node* build_huffman_tree(uint64_t freq[256]) {
    MinHeap heap;
    heap_init(&heap);

    // Por cada byte (letra/símbolo) que apareció al menos una vez, creamos un nodo hoja y lo metemos al heap.
    for (int b = 0; b < 256; b++) {
        if (freq[b] > 0) {
            Node *leaf = make_node((unsigned char)b, freq[b], NULL, NULL);
            heap_insert(&heap, leaf);
        }
    }

    if (heap.size == 0) {
        return NULL;
    }

    // Cuando solo hay una letra/simbolo/etc en el archivo
    if (heap.size == 1) {
        Node *only = heap_extract_min(&heap);
        Node *dummy = make_node(0, 0, NULL, NULL);
        Node *parent = make_node(0, only->freq, only, dummy);
        return parent;
    }

    // Mientras haya más de un nodo sacar los 2 más pequeños en frecuencia y combinarlos en un nodo padre
    while (heap.size > 1) {
        Node *a = heap_extract_min(&heap);
        Node *b = heap_extract_min(&heap);

        Node *parent = make_node(
            0,              
            a->freq + b->freq,
            a,              // hijo izquierdo
            b               // hijo derecho
        );

        heap_insert(&heap, parent);
    }

    // El único nodo que queda es la raíz del árbol.
    return heap_extract_min(&heap);
}

// # Creamos el archivo .huff. Donde un Byte -> secuencia de bits (8 bits)
// Recorremos el árbol. Cada vez que bajamos a la izquierda agregamos un 0, a la derecha agregamos un 1.
static void build_codes_rec(Node *root,
                            Code codes[256],
                            uint32_t curr_code,
                            uint32_t curr_len) {
    if (!root) return;

    // Si es hoja, registramos su código.
    if (root->left == NULL && root->right == NULL) {
        codes[root->byte].code   = curr_code;
        codes[root->byte].length = curr_len;
        return;
    }

    // Ir a la izquierda: agregamos un 0
    build_codes_rec(root->left,
                    codes,
                    (curr_code << 1) | 0,
                    curr_len + 1);

    // Ir a la derecha: agregamos un 1
    build_codes_rec(root->right,
                    codes,
                    (curr_code << 1) | 1,
                    curr_len + 1);
}

static void build_codes(Node *root, Code codes[256]) {
    for (int i = 0; i < 256; i++) {
        codes[i].code = 0;
        codes[i].length = 0;
    }
    build_codes_rec(root, codes, 0, 0);
}

// Esta estructura acumula bits hasta formar bytes
typedef struct {
    int fd;            // file descriptor POSIX de salida -> Identificador único dentro del sistema operativo que le permite al kernel saber que archivo le estas diciendo que abra
    uint8_t buffer;    // acumulador de bits
    int bit_count;     // cuántos bits hay ya en buffer
} BitWriter;

// Inicializa el BitWriter
static void bw_init(BitWriter *bw, int fd) {
    bw->fd = fd;
    bw->buffer = 0;
    bw->bit_count = 0;
}

// Escribe un bit (0 o 1) en el buffer interno.
// Cuando acumulamos 8 bits, escribimos 1 byte al archivo con write().
static void bw_write_bit(BitWriter *bw, int bit) {
    bw->buffer = (bw->buffer << 1) | (bit & 1);
    bw->bit_count++;

    if (bw->bit_count == 8) {
        // ya tenemos un byte completo
        ssize_t w = write(bw->fd, &bw->buffer, 1);
        if (w != 1) {
            perror("write");
            exit(1);
        }
        bw->bit_count = 0;
        bw->buffer = 0;
    }
}

static void bw_write_code(BitWriter *bw, uint32_t code, uint32_t length) {
    // Recorremos de más significativo a menos significativo.
    for (int i = length - 1; i >= 0; i--) {
        int bit = (code >> i) & 1;
        bw_write_bit(bw, bit);
    }
}

// Al final, si hay bits "sueltos", los empujamos alineando con ceros a la derecha.
static void bw_flush(BitWriter *bw) {
    if (bw->bit_count > 0) {
        bw->buffer <<= (8 - bw->bit_count);

        ssize_t w = write(bw->fd, &bw->buffer, 1);
        if (w != 1) {
            perror("write");
            exit(1);
        }
        bw->bit_count = 0;
        bw->buffer = 0;
    }
}

// Bit reader 
typedef struct {
    int fd;            // file descriptor POSIX de entrada
    uint8_t buffer;    // byte actual
    int bit_count;     // cuántos bits quedan sin leer en el buffer
    int eof;           // marcamos si ya no hay más bits que leer (acabamos de leer el archivo)
} BitReader;

static void br_init(BitReader *br, int fd) {
    br->fd = fd;
    br->buffer = 0;
    br->bit_count = 0;
    br->eof = 0;
}

// Devuelve 1 si pudo leer un bit y lo pone en *bit_out.
// Devuelve 0 si ya no hay más bits (eof en el archivo comprimido .huff)
static int br_read_bit(BitReader *br, int *bit_out) {
    // Si ya le+imos los bits del buffer anterior, leer un byte nuevo
    if (br->bit_count == 0) {
        uint8_t tmp;
        ssize_t r = read(br->fd, &tmp, 1);
        if (r == 0) {
            // no más datos en el archivo
            br->eof = 1;
            return 0;
        } else if (r < 0) {
            perror("read");
            exit(1);
        }
        br->buffer = tmp;
        br->bit_count = 8;
    }

    // Obtener el bit más significativo del buffer
    int bit = (br->buffer >> 7) & 1;
    br->buffer <<= 1;
    br->bit_count--;

    *bit_out = bit;
    return 1;
}


// Escribir header al archivo de salida
static void write_header(int fd_out, uint64_t freq[256]) {
    for (int i = 0; i < 256; i++) {
        uint64_t f = freq[i];
        ssize_t w = write(fd_out, &f, sizeof(uint64_t));
        if (w != sizeof(uint64_t)) {
            perror("write header");
            exit(1);
        }
    }
}

// Leer header del archivo comprimido
static void read_header(int fd_in, uint64_t freq[256]) {
    for (int i = 0; i < 256; i++) {
        uint64_t f;
        ssize_t r = read(fd_in, &f, sizeof(uint64_t));
        if (r != sizeof(uint64_t)) {
            perror("read header");
            exit(1);
        }
        freq[i] = f;
    }
}


// Devuelve 0 si todo bien, -1 si error al abrir archivo
int compress_file(const char *input_path, const char *output_path) {
    // 1. Abrir archivo de entrada con open()
    int fd_in = open(input_path, O_RDONLY);
    if (fd_in < 0) {
        perror("open input");
        return -1;
    }

    // 2. Contar frecuencias de cada byte (0..255)
    uint64_t freq[256];
    memset(freq, 0, sizeof(freq));

    uint8_t buffer[4096];
    while (1) {
        ssize_t r = read(fd_in, buffer, sizeof(buffer));
        if (r < 0) {
            perror("read input");
            close(fd_in);
            return -1;
        }
        if (r == 0) {
            // EOF
            break;
        }
        for (ssize_t i = 0; i < r; i++) {
            freq[ buffer[i] ]++;
        }
    }

    // 3. Construir árbol Huffman
    Node *root = build_huffman_tree(freq);

    // 4. Abrir archivo de salida
    int fd_out = open(output_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_out < 0) {
        perror("open output");
        close(fd_in);
        free_tree(root);
        return -1;
    }

    // 5. Escribir header (tabla de frecuencias)
    write_header(fd_out, freq);

    // 6. Si el archivo estaba vacío, terminamos
    if (root == NULL) {
        close(fd_in);
        close(fd_out);
        return 0;
    }

    // 7. Generar tabla de códigos Huffman
    Code codes[256];
    build_codes(root, codes);

    // 8. Regresar al inicio del archivo original para volverlo a leer
    off_t off = lseek(fd_in, 0, SEEK_SET);
    if (off == (off_t)-1) {
        perror("lseek");
        close(fd_in);
        close(fd_out);
        free_tree(root);
        return -1;
    }

    // 9. Escribir los bits comprimidos
    BitWriter bw;
    bw_init(&bw, fd_out);

    while (1) {
        ssize_t r = read(fd_in, buffer, sizeof(buffer));
        if (r < 0) {
            perror("read input 2");
            close(fd_in);
            close(fd_out);
            free_tree(root);
            return -1;
        }
        if (r == 0) break; // EOF

        for (ssize_t i = 0; i < r; i++) {
            uint8_t b = buffer[i];
            bw_write_code(&bw, codes[b].code, codes[b].length);
        }
    }

    bw_flush(&bw);
    close(fd_in);
    close(fd_out);
    free_tree(root);

    return 0;
}

// Devuelve 0 si todo bien, -1 si error al abrir archivo
int decompress_file(const char *input_path, const char *output_path) {
    // 1. Abrir archivo .huff para lectura
    int fd_in = open(input_path, O_RDONLY);
    if (fd_in < 0) {
        perror("open input");
        return -1;
    }

    // 2. Leer header (frecuencias) desde el archivo comprimido
    uint64_t freq[256];
    read_header(fd_in, freq);

    // 3. Reconstruir el mismo árbol Huffman
    Node *root = build_huffman_tree(freq);

    // 4. Calcular cuántos bytes originales debemos recrear, es decir, la suma de todas las frecunecias (Nodo raíz)
    uint64_t total_bytes = 0;
    for (int i = 0; i < 256; i++) {
        total_bytes += freq[i];
    }

    // 5. Abrir archivo de salida para escribir bytes restaurados
    int fd_out = open(output_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_out < 0) {
        perror("open output");
        close(fd_in);
        free_tree(root);
        return -1;
    }

    // 6. Si no había datos en el original, terminamos
    if (root == NULL || total_bytes == 0) {
        close(fd_in);
        close(fd_out);
        free_tree(root);
        return 0;
    }

    // 7. Leer bit por bit y recorrer el árbol
    BitReader br;
    br_init(&br, fd_in);

    uint64_t written = 0;
    while (written < total_bytes) {
        Node *curr = root;

        // bajar por el árbol hasta llegar a una hoja
        while (curr->left != NULL || curr->right != NULL) {
            int bit;
            if (!br_read_bit(&br, &bit)) {
                fprintf(stderr, "Error: datos comprimidos insuficientes\n");
                close(fd_in);
                close(fd_out);
                free_tree(root);
                return -1;
            }

            if (bit == 0) {
                curr = curr->left;
            } else {
                curr = curr->right;
            }
        }

        uint8_t original_byte = curr->byte;

        ssize_t w = write(fd_out, &original_byte, 1);
        if (w != 1) {
            perror("write output");
            close(fd_in);
            close(fd_out);
            free_tree(root);
            return -1;
        }

        written++;
    }

    close(fd_in);
    close(fd_out);
    free_tree(root);

    return 0;
}
