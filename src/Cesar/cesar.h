// Funcion para encriptar un archivo usando el cifrado César usando una clave entre 0 y 255 (ya que es para bytes)
int cesar_encrypt_file(const char *input_path, const char *output_path, unsigned char key);

// Funcion para desencriptar un archivo usando el cifrado César
int cesar_decrypt_file(const char *input_path, const char *output_path, unsigned char key);