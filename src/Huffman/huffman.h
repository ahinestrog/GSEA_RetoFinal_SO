#include <stdint.h>
#include <stddef.h> 

// Cada nodo del árbol de Huffman
typedef struct Node {
    unsigned char byte;      // el valor del byte (si el nodo es hoja)
    uint64_t freq;           // cuántas veces aparece esa letra en el árbol
    struct Node *left;       // hijo izquierdo (puntero)
    struct Node *right;      // hijo derecho (puntero)
} Node;


// Cola de prioridad mínima (min-heap) para construir el árbol.
// Siempre sacamos los 2 nodos con menor frecuencia y los unimos en un nodo padre, cuya frecuencia es la suma de los dos
typedef struct {
    Node* data[256]; // soportamos hasta 256 símbolos distintos
    int size;        // cuántos nodos hay actualmente en el heap
} MinHeap;


typedef struct {
    uint32_t code;    // bits del código
    uint32_t length;  // cuántos bits del código son válidos
} Code;


int compress_file(const char *input_path, const char *output_path);
int decompress_file(const char *input_path, const char *output_path);