
#ifndef DEBUGCONFFILE
#define DEBUGCONFFILE

#ifdef DEBUG_BUILD
#define Debug(...) do{ printf(__VA_ARGS__); } while(0)
#else
#define Debug(...) do { } while(0)
#endif



#endif //DEBUGCONFFILE
