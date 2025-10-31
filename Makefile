CC = gcc # Compilador de C

CFLAGS =  -Wall -Wextra -O2 -pthread -Isrc/Huffman -Isrc/Cesar # -Wall y -Wextra para advertencias, y 

TARGET = gsea  # Nombre del ejecutable

# Archivos fuente del proyecto
SRC = src/main.c src/Huffman/huffman.c src/Cesar/cesar.c


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
	find . -name "*.ces" -type f -delete
	find . -name "*.out" -type f -delete

# Corre un ejemplo con el archivo test.txt
run: all
	./$(TARGET) -c test.txt test.huff
	./$(TARGET) -d test.huff test.out