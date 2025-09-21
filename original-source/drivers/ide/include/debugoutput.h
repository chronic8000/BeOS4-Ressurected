#ifndef DEBUG_OUTPUT_H
#define DEBUG_OUTPUT_H

extern int debug_level_error;
extern int debug_level_info;
extern int debug_level_flow;

#if 1
#define DE(l) if(l<=debug_level_error)
#define DI(l) if(l<=debug_level_info)
#define DF(l) if(l<=debug_level_flow)
#else
#define DE(l) if(l==0)
#define DI(l) if(l==0)
#define DF(l) if(0)
#endif

#endif /* DEBUG_OUTPUT_H */
