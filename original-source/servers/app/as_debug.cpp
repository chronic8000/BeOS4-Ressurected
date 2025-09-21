
#include <Autolock.h>

#include "gr_types.h"
#include "proto.h"
#include "as_debug.h"
#include "heap.h"
#include "server.h"
#include "render.h"
#include "renderUtil.h"
#include "token.h"
#include "input.h"
#include "graphicDevice.h"
#include "bitmap.h"
#include "off.h"

#include "font_cache.h"

#include <Debug.h>

#define OUTPUT_TO_SERIAL		0
#define OUTPUT_TO_STDOUT		1
#define OUTPUT_TO_DEBUGPORT		1
#define OUTPUT_TO_CONSOLE		1

int32 debugIndent=0;

#if AS_DEBUG

#include "Unmangle.h"

RenderContext 	screenDebugContext[MAX_SCREENS];
RenderPort 		screenDebugPort[MAX_SCREENS];
RenderCanvas	screenDebugCanvas[MAX_SCREENS];

const char * lookup_symbol(uint32 addr, uint32 *offset);

class DebugDataIO : public BDataIO {
public:
	DebugDataIO()
		: fPos(0)
	{
	}
	
	virtual ~DebugDataIO()
	{
		flush_buffer();
	}
	
	virtual ssize_t Read(void */*buffer*/, size_t /*size*/)
	{
		return EPERM;
	}
	
	virtual ssize_t Write(const void *buffer, size_t size)
	{
		if (!buffer || size == 0) return 0;
		
		BAutolock l(fAccess);
		
		const size_t origSize = size;
		
		char* c = ((char*)buffer) + size - 1;
		while (c >= buffer && *c != '\n') c--;
		if (c >= buffer) {
			size_t len = (size_t)(c-(char*)buffer) + 1;
			append_buffer(buffer, len);
			size -= len;
			buffer = c + 1;
			flush_buffer();
		}
		
		if (size > 0) append_buffer(buffer, size);
		
		return origSize;
	}

private:
	void append_buffer(const void* data, size_t size)
	{
		while (size > 0) {
			if (size+fPos < sizeof(fBuffer)-1) {
				memcpy(fBuffer+fPos, data, size);
				fPos += size;
				size = 0;
			} else {
				size_t avail = sizeof(fBuffer)-fPos-1;
				memcpy(fBuffer+fPos, data, avail);
				fPos += avail;
				data = ((char*)data) + avail;
				size -= avail;
			}
			
			if (fPos >= sizeof(fBuffer)-1) flush_buffer();
		}
	}
	
	void flush_buffer()
	{
		if (fPos == 0) return;
		if (fPos >= sizeof(fBuffer)) fPos = sizeof(fBuffer)-1;
		fBuffer[fPos] = 0;
		DebugPrintf(("%s", fBuffer));
		fPos = 0;
	}
	
	BLocker fAccess;
	char fBuffer[1024];
	size_t fPos;
};

static DebugDataIO DebugOutIO;

BDataIO& DOut(DebugOutIO);

struct CallTreeRef {
	char *name;
	CallTree *tree;
};

BArray<CallTreeRef> callTreeRefs;

extern "C" char *	compiledWhen;
extern int32 screenFormatInc;
extern int32 screenRegionInc;
extern void SyncScreenInternal(	screen_desc *screen,
								RenderContext *context,
								RenderPort *port,
								RenderCanvas *canvas);

#define INITIAL_DEBUG_SCREEN_HEIGHT		180
#define DEBUG_SCROLLBACK				8192
#define DEBUG_TEXT_HEIGHT 				13
#define	BOTTOM_MARGIN					4
#define TOTAL_HEIGHT					((DEBUG_SCROLLBACK+1)*DEBUG_TEXT_HEIGHT)+BOTTOM_MARGIN

port_id debugInputPort=B_BAD_PORT_ID;
port_id debugOutputPort=B_BAD_PORT_ID;
sem_id debugOutputSem=B_BAD_SEM_ID;
char *debugOutputPending = NULL;
Benaphore debugOutputPendingLock("debugOutputPendingLock");
int32 debugScreenHeight=0;
int32 debugScrollingPos=TOTAL_HEIGHT;
int32 debugScreenHidden=true;
bool okayToScreen=false;
bool fontSet = false;
CountedBenaphore debugScreenLock("debugScreenLock");
char *debugScrollback[DEBUG_SCROLLBACK];
int32 debugBufPtr=0;
char debugCmd[256];
char *debugFont = "ProFontISOLatin1";
float debugFontSize = 9.0;
DebugInfo debugSettings = {
	DFLAG_WINDOW_VERBOSE	|
	DFLAG_VIEW_VERBOSE		|
//	DFLAG_LOCK_REGIONS		|
//	DFLAG_LOCK_VIEWS		|
	DFLAG_VIEW_TO_CONTEXT	|
	DFLAG_BITMAP_TO_CANVAS
};
char *okayInput = 	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
					"abcdefghijklmnopqrstuvwxyz"
					"1234567890.*? -/";

void AssertFont();
void DebugInputThread();
void DebugOutputThread();
void WriteToOutputPort(char *msg);
void DebugSendCommand(char *cmd);
void DebugScreenOutput(char *_string);
void DebugScreenOutputOneLine(char *string, bool advance);
void StartDebugScreenManip(point *p);
void EndDebugScreenManip(point p);
int32 AddDebuggerCommand(
	char *cmd, void *_func,
	void *userData=NULL, char *help=NULL,
	char *longHelp=NULL);


struct AppMsg {
	int32 what;
	char name[30];
};

