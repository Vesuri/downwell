inline void* operator new(__SIZE_TYPE__,void* address) noexcept{return address;}
#include "core/CorpseVerification.h"
downwell::CorpseVerification compileCorpseVerification(){return downwell::verifyCorpses();}
