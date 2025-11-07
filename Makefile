CC = gcc # Compilador de C

CFLAGS =  -Wall -Wextra -O2 -pthread -Isrc/Huffman -Isrc/Cesar -Isrc/Archiver # -Wall y -Wextra para advertencias, y 

TARGET = gsea  # Nombre del ejecutable

# Archivos fuente del proyecto
SRC = src/main.c src/Huffman/huffman.c src/Cesar/cesar.c src/Archiver/archiver.c


# # Archivos .o que generará el compilador
OBJ = $(patsubst %.c,%.o,$(SRC))

all: $(TARGET) # Lo que se ejecuta si hago el comando "make", que lo utilizo para correr todo el proyecto


# Contruir el ejecutable a partir de los .o
# $@ = nombre del archivo que será el ejecutable (gsea)
# $^ = toma los código fuente: main.o huffman.o cesar.o
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Cómo compilar cada archivo .c en su .o
# Toma el .c (entrada) $< y lo pone en su archivo de salida .o ($@)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -f $(OBJ) $(TARGET)
	find . -name "*.o" -type f -delete
	find . -name "*.huff" -type f -delete
	find . -name "*.har" -type f -delete
	find . -name "*.csar" -type f -delete
	find . -name "*.ces" -type f -delete
	find . -name "*.out" -type f -delete
	rm -rf -- carpeta_prueba_salida carpeta_prueba_salida_huffman carpeta_prueba_salida_cesar

# Corre ejemplos: Huffman + César
run: all
	@echo "=== Huffman: archivo individual ==="
	./$(TARGET) -c test.txt test.huff
	./$(TARGET) -d test.huff test_huffman.out
	@echo "=== Huffman: carpeta con hilos ==="
	./$(TARGET) -c carpeta_prueba/ paquete_huffman.har -t 4
	./$(TARGET) -d paquete_huffman.har carpeta_prueba_salida_huffman
	@echo "=== César: encriptar/desencriptar archivo ==="
	./$(TARGET) -e test.txt test.ces -k 42
	./$(TARGET) -u test.ces test_cesar.out -k 42
	@echo "=== César: encriptar/desencriptar carpeta con hilos ==="
	./$(TARGET) -e carpeta_prueba/ paquete_cesar.csar -k 42 -t 4
	./$(TARGET) -u paquete_cesar.csar carpeta_prueba_salida_cesar -k 42
	@echo "✓ Todos los tests ejecutados"