#define MSGCOUNT (sizeof(AppMsgLookupTable) / sizeof(AppMsgLookupTable[0]))
#define ENTRY(a) { a , #a }
AppMsg AppMsgLookupTable[] =
{
	ENTRY(GR_CREATE_WINDOW),
	ENTRY(GR_NEW_BITMAP),
	ENTRY(GR_SET_BITS),
	ENTRY(GR_DELETE_BITMAP),
	ENTRY(GR_NEW_APP),
	ENTRY(GR_CREATE_OFF_WINDOW),
	ENTRY(GR_SET_RECT_TRACK),
	ENTRY(GR_FIND_WINDOW),
	ENTRY(GR_SET_BITS_24),
	ENTRY(GR_SET_BITS_24_TST),
	ENTRY(GR_SET_DRAG),
	ENTRY(GR_GET_SCS),
	ENTRY(GR_SET_DRAG_BM),
	ENTRY(GR_SHOW_CURSOR),
	ENTRY(GR_HIDE_CURSOR),
	ENTRY(GR_OBSCURE_CURSOR),
	ENTRY(GR_ZOOM),
	ENTRY(GR_IMPORT_PICTURE),
	ENTRY(GR_CLONE_PICTURE),
	ENTRY(GR_DELETE_PICTURE),
	ENTRY(GR_GET_PICTURE_SIZE),
	ENTRY(GR_GET_PICTURE_DATA),
	ENTRY(GR_SET_TEAM_ID),
	ENTRY(GR_SET_FONT_SET),
	ENTRY(GR_GET_FONT_SET),
	ENTRY(GR_COUNT_SETS),
	ENTRY(GR_IS_CURSOR_HIDDEN),
	ENTRY(GR_SET_CURSOR2),
	ENTRY(GR_TRUNCATE_STRINGS),
	ENTRY(GR_LOCK_SCREEN),
	ENTRY(GR_UNLOCK_SCREEN),
	ENTRY(GR_GET_DESKTOP_COLOR),
	ENTRY(GR_SET_DESKTOP_COLOR),
	ENTRY(GR_ACTIVATE_TEAM),
	ENTRY(GR_COUNT_WS),
	ENTRY(GR_SET_WS_COUNT),
	ENTRY(GR_CURRENT_WS),
	ENTRY(GR_SELECT_WS),
	ENTRY(GR_SET_SCREEN_RES),
	ENTRY(GR_SET_SCREEN_RATE),
	ENTRY(GR_SET_SCREEN_POS),
	ENTRY(GR_PROPOSE_DISPLAY_MODE),
	ENTRY(GR_GET_DISPLAY_MODE_LIST),
	ENTRY(GR_SET_DISPLAY_MODE),
	ENTRY(GR_GET_DISPLAY_MODE),
	ENTRY(GR_GET_FRAME_BUFFER_CONFIG),
	ENTRY(GR_GET_ACCELERANT_DEVICE_INFO),
	ENTRY(GR_GET_PIXEL_CLOCK_LIMITS),
	ENTRY(GR_GET_TIMING_CONSTRAINTS),
	ENTRY(GR_GET_DPMS_CAPABILITIES),
	ENTRY(GR_GET_DPMS_STATE),
	ENTRY(GR_SET_DPMS_STATE),
	ENTRY(GR_GET_ACCELERANT_IMAGE_PATH),
	ENTRY(GR_GET_ACCELERANT_CLONE_INFO),
	ENTRY(GR_ACCELERANT_PERFORM_HACK),
	ENTRY(GR_GET_DISPLAY_DEVICE_PATH),
	ENTRY(GR_GET_SCROLL_BAR_INFO),
	ENTRY(GR_SET_SCROLL_BAR_INFO),
	ENTRY(GR_CTRL_GRAPHICS_CARD),
	ENTRY(GR_GET_SCREEN_BITMAP),
	ENTRY(GR_LOCK_DIRECT_SCREEN),
	ENTRY(GR_GET_CLONE_INFO),
	ENTRY(GR_SET_STANDARD_SPACE),	
	ENTRY(GR_GET_DRIVER_INFO),
	ENTRY(GR_GET_SYNC_INFO),
	ENTRY(GR_OPEN_INPUT_CHANNEL),
	ENTRY(GR_CLOSE_INPUT_CHANNEL),
	ENTRY(GR_SET_INPUT_CHANNEL),
	ENTRY(GR_GET_INPUT_CHANNEL),
	ENTRY(GR_SET_DRAG_PIXEL),
	ENTRY(GR_GET_FAMILY_LIST),
	ENTRY(GR_GET_STYLE_LIST),
	ENTRY(GR_GET_TUNED_FONT_LIST),
	ENTRY(GR_FONT_CHANGE_TIME),
	ENTRY(GR_STRING_WIDTH),
	ENTRY(GR_GET_ESCAPEMENTS),
	ENTRY(GR_GET_EDGES),
	ENTRY(GR_GET_HEIGHT),
	ENTRY(GR_GET_FONT_IDS),
	ENTRY(GR_GET_FONT_NAMES),
	ENTRY(GR_SET_CURSOR),
	ENTRY(GR_GET_STANDARD_FONTS),
	ENTRY(GR_SET_STANDARD_FONTS),
	ENTRY(GR_GET_FONT_CACHE_SETTINGS),
	ENTRY(GR_SET_FONT_CACHE_SETTINGS),
	ENTRY(GR_REMOVE_FONT_DIR),
	ENTRY(GR_CHANGE_FONT_DIR_STATUS),
	ENTRY(GR_CHANGE_FONT_FILE_STATUS),
	ENTRY(GR_GET_FONT_DIR_LIST),
	ENTRY(GR_GET_FONT_FILE_LIST),
	ENTRY(GR_SAVE_FONT_CACHE),
	ENTRY(GR_FONT_SET_CONTROL),
	ENTRY(GR_SAVE_TUNED_FONT_SET),
	ENTRY(GR_COUNT_WINDOWS),
	ENTRY(GR_GET_WINDOW_INFO),
	ENTRY(GR_DO_WINDOW_ACTION),
	ENTRY(GR_GET_TOKEN_LIST),
	ENTRY(GR_DO_MINI_TEAM),
	ENTRY(GR_DO_ACTIVATE_TEAM),
	ENTRY(GR_GET_UI_SETTINGS),
	ENTRY(GR_SET_UI_SETTINGS),
	ENTRY(GR_SET_MOUSE_POSITION),
	ENTRY(GR_SELECT_WS_ASYNC),
	ENTRY(GR_SET_COLOR_MAP),
	ENTRY(GR_GET_IDLE_TIME),
	ENTRY(GR_GET_SCREEN_INFO),
	ENTRY(GR_SET_FOCUS_FOLLOWS_MOUSE),
	ENTRY(GR_GET_FOCUS_FOLLOWS_MOUSE),
	ENTRY(GR_DIRECT_WINDOW_INFO),
	ENTRY(GR_GET_SERVER_HEAP_AREA),
	ENTRY(GR_GET_FONT_GLYPHS),
	ENTRY(GR_GET_FONT_FLAGS_INFO),
	ENTRY(GR_GET_FONT_BBOX),
	ENTRY(GR_GET_FONT_BLOCKS),
	ENTRY(GR_GET_GLYPHS_BBOX),
	ENTRY(GR_GET_STRINGS_BBOX),
	ENTRY(GR_GET_HAS_GLYPHS),
	ENTRY(GR_SET_MINI_ICON),
	ENTRY(GR_GET_UNBLOCKED_SCREEN),
	ENTRY(GR_SET_WINDOW_DECOR),
	ENTRY(GR_GET_OVERLAY_RESTRICTIONS),
	ENTRY(GR_NEW_CURSOR),
	ENTRY(GR_DELETE_CURSOR),
	
	ENTRY(GR_ADD_VIEW),
	ENTRY(GR_MOVE_VIEW),
	ENTRY(GR_MOVETO_VIEW),
	ENTRY(GR_SIZE_VIEW),
	ENTRY(GR_SIZETO_VIEW),
	ENTRY(GR_REMOVE_VIEW),
	ENTRY(GR_FIND_VIEW),
	ENTRY(GR_GET_VIEW_BOUND),
	ENTRY(GR_VIEW_FLAGS),
	ENTRY(GR_MOVE_WINDOW),
	ENTRY(GR_MOVETO_WINDOW),
	ENTRY(GR_SIZE_WINDOW),
	ENTRY(GR_SIZETO_WINDOW),
	ENTRY(GR_SELECT_WINDOW),
	ENTRY(GR_CLOSE_WINDOW),
	ENTRY(GR_WGET_TITLE),
	ENTRY(GR_WSET_TITLE),
	ENTRY(GR_WGET_BOX),
	ENTRY(GR_WIS_FRONT),
	ENTRY(GR_SEND_TO_BACK),
	ENTRY(GR_WGET_BOUND),
	ENTRY(GR_HIDE),
	ENTRY(GR_SHOW),
	ENTRY(GR_WINDOW_LIMITS),
	ENTRY(GR_IS_ACTIVE),
	ENTRY(GR_MINIMIZE),
	ENTRY(GR_MAXIMIZE),
	ENTRY(GR_SHOW_SYNC),
	ENTRY(GR_SET_WINDOW_FLAGS),
	ENTRY(GR_SEND_BEHIND),
	ENTRY(GR_ADD_SUBWINDOW),
	ENTRY(GR_REMOVE_SUBWINDOW),
	ENTRY(GR_SET_WINDOW_ALIGNMENT),
	ENTRY(GR_GET_WINDOW_ALIGNMENT),
	ENTRY(GR_GET_BASE_POINTER),
	ENTRY(GR_PICK_VIEW),	
	ENTRY(GR_MOVETO),
	ENTRY(GR_MOVEBY),
	ENTRY(GR_LINE),
	ENTRY(GR_LINETO),
	ENTRY(GR_RECTFRAME),
	ENTRY(GR_RECTFILL),
	ENTRY(GR_ARC_INSCRIBE_STROKE),
	ENTRY(GR_ARC_INSCRIBE_FILL),
	ENTRY(GR_ARC_STROKE),
	ENTRY(GR_ARC_FILL),
	ENTRY(GR_ROUND_RECT_FRAME),
	ENTRY(GR_ROUND_RECT_FILL),
	ENTRY(GR_FRAME_REGION),
	ENTRY(GR_FILL_REGION),
	ENTRY(GR_POLYFRAME),
	ENTRY(GR_POLYFILL),
	ENTRY(GR_DRAW_BEZIER),
	ENTRY(GR_FILL_BEZIER),
	ENTRY(GR_ELLIPSE_INSCRIBE_STROKE),
	ENTRY(GR_ELLIPSE_INSCRIBE_FILL),
	ENTRY(GR_ELLIPSE_STROKE),
	ENTRY(GR_ELLIPSE_FILL),
	ENTRY(GR_DRAW_BITMAP),
	ENTRY(GR_SCALE_BITMAP),
	ENTRY(GR_SCALE_BITMAP1),
	ENTRY(GR_DRAW_BITMAP_A),
	ENTRY(GR_SCALE_BITMAP_A),
	ENTRY(GR_SCALE_BITMAP1_A),
	ENTRY(GR_DRAW_STRING),
	ENTRY(GR_MOVE_BITS),
	ENTRY(GR_RECT_INVERT),
	ENTRY(GR_START_VARRAY),
	ENTRY(GR_PLAY_PICTURE),
	ENTRY(GR_STROKE_SHAPE),
	ENTRY(GR_FILL_SHAPE),
	ENTRY(GR_SET_DRAW_MODE),
	ENTRY(GR_VIEW_SET_ORIGIN),
	ENTRY(GR_SET_PEN_SIZE),
	ENTRY(GR_SET_VIEW_COLOR),
	ENTRY(GR_SET_LINE_MODE),
	ENTRY(GR_SET_FONT_CONTEXT),
	ENTRY(GR_FORE_COLOR),
	ENTRY(GR_BACK_COLOR),
	ENTRY(GR_NO_CLIP),
	ENTRY(GR_SET_CLIP),
	ENTRY(GR_SET_SCALE),
	ENTRY(GR_SET_ORIGIN),
	ENTRY(GR_PUSH_STATE),
	ENTRY(GR_POP_STATE),
	ENTRY(GR_SET_PATTERN),
	ENTRY(GR_SET_VIEW_BITMAP),
	ENTRY(GR_SET_FONT_FLAGS),
	ENTRY(GR_GET_PEN_SIZE),
	ENTRY(GR_GET_FORE_COLOR),
	ENTRY(GR_GET_BACK_COLOR),
	ENTRY(GR_GET_PEN_LOC),
	ENTRY(GR_GET_LOCATION),
	ENTRY(GR_GET_DRAW_MODE),
	ENTRY(GR_GET_CLIP),
	ENTRY(GR_GET_CAP_MODE),
	ENTRY(GR_GET_JOIN_MODE),
	ENTRY(GR_GET_MITER_LIMIT),
	ENTRY(GR_GET_ORIGIN),
	ENTRY(GR_ADD_SCROLLBAR),
	ENTRY(GR_SCROLLBAR_SET_VALUE),
	ENTRY(GR_SCROLLBAR_SET_RANGE),
	ENTRY(GR_SCROLLBAR_SET_STEPS),
	ENTRY(GR_SCROLLBAR_SET_PROPORTION),
	ENTRY(GR_PT_TO_SCREEN),
	ENTRY(GR_PT_FROM_SCREEN),
	ENTRY(GR_RECT_TO_SCREEN),
	ENTRY(GR_RECT_FROM_SCREEN),
	ENTRY(GR_GET_VIEW_MOUSE),
	ENTRY(GR_INVAL),
	ENTRY(GR_UPDATE_OFF),
	ENTRY(GR_UPDATE_ON),
	ENTRY(GR_NEED_UPDATE),
	ENTRY(GR_GET_UPDATE_STATE),
	ENTRY(GR_SYNC),
	ENTRY(GR_START_PICTURE),
	ENTRY(GR_RESTART_PICTURE),
	ENTRY(GR_END_PICTURE),
	ENTRY(GR_WORKSPACE),
	ENTRY(GR_SET_WS),
	ENTRY(GR_DATA),
	ENTRY(GET_DROP),
	ENTRY(GR_INIT_DIRECTWINDOW),
	ENTRY(GR_SET_FULLSCREEN),	
	ENTRY(GR_EXPERIMENTAL_MESSAGES),
	ENTRY(GR_START_DISK_PICTURE),
	ENTRY(GR_LOOPER_THREAD),
	ENTRY(GR_PLAY_DISK_PICTURE),	
	ENTRY(E_START_DRAW_WIND),
	ENTRY(E_END_DRAW_WIND),
	ENTRY(E_DRAW_VIEW),
	ENTRY(E_END_DRAW_VIEW),
	ENTRY(E_DRAW_VIEWS),
	ENTRY(E_START_DRAW_VIEW)
};

