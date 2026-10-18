struct Library;
struct Interrupt;
Interrupt* AddICRVector(Library*,unsigned,Interrupt*);
void RemICRVector(Library*,unsigned,Interrupt*);
