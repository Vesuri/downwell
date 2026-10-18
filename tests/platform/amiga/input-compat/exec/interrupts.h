#pragma once
struct Node {unsigned char ln_Type; signed char ln_Pri; char* ln_Name;};
struct Interrupt {Node is_Node;void* is_Data;void (*is_Code)();};