char *LookupMsg(int32 what)
{
	for (int32 i=0;i<((int32)MSGCOUNT);i++) {
		if (AppMsgLookupTable[i].what == what)
			return AppMsgLookupTable[i].name;
	};
	return "Unknown message!";
};

CallStack::CallStack()
{
	for (int32 i=0;i<CALLSTACK_DEPTH;i++) m_caller[i] = 0;
};

#if __POWERPC__
static __asm unsigned long * get_caller_frame();

static __asm unsigned long *
get_caller_frame ()
{
	lwz     r3, 0 (r1)
	blr
}

#endif

#define bogus_addr(x)  ((x) < 0x80000000)

unsigned long CallStack::GetCallerAddress(int level)
{
#if __INTEL__
	uint32 fp = 0, nfp, ret=0;

	level += 2;
	
	fp = (uint32)get_stack_frame();
	if (bogus_addr(fp))
		return 0;
	nfp = *(ulong *)fp;
	while (nfp && --level > 0) {
		if (bogus_addr(fp))
			return 0;
		nfp = *(ulong *)fp;
		ret = *(ulong *)(fp + 4);
		if (bogus_addr(ret))
			break;
		fp = nfp;
	}

	return ret;
#else
	unsigned long *cf = get_caller_frame();
	unsigned long ret = 0;
	
	level += 1;
	
	while (cf && --level > 0) {
		ret = cf[2];
		if (ret < 0x80000000) break;
		cf = (unsigned long *)*cf;
	}

	return ret;
#endif
}

void CallStack::SPrint(char *buffer)
{
	char tmp[32];
	buffer[0] = 0;
	for (int32 i = 0; i < CALLSTACK_DEPTH; i++) {
		sprintf(tmp, " 0x%lx", m_caller[i]);
		strcat(buffer, tmp);
	}
}

void CallStack::Print()
{
	char tmp[32];
	for (int32 i = 0; i < CALLSTACK_DEPTH; i++) {
		DebugPrintf((" 0x%x", m_caller[i]));
		if (!m_caller[i]) break;
	}
}

void CallStack::LongPrint()
{
	char tmp[256];
	uint32 offs;
	for (int32 i = 0; i < CALLSTACK_DEPTH; i++) {
#if __INTEL__
		if (!demangle(lookup_symbol(m_caller[i],&offs),tmp,256))
#endif
			strncpy(tmp,lookup_symbol(m_caller[i],&offs),256);
		DebugPrintf((" 0x%08x: <%s>+0x%08x\n", m_caller[i], tmp, offs));
		if (!m_caller[i]) break;
	}
}

void CallStack::Update(int32 ignoreDepth)
{
	for (int32 i = 1; i <= CALLSTACK_DEPTH; i++) {
		m_caller[i - 1] = GetCallerAddress(i+ignoreDepth);
	};
};

CallStack &CallStack::operator=(const CallStack& from)
{
	for (int32 i=0;i<CALLSTACK_DEPTH;i++) m_caller[i] = from.m_caller[i];
	return *this;
}

bool CallStack::operator==(CallStack s) const
{
	for (int32 i=0;i<CALLSTACK_DEPTH;i++) if (m_caller[i] != s.m_caller[i]) return false;
	return true;
}

CallTreeNode::~CallTreeNode()
{
	PruneNode();
};

void CallTreeNode::PruneNode()
{
	for (int32 i=0;i<branches.CountItems();i++)
		delete branches[i];
	branches.SetItems(0);
};

void CallTreeNode::ShortReport()
{
	if (!parent) return;
	parent->ShortReport();
	if (parent->parent)
		DebugPrintf((", %08x",addr));
	else
		DebugPrintf(("%08x",addr));
};

void CallTreeNode::LongReport(char *buffer, int32 bufferSize)
{
	uint32 offs;
	if (!parent) return;
	parent->LongReport(buffer,bufferSize);
#if __INTEL__
	if (!demangle(lookup_symbol(addr,&offs),buffer,bufferSize))
#endif
		strncpy(buffer,lookup_symbol(addr,&offs),bufferSize);
	DebugPrintf(("  0x%08x: <%s>+0x%08x\n", addr, buffer, offs));
};

CallTree::CallTree(const char *name)
{
	addr = 0;
	count = 0;
	higher = lower = highest = lowest = parent = NULL;
	CallTreeRef ref;
	ref.name = dbStrdup(name);
	ref.tree = this;
	callTreeRefs.AddItem(ref);
	lock = new Benaphore(const_cast<char*>(name));
};

CallTree::~CallTree()
{
	for (int32 i=0;i<callTreeRefs.CountItems();i++) {
		if (callTreeRefs[i].tree == this) {
			dbFree(callTreeRefs[i].name);
			callTreeRefs.RemoveItem(i);
			break;
		};
	};
	delete lock;
};

void CallTree::Prune()
{
	PruneNode();
	highest = lowest = higher = lower = NULL;
	count = 0;
};

void CallTree::Report(int32 rcount, bool longReport)
{
	lock->Lock();

	char buffer[1024];
	CallTreeNode *n = highest;
	DebugPrintf(("%d total hits, reporting top %d\n",count,rcount));
	DebugPrintf(("-------------------------------------------------\n"));
	while (n && rcount--) {
		if (longReport) {
			DebugPrintf(("%d hits-------------------------------\n",n->count));
			n->LongReport(buffer,1024);
		} else {
			DebugPrintf(("%d hits --> ",n->count));
			n->ShortReport();
			DebugPrintf(("\n"));
		};
		n = n->lower;
	};

	lock->Unlock();
};

bool is_top(uint32 addr);

void CallTree::AddToTree(CallStack *stack, int32 hitCount)
{
	CallTreeNode *n,*next,*replace;
	int32 i,index = 0;
	uint32 offs;

	if (!hitCount) return;
	if (!stack->m_caller[0]) return;

	lock->Lock();

	n = this;
	while (stack->m_caller[index] && !is_top(stack->m_caller[index])) {
		for (i=0;i<n->branches.CountItems();i++) {
			next = n->branches[i];
			if (next->addr == stack->m_caller[index]) goto gotIt;
		};
		next = new CallTreeNode;
		next->addr = stack->m_caller[index];
		next->higher = NULL;
		next->lower = NULL;
		next->count = 0;
		next->parent = n;
		n->branches.AddItem(next);
		gotIt:
		n = next;
		index++;
		if (index == CALLSTACK_DEPTH) break;
	};
	if (n->count == 0) {
		n->higher = lowest;
		if (lowest) lowest->lower = n;
		else highest = n;
		lowest = n;
	};
	count+=hitCount;
	n->count+=hitCount;
	while (n->higher && (n->count > n->higher->count)) {
		replace = n->higher;
		replace->lower = n->lower;
		if (replace->lower == NULL) lowest = replace;
		else replace->lower->higher = replace;
		n->lower = replace;
		n->higher = replace->higher;
		if (n->higher == NULL) highest = n;
		else n->higher->lower = n;
		replace->higher = n;
	};

	lock->Unlock();
};

DebugNode::DebugNode(char *name)
{
	m_traceFlags = tfFatalError;
	m_name = name;
};

DebugNode::~DebugNode()
{
};

