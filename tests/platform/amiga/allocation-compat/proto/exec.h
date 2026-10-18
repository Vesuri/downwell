#ifndef DOWNWELL_TEST_EXEC_ALLOCATION_H
#define DOWNWELL_TEST_EXEC_ALLOCATION_H
#include <cstddef>
void* AllocMem(std::size_t,unsigned);
void FreeMem(void*,std::size_t);
#endif
