#include "AmigaMemoryCopy.h"
extern "C" void* memcpy(void* destination,const void* source,std::size_t length){
    return downwellMemoryCopy(destination,source,length);
}