void DebugNode::SetName(char *name)
{
	m_name = name;
};

void DebugNode::SetTrace(uint32 traceType, bool enabled)
{
	char *name = NodeName();

	if (enabled) {
		uint32 hum = m_traceFlags | traceType;
		uint32 x = hum ^ m_traceFlags;
		if (x & tfInfo) DebugPrintf(("%s(0x%08x): tfInfo tracing turned ON\n",name,this));
		if (x & tfWarning) DebugPrintf(("%s(0x%08x): tfWarning tracing turned ON\n",name,this));
		if (x & tfError) DebugPrintf(("%s(0x%08x): tfError tracing turned ON\n",name,this));
		if (x & tfFatalError) DebugPrintf(("%s(0x%08x): tfFatalError tracing turned ON\n",name,this));
		m_traceFlags = hum;
	} else {
		uint32 hum = m_traceFlags & ~traceType;
		uint32 x = hum ^ m_traceFlags;
		if (x & tfInfo) DebugPrintf(("%s(0x%08x): tfInfo tracing turned OFF\n",name,this));
		if (x & tfWarning) DebugPrintf(("%s(0x%08x): tfWarning tracing turned OFF\n",name,this));
		if (x & tfError) DebugPrintf(("%s(0x%08x): tfError tracing turned OFF\n",name,this));
		if (x & tfFatalError) DebugPrintf(("%s(0x%08x): tfFatalError tracing turned OFF\n",name,this));
		m_traceFlags = hum;
	};
};

char *TypeToName(uint32 traceType)
{
	char *traceName = "Unknown";
	switch (traceType) {
		case tfInfo: traceName = "tfInfo"; break;
		case tfWarning: traceName = "tfWarning"; break;
		case tfError: traceName = "tfError"; break;
		case tfFatalError: traceName = "tfFatalError"; break;
	};
	return traceName;
};

void DebugTrace(DebugNode *node, uint32 traceType, const char *format, va_list *args)
{
	if (!node) return;
	if (node->m_traceFlags & traceType) {
		char s[8192];
		sprintf(s,"%s(%s:%08lx): ",TypeToName(traceType),node->NodeName(),(uint32)node);
		vsprintf(s+strlen(s),format,*args);
		DebugPrint((s));
	};
};

void DebugNode::Trace(uint32 traceType, const char *format, va_list *args)
{
	char		s[8192];
	char *		traceName = TypeToName(traceType);

	sprintf(s,"%s(%s:%08lx): ",NodeName(),traceName,(uint32)this);
	vsprintf(s+strlen(s), format, *args);
	DebugPrint((s));
};

void DebugTrace(DebugNode *node, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	DebugTrace(node,tfInfo,format,&args);
	va_end(args);
};

void DebugTrace(DebugNode *node, uint32 traceType, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	DebugTrace(node,traceType,format,&args);
	va_end(args);
};

void DebugNode::DumpDebugInfo(DebugInfo *)
{
	DebugPrintf(("DebugNode(0x%08x): Override DumpDebugInfo()!\n"));
};

typedef void (*DebuggerFunc)(int32 argc, char **argv, void *data);
struct DebugCmd {
	char			cmd[20];
	char *			help;
	char *			longHelp;
	DebuggerFunc	func;
	void *			data;
};

DebugCmd commands[64];
int32 commandCount=0;

bool DebugProcessCommand(char *_cmd)
{
	char __cmd[1024];
	strcpy(__cmd,_cmd);
	char *cmd = __cmd;
	
	while (*cmd == ' ') cmd++;
	char *args = cmd;
	while ((*args != ' ') && (*args != 0)) args++;
	while (*args == ' ') *args++ = 0;
	for (int32 i=0;i<commandCount;i++) {
		DebugCmd *c = commands+i;
		if (strcasecmp(cmd,c->cmd)==0) {
			if (*args == 0) c->func(0,NULL,c->data);
			else {
				char *argv[32];
				int32 argc=0;
				while (*args != 0) {
					bool quoted = false;
					if (*args == '\"') {
						quoted = true;
						args++;
					};
					argv[argc++] = args;
					while ((*args != ' ') && (*args != 0) && (!quoted || (*args != '\"'))) args++;
					while ((*args == ' ') || (quoted && (*args == '\"'))) *args++ = 0;
				};
				c->func(argc,argv,c->data);
			};
			return true;
		};
	};
	
	return false;
};

void ShowDebuggerCommands()
{
	for (int32 i=0;i<commandCount;i++) {
		DebugCmd *c = commands+i;
		int32 len = strlen(c->cmd);
		len = 15-len;
		if (len < 0) len = 0;
		DebugPushIndent(len);
		if (c->help)	DebugPrintf(("\"%s\"     %s\n",c->cmd,c->help));
		else			DebugPrintf(("\"%s\"     <No description>\n",c->cmd));
		DebugPopIndent(len);
	};
};

void ShowKeys()
{
	DebugPrint((
		"         SHIFT-UP     Scroll up\n"
		"       SHIFT-DOWN     Scroll up\n"
		"     SHIFT-PAGEUP     Scroll up one page\n"
		"   SHIFT-PAGEDOWN     Scroll down one page\n"
		"       CONTROL-UP     Expand debugging console\n"
		"     CONTROL-DOWN     Shrink debugging console\n"
		"           INSERT     Hide/show debugging console\n"
	));
};

int32 AddDebuggerCommand(char *cmd, void *_func, void *userData, char *help, char *longHelp)
{
	strncpy(commands[commandCount].cmd,cmd,19);
	commands[commandCount].func = (DebuggerFunc)_func;
	commands[commandCount].data = userData;
	if (help) commands[commandCount].help = dbStrdup(help);
	else commands[commandCount].help = NULL;
	if (longHelp) commands[commandCount].longHelp = dbStrdup(longHelp);
	else commands[commandCount].longHelp = NULL;
	return commandCount++;
};

bool HideDebugScreen();
bool ShowDebugScreen();

/********************************************************/

void RegionSem(int32 argc, char **argv)
{
	GlobalRegionsLock.DumpInfo();
};

struct IterInfo {
	int32 token;
	void *ptr;
	bool succeeded;
};

struct DebugNodeType {
	int16 tokenType;
	char *name;
};

DebugNodeType dots[] = {
	{ TT_WINDOW|TT_ATOM, "window" },
	{ TT_ATOM, "atom" },
	{ TT_VIEW, "view" }
};

uint32 DebugNodeIterator(int32 token, int32 /*owner*/, int16 /*type*/, void *data, IterInfo *info)
{
	if (((info->token == -1) && (info->ptr == NULL)) ||
		(info->token == token) || (info->ptr == data)) {
		SAtom *o = (SAtom*)data;
		o->DumpDebugInfo(&debugSettings);
		info->succeeded = true;
	};
	return 0;
};

void DebugNodeInfo(int32 argc, char **argv, DebugNodeType *type)
{
	IterInfo info;

	info.succeeded = false;
	info.token = -1;
	info.ptr = NULL;
	
	void* orig_ptr = NULL;
	
	bool acquired = false;
	
	if (argc) {
		if ((argv[0][0] == '0') && (argv[0][1] == 'x'))
			info.ptr = orig_ptr = (void*)(strtoul(argv[0]+2,NULL,16));
		else
			info.token = atoi(argv[0]);
	};
	
	tokens->iterate_tokens(ANY_USER,type->tokenType,(token_iterator)DebugNodeIterator,&info);

	if (!info.succeeded && orig_ptr && BAtom::ExistsAndAcquire((BAtom*)orig_ptr)) {
		DebugNodeIterator(-1, 0, 0, orig_ptr, &info);
		((BAtom*)orig_ptr)->Release();
		info.succeeded = true;
	}
	
	if (!info.succeeded) {
		if (info.ptr) {
			DebugPrintf(("No %s at address 0x%08x is in the token registry\n",type->name,info.ptr));
		} else if (info.token != -1) {
			DebugPrintf(("Token %d either isn't a %s or doesn't exist\n",info.token,type->name));
		} else {
			DebugPrintf(("No %ss in the token registry\n",type->name));
		};
	};
};

uint32 TokenIterator(int32 token, int32 owner, int16 type, void *data, void *)
{
	char *typeName;
	if (type & TT_WINDOW) {
		if (type & TT_OFFSCREEN)		typeName = " offscreen";
		else							typeName = "    window";
	} else if (type & TT_PICTURE)		typeName = "   picture";
	else if (type & TT_BITMAP)			typeName = "    bitmap";
	else if (type & TT_VIEW)			typeName = "      view";
	else if (type & TT_FONT_CACHE)		typeName = "font cache";
	else if (type & TT_ATOM)			typeName = "      atom";
	else								typeName = "   unknown";

	DebugPrintf(("    %5d %8d %s   0x%08x",token,owner,typeName,data));

	if (type & TT_WINDOW) {
		TWindow *w = (TWindow*)data;
		if (type & TT_OFFSCREEN) {
			SBitmap *b = ((TOff*)w)->Bitmap();
			DebugPrintf(("    (%d x %d)", b->Canvas()->pixels.w, b->Canvas()->pixels.h));
		} else DebugPrintf(("    \"%s\"",w->get_name()));
	} else if (type & TT_BITMAP) {
		SBitmap *b = (SBitmap*)data;
		DebugPrintf(("    (%d x %d)", b->Canvas()->pixels.w, b->Canvas()->pixels.h));
	};

	if (type & TT_ATOM) {
		SAtom *atom = (SAtom*)data;
		DebugPrintf(("    (refs = %d,%d)", atom->ServerRefs(),atom->ClientRefs()));
	};

	DebugPrint(("\n"));
	return 0;
};

