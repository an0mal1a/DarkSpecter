#ifndef BASE64_H
#define BASE64_H

#include <stdlib.h>
#include <memory.h> 

char *base64_encode(const unsigned char *data,
                    size_t input_length,
                    size_t *output_length);


char* base64_decode(const char *data,
                             size_t input_length,
                             size_t *output_length);


#endif //BASE46_H