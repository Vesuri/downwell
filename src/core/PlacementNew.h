#ifndef DOWNWELL_PLACEMENT_NEW_H
#define DOWNWELL_PLACEMENT_NEW_H
#ifdef DOWNWELL_FREESTANDING
#include <cstddef>
inline void* operator new(std::size_t,void* address) noexcept{return address;}
#else
#include <new>
#endif
#endif