int32 debugVar[10];
void SetVar(int32 argc, char **argv)
{
	if (argc < 1) {
		DebugPrintf(("See usage\n"));
		return;
	};
	
	int32 var = atoi(argv[0]);
	int32 value=1;
	if (argc == 2) value = atoi(argv[1]);
	debugVar[var] = value;
};

void CallTreeDisplay(int32 argc, char **argv)
{
	int32 i,rcount=20,longFormat=0;
	if (argc < 1) {
		for (i=0;i<callTreeRefs.CountItems();i++) {
			DebugPrintf(("  \"%s\" : %d hits\n",callTreeRefs[i].name,callTreeRefs[i].tree->count));
		};
		return;
	};

	for (i=0;i<callTreeRefs.CountItems();i++) {
		if (!strcmp(callTreeRefs[i].name,argv[0])) break;
	};
	if (i == callTreeRefs.CountItems()) return;

	if (argc > 1) rcount = atoi(argv[1]);
	if (argc > 2) longFormat = 1;

	DebugPrintf(("CallTreeDisplay %s %08x %d %d\n",callTreeRefs[i].name,
		callTreeRefs[i].tree,rcount,longFormat));
	
	callTreeRefs[i].tree->Report(rcount,longFormat);
};

#if INTERNAL_LEAKCHECKING
extern int32 LeakyLookup(void *ptr, char *comment, CallStack *stack);

void LookupMalloc(int32 argc, char **argv)
{
	if (argc < 1) {
		DebugPrintf(("See usage\n"));
		return;
	};
	
	uint32 addr;
	sscanf(argv[0],"%lx",&addr);
	
	int32 size;
	char comment[256];
	CallStack stack;
	
	size = LeakyLookup((void*)addr,comment,&stack);
	if (size == -1) {
		DebugPrintf(("No allocation at %08x\n",addr));
	} else {
		char callstack[1024];
		stack.SPrint(callstack);
		DebugPrintf(("%08x: \"%s\", size=%ld, callstack= %s\n",
			addr,comment,size,callstack));
	};
};
#endif

void LookupSym(int32 argc, char **argv)
{
	if (argc < 1) {
		DebugPrintf(("See usage\n"));
		return;
	};
	
	uint32 offset,addr;
	sscanf(argv[0],"%lx",&addr);
	const char *name = lookup_symbol(addr,&offset);
	DebugPrintf(("0x%08x: <%s>+0x%08x\n",addr,name,offset));
};

static int32 cur_mark = 0;

void MarkAtoms(int32 argc, char **argv)
{
	//if (argc>0) tokenOwner = atoi(argv[0]);
	//if (argc>1) tokenType = atoi(argv[1]);

	cur_mark = BAtom::MarkLeakReport();
	printf("Starting atom context #%ld\n", cur_mark);
};

void DumpAtoms(int32 argc, char **argv)
{
	if (argc>0) {
		if (strcasecmp("current", argv[0]) == 0) {
			DebugPrintf(("Atoms in current context that still exist:\n"));
			BAtom::LeakReport(DOut, cur_mark);
			return;
		}
		if (strcasecmp("last", argv[0]) == 0) {
			DebugPrintf(("Atoms in last context that still exist:\n"));
			BAtom::LeakReport(DOut, cur_mark-1, cur_mark-1);
			return;
		}
	}

	DebugPrintf(("All active atoms:\n"));
	BAtom::LeakReport(DOut);
};

void DumpTokens(int32 argc, char **argv)
{
	int32 tokenOwner = ANY_USER;
	if (argc>0) tokenOwner = atoi(argv[0]);
	int16 tokenType = TT_ANY;
	if (argc>1) tokenType = atoi(argv[1]);

	DebugPrintf((	"\nTokenspace for owner=%d, type=%d:\n\n"
					"    Token    Owner       Type         Data\n"
					"    ----- -------- ----------   ----------\n",
					tokenOwner,tokenType));
	tokens->iterate_tokens(tokenOwner,tokenType,TokenIterator);
};

#ifdef INTERNAL_LEAKCHECKING
extern void grLeakyReport(int32 count, char *tag, int32 priority, int32 longReport);
void MallocReport(int32 argc, char **argv, int32 longReport)
{
	char *tag = NULL;
	int32 count = 10;
	if (argc>0) count = atoi(argv[0]);
	if (argc>1) tag = argv[1];
	grLeakyReport(count,tag,0,longReport);
};
#endif

void DebugHelp(int32 argc, char **argv)
{
	if (argc) {
		char *cmd = argv[0];
		for (int32 i=0;i<commandCount;i++) {
			DebugCmd *c = commands+i;
			if (strcasecmp(cmd,c->cmd)==0) {
				if (c->longHelp)
					DebugPrint((c->longHelp));
				else
					DebugPrintf(("Sorry, no help for command \"%s\".\n",c->cmd));
				return;
			};
		};
		DebugPrintf(("No such command: \"%s\"\n",cmd));
		return;
	};

	DebugPrint(("\n"));
	ShowKeys();
	ShowDebuggerCommands();
};

/********************************************************/

bool HideDebugScreen()
{
	debugScreenLock.Lock();
	if (debugScreenHidden) {
		debugScreenLock.Unlock();
		return false;
	};

	int32 realSize = debugScreenHeight;
	ResizeDebugScreen(-realSize);
	debugScreenHidden = true;
	debugScreenHeight = realSize;

	debugScreenLock.Unlock();
	return true;
};

bool ShowDebugScreen()
{
	debugScreenLock.Lock();
	if (!debugScreenHidden) {
		debugScreenLock.Unlock();
		return false;
	};

	int32 realSize = debugScreenHeight;
	debugScreenHeight = 0;
	debugScreenHidden = false;
	ResizeDebugScreen(realSize);

	debugScreenLock.Unlock();
	return true;
};

void ToggleDebugScreen()
{
	debugScreenLock.Lock();
	if (debugScreenHidden) ShowDebugScreen();
	else HideDebugScreen();
	debugScreenLock.Unlock();
};

void RedrawDebugScreen(int32 top, int32 bottom)
{
	if (bottom < top) return;

	debugScreenLock.Lock();
	RenderContext *c = &screenDebugContext[0];
	fpoint p,np;
	rect ir;
	ir.left = 0;
	ir.right = ScreenX(0)-1;
	ir.top = top;
	ir.bottom = bottom;

	AssertFont();

	region *tmpR = newregion();
	set_region(tmpR,&ir);
	grClipToRegion(c,tmpR);

	int32 firstLine = top / DEBUG_TEXT_HEIGHT;
	int32 bottomLine = (bottom+DEBUG_TEXT_HEIGHT-1) / DEBUG_TEXT_HEIGHT;

	np.h = 2; np.v = (firstLine+1)*DEBUG_TEXT_HEIGHT;

	firstLine = (firstLine + debugBufPtr + 1) % DEBUG_SCROLLBACK;
	bottomLine = (bottomLine + debugBufPtr + 1) % DEBUG_SCROLLBACK;

	if (ir.bottom > (TOTAL_HEIGHT-BOTTOM_MARGIN-DEBUG_TEXT_HEIGHT+3)) {
		ir.bottom = (TOTAL_HEIGHT-BOTTOM_MARGIN-DEBUG_TEXT_HEIGHT+3);
	};

	grLock(c);
		grGetPenLocation(c,&p);
		grSetForeColor(c,rgb_dk_gray);
		grFillIRect(c,ir);
		grSetForeColor(c,rgb_white);
		grSetBackColor(c,rgb_dk_gray);
		goto into;
		while (firstLine != bottomLine) {
			firstLine = (firstLine + 1) % DEBUG_SCROLLBACK;
			np.v += DEBUG_TEXT_HEIGHT;
			into:
			if (debugScrollback[firstLine]) {
				grSetPenLocation(c,np);
				grDrawString(c,(uchar*)debugScrollback[firstLine]);
			};
		};
		np.v = TOTAL_HEIGHT-2;
		ir.bottom = ir.top = ir.bottom+1;
		grFillIRect(c,ir);
		ir.top = ir.bottom+1;
		ir.bottom = TOTAL_HEIGHT-1;
		grSetForeColor(c,rgb_black);
		grFillIRect(c,ir);
		grSetForeColor(c,rgb_white);
		grSetBackColor(c,rgb_black);
		grSetPenLocation(c,np);
		grDrawString(c,(uchar*)debugCmd);
		grSetPenLocation(c,p);
	grUnlock(c);

	grClearClip(c);
	
	debugScreenLock.Unlock();
};

