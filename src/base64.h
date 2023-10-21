#ifndef BASE46_H
#define BASE46_H

#include <stdlib.h>
#include <memory.h>
 
char *base64_encode(const unsigned char *data, size_t input_length);

 
char* base64_decode(char* cipher);


#endif //BASE46_H