### DarkSpecter

- Atención: No me hago cargo ni responsable del uso que se le puede dar a este repositorio!
- Recuerda: Está hecho para fines educativos y de aprendizaje.

En desarollo...


# Test Linux

1. Modifica el archivo **client.c** para especificar la dirección IP y el puerto de destino y si es necesario el del servidor:
   1. Cliente:

      ![img.png](img/img.png)

   2. Server:

      ![img_1.png](img/img_1.png)

2.  Compilamos:

    - Cliente:
        `gcc linux/client.c -o linux/client`
    - Server:
        `gcc linux/client.c -o linux/client`


3. Ejecutamos:

- Server:

    ![img_2.png](img/img_2.png)



# Test Windows

1. Modificamos el archivo **client.c** especificando la IP de atacante y el puerto especificado en **server.c**

    ![img_3.png](img/img_3.png)


2. Compilamos:

    - Cliente:
      ` gcc .\client.c -o .\dist\client -lws2_32 -lShlwapi`

    - Server:
      `gcc .\server.c -o .\dist\server -lws2_32 -lShlwapi`


3. Ejecutamos:

- Server:

  ![img_4.png](img/img_4.png)

# Aviable Commands
Por el momento tenemos disponible los siguientes comandos:

    - help            -> Show help message
    - shell           -> Enter shell mode ("q" for exit)
    - exec            -> Execute command without shell mode
    - download <file> -> Download file from target
    - upload <file>   -> Upload local file to target
    - q / exit        -> Exit server
    - q -y / exit -y  -> Exit server and client (close binary) 