void ScrollDebugScreen(int32 delta)
{
	if (!delta) return;

	debugScreenLock.Lock();

	int32 newScroll = debugScrollingPos+delta;
	if (newScroll < 0) newScroll = 0;
	if (newScroll > (TOTAL_HEIGHT-debugScreenHeight))
		newScroll = (TOTAL_HEIGHT-debugScreenHeight);
	delta = newScroll - debugScrollingPos;
	if (!delta || debugScreenHidden) {
		debugScreenLock.Unlock();
		return;
	};

	RenderContext *c = &screenDebugContext[0];

	rect r;
	r.left = 0;
	r.right = ScreenX(0)-1;
	r.top = 0;
	r.bottom = TOTAL_HEIGHT-1;
	fpoint fp;
	point p;
	p.h = 0; p.v = -delta;
	grCopyPixels(c,r,p,true);
	
	debugScrollingPos = newScroll;
	fp.h = 0; fp.v = -debugScrollingPos;
	grSetOrigin(c,fp);

	int32 top,bottom;
	if (delta>0) {
		bottom = debugScrollingPos+debugScreenHeight-1;
		top = bottom - delta + 1;
		if (top < debugScrollingPos+1)
			top = debugScrollingPos+1;
	} else {
		top = debugScrollingPos+1;
		bottom = top - delta - 1;
		if (bottom > debugScrollingPos+debugScreenHeight-1)
			bottom = debugScrollingPos+debugScreenHeight-1;
	};
	RedrawDebugScreen(top,bottom);

	debugScreenLock.Unlock();
};

void PageUpDebugScreen()
{
	ScrollDebugScreen(-debugScreenHeight);
};

void PageDownDebugScreen()
{
	ScrollDebugScreen(debugScreenHeight);
};

void ResizeDebugScreen(int32 delta)
{
	if (!delta) return;

	debugScreenLock.Lock();

	int32 newDebugScreenHeight = debugScreenHeight + delta;
	int32 screenHeight = ScreenY(0)+debugScreenHeight;
	if (newDebugScreenHeight > screenHeight)
		newDebugScreenHeight = screenHeight;
	if (newDebugScreenHeight < 0)
		newDebugScreenHeight = 0;
	if (newDebugScreenHeight == debugScreenHeight) {
		debugScreenLock.Unlock();
		return;
	};

	int32 oldHeight = debugScreenHeight;
	debugScreenHeight = newDebugScreenHeight;

	if (debugScreenHidden) {
		debugScreenLock.Unlock();
		return;
	};

	RenderContext *c = &screenDebugContext[0];
	RenderPort *port = &screenDebugPort[0];

	region *tmpR = newregion();
	rect ir;
	ir.left = 0;
	ir.top = screenHeight-debugScreenHeight;
	ir.right = ScreenX(0)-1;
	ir.bottom = screenHeight-1;
	set_region(port->portClip,&ir);
	port->origin.h = 0;
	port->origin.v = screenHeight-debugScreenHeight;
	port->infoFlags = RPORT_RECALC_REGION | RPORT_RECALC_ORIGIN;
	
	fpoint oldPen;
	grGetPenLocation(c,&oldPen);

	ir.top = ir.bottom = 0;
	fpoint p;
	grPopState(c);
		grClearClip(c);
		grSetForeColor(c,rgb_gray);
		grFillIRect(c,ir);
		ir.top++;
		ir.bottom = debugScreenHeight-1;
		set_region(tmpR,&ir);
		grClipToRegion(c,tmpR);
	grPushState(c);

	debugScrollingPos -= debugScreenHeight-oldHeight;
	p.h = 0; p.v = -debugScrollingPos;
	grSetOrigin(c,p);
	grSetPenLocation(c,oldPen);

	RedrawDebugScreen(debugScrollingPos,debugScrollingPos+debugScreenHeight-oldHeight);

	BGraphicDevice::Device(0)->Canvas()->pixels.h = screenHeight-debugScreenHeight;
	ir.left = ir.top = 0;
	ir.right = ScreenX(0)-1;
	ir.bottom = screenHeight-debugScreenHeight-1;
	set_region(BGraphicDevice::Device(0)->Port()->portClip,&ir);
	set_region(BGraphicDevice::Device(0)->MousePort()->portClip,&ir);
	BGraphicDevice::Device(0)->m_formatInc++;
	BGraphicDevice::Device(0)->m_regionInc++;

	grLock(BGraphicDevice::Device(0)->Context());
	grUnlock(BGraphicDevice::Device(0)->Context());
	grLock(BGraphicDevice::Device(0)->MouseContext());
	grUnlock(BGraphicDevice::Device(0)->MouseContext());

	kill_region(tmpR);
	debugScreenLock.Unlock();
};

void DebugScreenOutputOneLine(char *string, bool advance)
{
	if (debugScrollingPos != TOTAL_HEIGHT-debugScreenHeight)
		ScrollDebugScreen(TOTAL_HEIGHT-debugScreenHeight - debugScrollingPos);

	rect r;
	r.left = 0;
	r.right = screenDebugCanvas[0].pixels.w-1;
	r.top = 0;
	r.bottom = TOTAL_HEIGHT-BOTTOM_MARGIN-DEBUG_TEXT_HEIGHT+1;
	fpoint fp;
	point p;
	p.h = 0; p.v = -DEBUG_TEXT_HEIGHT;

	RenderContext *c = &screenDebugContext[0];

	char **ptr = debugScrollback+debugBufPtr;
	if (*ptr) {
		char *n = (char*)dbMalloc(strlen(string)+strlen(*ptr)+1);
		strcpy(n,*ptr);
		strcat(n,string);
		dbFree(*ptr);
		*ptr = n;
	} else {
		int32 indent = debugIndent;
		if (debugIndent) {
			*ptr = (char*)dbMalloc(strlen(string)+indent+1);
			for (int32 i=0;i<indent;i++) (*ptr)[i] = ' ';
			(*ptr)[indent] = 0;
#if OUTPUT_TO_SERIAL
			_sPrintf("%s",*ptr);
#endif
#if OUTPUT_TO_STDOUT
			printf("%s",*ptr);
#endif
#if OUTPUT_TO_DEBUGPORT
			WriteToOutputPort(*ptr);
#endif
#if OUTPUT_TO_CONSOLE
			if (!debugScreenHidden) grDrawString(c,(uchar*)*ptr);
#endif
			strcat(*ptr,string);
		} else
			*ptr = dbStrdup(string);
	};

#if OUTPUT_TO_SERIAL
	_sPrintf(string);
	if (advance) _sPrintf("\n");
#endif
#if OUTPUT_TO_STDOUT
	printf(string);
	if (advance) printf("\n");
	fflush(stdout);
#endif
#if OUTPUT_TO_DEBUGPORT
	WriteToOutputPort(string);
	if (advance) WriteToOutputPort("\n");
#endif

#if OUTPUT_TO_CONSOLE
	if (!debugScreenHidden) {
		grSetForeColor(c,rgb_white);
		grSetBackColor(c,rgb_dk_gray);
		grDrawString(c,(uchar*)string);
	};
	
	if (advance) {
		debugBufPtr = (debugBufPtr+1) % DEBUG_SCROLLBACK;
		ptr = debugScrollback+debugBufPtr;
		if (*ptr) dbFree(*ptr);
		*ptr = NULL;

		if (!debugScreenHidden) {
			grCopyPixels(c,r,p,true);
			grSetForeColor(c,rgb_dk_gray);
			r.top = r.bottom - DEBUG_TEXT_HEIGHT + 2;
			RedrawDebugScreen(r.top,r.bottom);
			fp.h = 2; fp.v = TOTAL_HEIGHT-BOTTOM_MARGIN-DEBUG_TEXT_HEIGHT;
			grSetPenLocation(c,fp);
		};
	};
	
#endif
};

void AssertFont()
{
	if (!fontSet && !debugScreenHidden) {
		RenderContext *c = &screenDebugContext[0];
		grPopState(c);

		fc_context_packet fc;
		fc.size = debugFontSize;
		fc.f_id = fc_get_family_id(debugFont);
		fc.s_id = fc_get_default_style_id(fc.f_id);
		grSetFontFromPacket(c,&fc,sizeof(fc),B_FONT_FAMILY_AND_STYLE|B_FONT_SIZE);

		grPushState(c);
		fpoint fp; fp.h = 0; fp.v = -debugScrollingPos;
		grSetOrigin(c,fp);
		fp.h = 2; fp.v = TOTAL_HEIGHT-BOTTOM_MARGIN-DEBUG_TEXT_HEIGHT;
		grSetPenLocation(c,fp);

		fontSet = true;
	};
};

void DebugScreenOutput(char *_string)
{
	if (!_string) return;

	AssertFont();

	char *string = dbStrdup(_string);
	char *stringToFree = string;

	debugScreenLock.Lock();
	char *end=string;
	while (*end != 0) {
		if (*end == '\n') {
			*end = 0;
			DebugScreenOutputOneLine(string,true);
			string = end+1;
		};
		end++;
	};
	if (*string) DebugScreenOutputOneLine(string,false);
	debugScreenLock.Unlock();

	dbFree(stringToFree);
};

