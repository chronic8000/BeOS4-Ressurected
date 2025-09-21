
#ifndef AS_DEBUG_H
#define AS_DEBUG_H

#ifdef DEBUGGING_CONSOLE
#define AS_DEBUG 1
#else
#define AS_DEBUG 0
#endif

#include <IRegion.h>

#define dbMalloc 	malloc
#define dbFree 		free
#define dbRealloc 	realloc
#define dbStrdup 	strdup

#if AS_DEBUG


#include "BArray.h"

#include <stdarg.h>

#ifdef	__cplusplus
extern "C" {
#endif

#define DFLAG_WINDOW_VERBOSE	0x00000001
#define DFLAG_VIEW_VERBOSE		0x00000002
#define DFLAG_LOCK_REGIONS		0x00000004
#define DFLAG_LOCK_VIEWS		0x00000008
#define DFLAG_VIEW_TO_CONTEXT	0x00000010
#define DFLAG_BITMAP_TO_CANVAS	0x00000020
#define DFLAG_VIEW_TRAVERSE		0x00000040

#define tfInfo					0x00000001
#define tfWarning				0x00000002
#define tfError					0x00000004
#define tfFatalError			0x00000008
#define tfDebug					0x10000000

typedef struct {
	uint32 debugFlags;
} DebugInfo;

extern void DebugPushIndent(int32 indent);
extern void DebugPopIndent(int32 indent);
extern void DebugPrintfReal(const char *, ...);
extern void DebugPrintReal(char *);
extern void ShowRegionReal(region *r, char *s);
extern char *LookupMsg(int32 what);

#define DebugPrintf(a) DebugPrintfReal a
#define DebugPrint(a) DebugPrintReal a
#define ShowRegion(a) ShowRegionReal a

#define DebugOnly(a) a

/* Prototypes for debugging screen manipulation */
extern void ScrollDebugScreen(int32 delta);
extern void PageUpDebugScreen();
extern void PageDownDebugScreen();
extern void ToggleDebugScreen();
extern void DebugScreenInput(int32 key, uint32 mods, char *bytes);

#ifdef	__cplusplus
}

class BDataIO;
extern BDataIO& DOut;

#define CALLSTACK_DEPTH 9

class CallStack {
	public:

		uint32			m_caller[CALLSTACK_DEPTH];

						CallStack();
		unsigned long 	GetCallerAddress(int level);
		void 			SPrint(char *buffer);
		void 			Print();
		void 			LongPrint();
		void 			Update(int32 ignoreDepth=0);

		CallStack		&operator=(const CallStack &from);
		bool			operator==(CallStack) const;
};

class CallTreeNode {
	public:
	uint32					addr;
	int32					count;
	CallTreeNode *			higher;
	CallTreeNode *			lower;
	CallTreeNode *			parent;
	BArray<CallTreeNode*>	branches;
	
							~CallTreeNode();
	void					PruneNode();
	void					ShortReport();
	void					LongReport(char *buffer, int32 bufferSize);
};

class Benaphore;
class CallTree : public CallTreeNode {
	public:
	CallTreeNode *			highest;
	CallTreeNode *			lowest;
	Benaphore *				lock;

							CallTree(const char *name);
							~CallTree();
	void					Prune();
	void					AddToTree(CallStack *stack, int32 count=1);
	void					Report(int32 count, bool longReport=false);
};

class DebugNode {
	public:
		uint32			m_traceFlags;
		char *			m_name;

						DebugNode(char *name="some DebugNode");
		virtual			~DebugNode();
		
				char *	NodeName() { return m_name; }
				void	SetName(char *name);

		virtual	void	SetTrace(uint32 traceType, bool enabled);
				void	Trace(uint32 traceType, const char *format, va_list *args);

		virtual	void	DumpDebugInfo(DebugInfo *d);
		
};

extern void ResizeDebugScreen(int32 delta);

#define DebugCheckpoint(a) DebugPrintfReal("Checkpoint%s(%d):\"%s\"\n",__FILE__,__LINE__,#a);
#define DebugAssert(a) 															\
	if (!a) {																	\
		char s[512];															\
		sprintf(s,"%s(%d): assertion failed: \"%s\"",__FILE__,__LINE__,#a);		\
		DebugPrintfReal("%s\n",s);												\
		debugger(s);															\
	}
#define DEBUGTRACE(a) DebugTrace a
extern void DebugTrace(DebugNode *, const char *, ...);
extern void DebugTrace(DebugNode *, uint32 traceType, const char *, ...);
extern void DebugTrace(DebugNode *, uint32 traceType, const char *, va_list *args);

#endif

#else

#define DebugCheckpoint(a)
#define DebugAssert(a)
#define DEBUGTRACE(a)
#define DebugPrintf(a)
#define DebugPrint(a)
#define ShowRegion(a)
#define DebugOnly(a)

#ifdef	__cplusplus
extern "C" {
#endif

extern void DebugPushIndent(int32 indent);
extern void DebugPopIndent(int32 indent);
extern void DebugPrintfReal(const char *, ...);
extern void DebugPrintReal(char *);
extern void ShowRegionReal(region *r, char *s);
extern char *LookupMsg(int32 what);

#ifdef	__cplusplus
}
#endif

#endif

#endif
