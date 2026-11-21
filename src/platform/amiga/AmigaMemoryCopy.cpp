#include "AmigaMemoryCopy.h"
extern "C" void* memcpy(void* destination,const void* source,std::size_t length){
    return downwellMemoryCopy(destination,source,length);
}
extern "C" void* memset(void* destination,int value,std::size_t length){
    return downwellMemoryFill(destination,value,length);
}