void DebugScreenInput(int32 key, uint32 mods, char *s)
{
	debugScreenLock.Lock();

	if (mods & SHIFT_KEY) {
		switch (key) {
			case 0x57: // B_UP_ARROW
				ScrollDebugScreen(-13);
				break;
			case 0x62: // B_DOWN_ARROW
				ScrollDebugScreen(13);
				break;
			case 0x21: // B_PAGE_UP
				PageUpDebugScreen();
				break;
			case 0x36: // B_PAGE_DOWN
				PageDownDebugScreen();
				break;
		};
		goto exit;
	} else if (mods & CONTROL_KEY) {
		/* Resize the debugging window */
		if ((key == 0x57) || (key == 0x62)) {
			point p;
			StartDebugScreenManip(&p);
			if (key == 0x57) // B_UP_ARROW
				ResizeDebugScreen(13);
			else if (key == 0x62) // B_DOWN_ARROW
				ResizeDebugScreen(-13);
			EndDebugScreenManip(p);
		};
		goto exit;
	} else if (mods & COMMAND_KEY) {
		DebugPrintf(("debug key_down = 0x%x, bytes[0]=%d\n",key,s[0]));
		goto exit;
	} else {
		if (key == 0x1f) { // B_INSERT
			point p;
			StartDebugScreenManip(&p);
			ToggleDebugScreen();
			EndDebugScreenManip(p);
			goto exit;
		};
	};

	if (debugScrollingPos != TOTAL_HEIGHT-debugScreenHeight)
		ScrollDebugScreen(TOTAL_HEIGHT-debugScreenHeight - debugScrollingPos);

	switch (key) {
		case 0x1e: // B_BACKSPACE
		{
			int32 len = strlen(debugCmd);
			if (len) debugCmd[len-1] = 0;
			break;
		}
		case 0x47: // B_ENTER
		{
			int32 len = strlen(debugCmd);
			if (!len) break;
			DebugSendCommand(debugCmd);
			debugCmd[0] = 0;
			break;
		}
		default: // Some other key
			if (strchr(okayInput,s[0]))
				strncat(debugCmd,s,255);
			break;
	};

	RedrawDebugScreen(TOTAL_HEIGHT-BOTTOM_MARGIN-DEBUG_TEXT_HEIGHT,TOTAL_HEIGHT-1);

	exit:
	debugScreenLock.Unlock();
};

void SyncScreenInternal(display_mode *mode, RenderContext *context, RenderPort *port, RenderCanvas *canvas)
{
	int32 pixelFormat;
	uint8 endianess;

	*canvas = *BGraphicDevice::Device(0)->Canvas();

	rect r;
	port->origin.h = port->origin.v = r.left = r.top = 0;
	r.right = canvas->pixels.w-1;
	r.bottom = canvas->pixels.h-1;
	set_region(port->portClip,&r);
};

void SyncDebugScreen(int32 screenIndex, display_mode *mode)
{
	debugScreenLock.Lock();

	int32 initialHeight = INITIAL_DEBUG_SCREEN_HEIGHT;
	SyncScreenInternal(mode,
		&screenDebugContext[screenIndex],
		&screenDebugPort[screenIndex],
		&screenDebugCanvas[screenIndex]);

	if (okayToScreen) {
		grPopState(&screenDebugContext[screenIndex]);
		initialHeight = debugScreenHeight;
		debugScreenHeight = 0;
	};
	
	RenderPort *port = &screenDebugPort[screenIndex];
	clear_region(port->portClip);
	port->origin.h = 0;
	port->origin.v = mode->timing.v_display;
	port->infoFlags = RPORT_RECALC_REGION | RPORT_RECALC_ORIGIN;

	if (!okayToScreen) {
		for (int32 i=0;i<DEBUG_SCROLLBACK;i++)
			debugScrollback[i] = NULL;
	};
	okayToScreen = true;

	grPushState(&screenDebugContext[screenIndex]);

	fpoint fp; fp.h = 0; fp.v = -debugScrollingPos;
	grSetOrigin(&screenDebugContext[screenIndex],fp);

	fp.h = 2; fp.v = TOTAL_HEIGHT-BOTTOM_MARGIN-DEBUG_TEXT_HEIGHT;
	grSetPenLocation(&screenDebugContext[screenIndex],fp);

	ResizeDebugScreen(initialHeight);

	debugScreenLock.Unlock();
};

void * FakeLock(void *ptr)
{
	return ptr;
};

void UnlockDebugPort(RenderPort *port)
{
	port->infoFlags = 0;
};

void UnlockDebugCanvas(RenderCanvas *canvas)
{
};

extern void *LockScreenAccel(AccelPackage *accel, engine_token **token, sync_token *syncToken);
extern void UnlockScreenAccel(AccelPackage *accel, engine_token **token, sync_token *syncToken);
void load_symbols();

void InitDebugScreen(int32 screenIndex)
{
	grInitRenderContext(&screenDebugContext[screenIndex]);
	grInitRenderPort(&screenDebugPort[screenIndex]);
	grSetContextPort(&screenDebugContext[screenIndex],&screenDebugPort[screenIndex],
		(LockRenderPort)FakeLock,(UnlockRenderPort)UnlockDebugPort);
	grSetPortCanvas(&screenDebugPort[screenIndex],&screenDebugCanvas[screenIndex],
		(LockRenderCanvas)FakeLock,(UnlockRenderCanvas)UnlockDebugCanvas);
	grSetCanvasAccel(&screenDebugCanvas[screenIndex],&screenDebugCanvas[screenIndex],
		(LockAccelPackage)LockScreenAccel,(UnlockAccelPackage)UnlockScreenAccel);
};

int32 trackThread = -2;
void TrackThread(int32 argc, char **argv)
{
	int32 tid = atoi(argv[0]);
	trackThread = tid;
};

void init_debugger()
{
	load_symbols();

	debugCmd[0] = 0;
	/* Add standard debugger commands here */
	AddDebuggerCommand("areafree",(void *)Area::DumpAreas,NULL,
		"Show the area free list which the app_server maintains",
		NULL);
	AddDebuggerCommand("atom",(void *)DebugNodeInfo,&dots[1],
		"Show info about an atom (see help for args)",
		"Usage is 'atom <atom_ID>', where the atom_ID can be\n"
		"the atom's address OR the atom's token.  If it is in\n"
		"hex, (i.e. 0x*) it is assumed to be the address, otherwise\n"
		"it is treated as a token\n");
	AddDebuggerCommand("calltree",(void *)CallTreeDisplay,NULL,
		"By itself for the list of call trees, or with a call tree name for a report",
		"calltree                  - For the list\n"
		"calltree <name>           - For a short report\n"
		"calltree <name> <#>       - For a short report of # entries\n"
		"calltree <name> <#> long  - For a long report of # entries\n");
	AddDebuggerCommand("help",(void *)DebugHelp,NULL,
		"By itself for this list, or with a command name for usage",
		"Smartass.\n");
	AddDebuggerCommand("lookup",(void *)LookupSym,NULL,
		"Lookup a symbol",
		"lookup <address>\n");
#ifdef INTERNAL_LEAKCHECKING
	AddDebuggerCommand("lleaks",(void *)MallocReport,(void*)1,
		"Long list outstanding mallocs",
		"By default, it lists the top 10 outstanding malloc locations,\n"
		"prioritized by the number of calls.  The first possible argument\n"
		"is the number of entries to list.  The second is the tag to limit the\n"
		"listing to.  I.E. \"mallocs 25 uselesscrap\" will list the top 25\n"
		"malloc locations that tag the malloc with the tag \"uselesscrap\".\n");
	AddDebuggerCommand("leaks",(void *)MallocReport,NULL,
		"List outstanding mallocs",
		"By default, it lists the top 10 outstanding malloc locations,\n"
		"prioritized by the number of calls.  The first possible argument\n"
		"is the number of entries to list.  The second is the tag to limit the\n"
		"listing to.  I.E. \"mallocs 25 uselesscrap\" will list the top 25\n"
		"malloc locations that tag the malloc with the tag \"uselesscrap\".\n");
	AddDebuggerCommand("lmalloc",(void *)LookupMalloc,NULL,
		"Lookup a memory allocation",
		"");
#endif
	AddDebuggerCommand("rsem",(void *)RegionSem,NULL,
		"Dump info about or manipulate region_sem",
		"rsem   : Display ownership and disposition of region_sem\n"
		"rsem r : Release region_sem (dangerous!)\n"
		"rsem a : Acquire region_sem (dangerous!)\n");
	AddDebuggerCommand("setvar",(void *)SetVar,NULL,
		"Set the value of a debugging variable",
		"Usage is \"setvar <varnum> <value>\" where <varnum>\n"
		"is a value from 0 to 9 and <value> is any 32-bit value\n");
	AddDebuggerCommand("mark",(void *)MarkAtoms,NULL,
		"Start a new atom leak context",
		NULL);
	AddDebuggerCommand("atoms",(void *)DumpAtoms,NULL,
		"Dump info about atoms",
		"atoms         : Display all existing BAtom instances\n"
		"atoms current : Display existing BAtoms in current context\n"
		"atoms last    : Display existing BAtoms in last context\n");
	AddDebuggerCommand("tokens",(void *)DumpTokens,NULL,
		"Dump info about tokens (optional args: owner type)",
		NULL);
	AddDebuggerCommand("track",(void *)TrackThread,NULL,
		"Enable tracking for a window thread (usage: \"track <threadID>\")",
		NULL);
	AddDebuggerCommand("view",(void *)DebugNodeInfo,&dots[2],
		"Show info about a view (see help for args)",
		"Usage is 'view <view_ID>', where the view_ID can be\n"
		"the view's address OR the view's token.  If it is in\n"
		"hex, (i.e. 0x*) it is assumed to be the address, otherwise\n"
		"it is treated as a token\n");
	AddDebuggerCommand("window",(void *)DebugNodeInfo,&dots[0],
		"Show info about a window (see help for args)",
		"Usage is 'window <window_ID>', where the window_ID can be\n"
		"the window's address OR the window's token.  If it is in\n"
		"hex, (i.e. 0x*) it is assumed to be the address, otherwise\n"
		"it is treated as a token\n");

	for (int32 i=0;i<10;i++) debugVar[i] = 0;
	xspawn_thread((thread_entry)DebugInputThread,"DebugInput",B_NORMAL_PRIORITY,NULL);
	xspawn_thread((thread_entry)DebugOutputThread,"DebugOutput",B_NORMAL_PRIORITY,NULL);
};

void DebugSendCommand(char *cmd)
{
	write_port_etc(debugInputPort,'icmd',cmd,strlen(cmd)+1,B_TIMEOUT,500000);
};

void DebugOutputThread()
{
	char *msg;

	debugOutputSem = create_sem(0,"app_server:debugOutput");
	while (1) {
		acquire_sem(debugOutputSem);
		debugOutputPendingLock.Lock();
		msg = debugOutputPending;
		debugOutputPending = NULL;
		debugOutputPendingLock.Unlock();
		if (!msg) DebugScreenOutput("DebugOutputThread triggered, but no debug output pending!\n");
		else DebugScreenOutput(msg);
		dbFree(msg);
	};
};

void QueueOutput(char *s)
{
	char *old;
	int32 size;

	debugOutputPendingLock.Lock();
	old = debugOutputPending;
	size = strlen(s) + 1;
	if (old) size += strlen(old);
	debugOutputPending = (char*)dbMalloc(size);
	debugOutputPending[0] = 0;
	if (old) strcat(debugOutputPending,old);
	strcat(debugOutputPending,s);
	debugOutputPendingLock.Unlock();
	
	if (!old)
		release_sem(debugOutputSem);
	else
		dbFree(old);
};

extern char app_server_name[80];

void DebugInputThread()
{
	int32 msg;
	char msgBuffer[1024];

	sprintf(msgBuffer,"%s:debugInput",app_server_name);
	debugInputPort = create_port(1,msgBuffer);
	while (1) {
		read_port(debugInputPort,&msg,msgBuffer,1024);
		if (msg == 'icmd') {
			if (msgBuffer[0] != 0) {
				if (!DebugProcessCommand(msgBuffer)) {
					DebugPrintf(("No such command: \"%s\"\n",msgBuffer));
				};
			};
		} else if (msg == '_cmd') {
			/*	A little different than the internal command.  See if it's understood... if
				not, echo it back out of the debug output with a special code. */
			if (msgBuffer[0] != 0) {
				if (!DebugProcessCommand(msgBuffer)) {
					write_port_etc(debugOutputPort,'echo',msgBuffer,
						strlen(msgBuffer)+1,B_TIMEOUT,2000000);
				};
			};
		} else if (msg == 'oprt') {
			port_id p = find_port(msgBuffer);
			if (p != B_BAD_VALUE) {
				DebugPrintf(("Connected with external debug console on port %d:\"%s\"\n",p,msgBuffer));
				debugOutputPort = p;
				sprintf(msgBuffer,"Connected to app_server on port %ld...\n",debugOutputPort);
				WriteToOutputPort(msgBuffer);
			};
		};
	};
};

void WriteToOutputPort(char *msg)
{
	if (debugOutputPort != B_BAD_VALUE) {
		if (write_port_etc(debugOutputPort,'dmsg',msg,strlen(msg)+1,B_TIMEOUT,2000000) != B_OK) {
			debugOutputPort = B_BAD_VALUE;
		};
	};
};

#endif

void DebugPrintfReal(const char *format, ...)
{
	va_list 	args;
	char		s[8192];
	
	va_start(args, format);
	vsprintf(s, format, args);
#if AS_DEBUG
	if (okayToScreen) {
		QueueOutput(s);
	} else {
		printf(s);
	};
#else
	printf(s);
#endif
	va_end(args);
};

void DebugPrintReal(char *str)
{
#if AS_DEBUG
	if (okayToScreen) {
		QueueOutput(str);
	} else {
		printf(str);
	};
#else
	printf(str);
#endif
};

void DebugPushIndent(int32 indent)
{
	atomic_add(&debugIndent,indent);
};

void DebugPopIndent(int32 indent)
{
	atomic_add(&debugIndent,-indent);
};

void ShowRegionReal(region *r, char *s)
{
	rect tmp,bnd;
	if (!r) DebugPrintf(("\"%s\" = NULL\n",s));
	else if (r->CountRects() > 0) {
		DebugPrintf(("\"%s\" = %d %d 0x%08x {\n",s,r->CountRects(),r->CountAvail(),r->Rects()));
		DebugPrintf(("   {%d,%d,%d,%d}",
			r->Rects()[0].left,r->Rects()[0].top,r->Rects()[0].right,r->Rects()[0].bottom));
		bnd = r->Rects()[0];
		for (int i=1;i<r->CountRects();i++) {
			tmp = r->Rects()[i];
			if (tmp.top < bnd.top)
				bnd.top = tmp.top;
			if (tmp.bottom > bnd.bottom)
				bnd.bottom = tmp.bottom;
			if (tmp.left < bnd.left)
				bnd.left = tmp.left;
			if (tmp.right > bnd.right)
				bnd.right = tmp.right;
			DebugPrintf((",\n   {%d,%d,%d,%d}",
				r->Rects()[i].left,r->Rects()[i].top,r->Rects()[i].right,r->Rects()[i].bottom));
		};
		DebugPrintf(("\n}\n"));
		DebugPrintf(("\"%s\".bound = {%d,%d,%d,%d}\n",s,
			r->Bounds().left,r->Bounds().top,r->Bounds().right,r->Bounds().bottom));
		DebugPrintf(("\"%s\".(real)bound = {%d,%d,%d,%d}\n",s,
			bnd.left,bnd.top,bnd.right,bnd.bottom));
		
	} else DebugPrintf(("\"%s\" = <empty>\n",s));
};

struct symbol {
	uint32	addr;
	uint32	name;
};

char *			symbolNames = NULL;
int32			symbolNamesSize = 0;
int32			symbolNamesPtr = 0;
BArray<symbol>	symbolTable;
uint32			_startIsAt;
uint32			after_start;

int symbol_cmp(const void *p1, const void *p2)
{
	const symbol *sym1 = (const symbol*)p1;
	const symbol *sym2 = (const symbol*)p2;
	return (sym1->addr == sym2->addr)?0:
			((sym1->addr > sym2->addr)?1:-1);
};

void load_symbols()
{
	image_info info;
	char name[255];
	void *location;
	int32 symType,cookie=0,n=0,nameLen=255;
	symbol sym;
	while (get_next_image_info(0, &cookie, &info) == B_OK) {
//		if (info.type == B_APP_IMAGE) {
			image_id id = info.id;
			n = 0;
			while (get_nth_image_symbol(id,n,name,&nameLen,&symType,&location) == B_OK) {
				while ((symbolNamesPtr + nameLen) > symbolNamesSize) {
					if (symbolNamesSize > 0) symbolNamesSize *= 2;
					else symbolNamesSize = 1024;
					symbolNames = (char*)dbRealloc(symbolNames,symbolNamesSize);
				};
				sym.addr = (uint32)location;
				sym.name = symbolNamesPtr;
				symbolTable.AddItem(sym);
				strcpy(symbolNames+symbolNamesPtr,name);
				symbolNamesPtr += nameLen;
				nameLen = 255;
				symType = B_SYMBOL_TYPE_ANY;
				n++;
			};
//			break;
//		};
	};
	qsort(symbolTable.Items(),symbolTable.CountItems(),sizeof(symbol),&symbol_cmp);
	int32 count = symbolTable.CountItems();
	_startIsAt = 0xFFFFFFFF;
	after_start = 0xFFFFFFFE;
	for (int32 i=0;i<count;i++) {
		if (strcmp(symbolNames + symbolTable[i].name,"exit_thread") == 0) {
			_startIsAt = symbolTable[i].addr;
			if (i == (count-1))
				after_start = _startIsAt + 0x500;
			else
				after_start = symbolTable[i+1].addr;
			break;
		};
	};
};

bool is_top(uint32 addr)
{
	if ((addr >= _startIsAt) && (addr < after_start)) return true;
	return false;
};

const char * lookup_symbol(uint32 addr, uint32 *offset)
{
	int32 i;
	if (symbolNames == NULL) load_symbols();
	if (symbolNames == NULL) return "";
	if ((addr >= _startIsAt) && (addr < after_start)) return "exit_thread";
	int32 count = symbolTable.CountItems();
	for (i=0;i<(count-1);i++) {
		if ((addr >= symbolTable[i].addr) && (addr < symbolTable[i+1].addr)) break;
	};
	*offset = addr - symbolTable[i].addr;
	return symbolNames + symbolTable[i].name;
};
