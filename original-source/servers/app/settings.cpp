
#include <Debug.h>
#include <string.h>
#include <FindDirectory.h>
#include <OS.h>
#include <String.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <stdarg.h>
#include "as_support.h"
#include "proto.h"
#include "gr_types.h"
#include "settings.h"
#include "server.h"
#include "nuDecor.h"

extern	int32				ws_count;
extern	display_mode		ws_res[32];
extern	rgb_color			desk_rgb[MAX_WORKSPACE];
		bool				scroll_proportional = TRUE;
		bool				scroll_double_arrows = TRUE;
		int32				scroll_knob = 1;
		int32				scroll_knob_size = 15;
		bool				loadEntireFontFile = false;		
		unneeded_cursor_mode	cursorReleasedMode = kUnneededCursorHidden;
		unneeded_cursor_mode	cursorPressedMode = kUnneededCursorPoint;

extern char settingsFileName[128];

/*-------------------------------------------------------------*/

typedef void * (*SettingFunc)(void *parentToken, FILE *args);

struct SettingNode {
	SettingNode(char *_tag, SettingFunc _func, SettingNode *_children, SettingNode *_next);
	~SettingNode();

	char *tag;
	SettingFunc func;
	SettingNode *children;
	SettingNode *next;
};

SettingNode *settingsTop;

SettingNode::SettingNode(
	char *_tag, SettingFunc _func,
	SettingNode *_children, SettingNode *_next)
{
	tag = grStrdup(_tag,"settings tag",MALLOC_CANNOT_FAIL);
	func = _func;
	children = _children;
	next = _next;
};

SettingNode::~SettingNode()
{
	grFree(tag);
	SettingNode *n;
	while (children) {
		n = children;
		children = children->next;
		delete n;
	};
};

/*-------------------------------------------------------------*/

class SettingsFile {
	
	public:
				SettingsFile();

		void	ReadSettings(SettingNode *root);
		void	BeginWritingSettings();
		void	WriteSetting(const char *str, ...);
		void	OpenSettingGroup(const char *str, ...);
		void	CloseSettingGroup();
		void	EndWritingSettings();

	private:
	
		int32	m_indent;
		char	m_path[256];
		FILE *	m_file;
};

static SettingsFile * settingsFile;

void settings_path(const char *filename, char *path, int32 pathLen)
{
	status_t result = find_directory(B_USER_SETTINGS_DIRECTORY, -1, true, path, pathLen);
	if (result != B_OK) strcpy(path,"/boot");
	strcat(path, "/");
	strcat(path, filename);
};

static bool read_string(char* into, size_t len, FILE *from)
{
	if (len < 2)
		return false;
	len--;
	
	size_t i=0;
	int c;
	while ((c=fgetc(from)) != EOF && isspace(c)) ;
	
	if (c != EOF) {
		if (c == '"') {
			while (i<len && (c=fgetc(from)) != EOF && c != '"')
				into[i++] = (char)c;
		} else {
			into[i++] = (char)c;
			while (i<len && (c=fgetc(from)) != EOF && !isspace(c))
				into[i++] = (char)c;
			if (c != EOF)
				ungetc(c, from);
		}
	}
	into[i] = 0;
	return true;
}

struct settings_enum_item {
	const char* name;
	int32 value;
};

static int32 read_enum(const settings_enum_item* items, FILE *from, bool* found = NULL)
{
	char name[128];
	read_string(name, sizeof(name), from);
	while (items->name) {
		if (strcasecmp(name, items->name) == 0) {
			if (found) *found = true;
			return items->value;
		}
		items++;
	}
	if (found) *found = false;
	return -1;
}

static void write_enum(const settings_enum_item* items, int32 value,
						const char* name, SettingsFile* file)
{
	while (items->name) {
		if (items->value == value) {
			file->WriteSetting("%s %s", name, items->name);
			return;
		}
		items++;
	}
}

SettingsFile::SettingsFile()
{
	status_t result = find_directory(B_USER_SETTINGS_DIRECTORY, -1, true, m_path, 256);
	if (result != B_OK) strcpy(m_path,"/boot");
	strcat(m_path, "/");
	strcat(m_path, settingsFileName);
};

void SettingsFile::ReadSettings(SettingNode *root)
{
	m_file = fopen(m_path,"r");
	if (!m_file) return;

	int32 nesting = 0;
	void *tokens[32],*newToken;
	SettingNode *levels[32];
	tokens[0] = NULL;
	levels[0] = root;
	char tag[128];

	if (!m_file) return;
	if (fscanf(m_file,"%s",tag) == 1) {
		while (1) {
			SettingNode *n = levels[nesting];
			if (tag[0] == '}') {
				if (--nesting < 0) {
					DebugPrintf(("Negative nesting while reading settings\n"));
					return;
				};
				if (fscanf(m_file,"%s",tag) != 1) {
					if (nesting) DebugPrintf(("Settings file ending unexpectedly...\n"));
					return;
				};
				continue;
			};
			while (n && (strcmp(tag,n->tag) != 0)) n = n->next;
			if (n == NULL) {
				DebugPrintf(("Unknown tag while reading settings\n"));
				return;
			};

			if (n->func) {
				tokens[nesting+1] = n->func(tokens[nesting],m_file);
			} else {
				tokens[nesting+1] = NULL;
			};
			
			if (fscanf(m_file,"%s",tag) != 1) {
				DebugPrintf(("Settings file ending unexpectedly...\n"));
				return;
			};
	
			if (tag[0] == '{') {
				levels[++nesting] = n->children;
				if (fscanf(m_file,"%s",tag) != 1) {
					DebugPrintf(("Settings file ending unexpectedly...\n"));
					return;
				};
			};
		};
	};
	
	fclose(m_file);
};

void SettingsFile::BeginWritingSettings()
{
	m_file = fopen(m_path,"w+");
	m_indent = 0;
};

void SettingsFile::WriteSetting(const char *str, ...)
{
	va_list args;
	va_start(args, str);
	
	if (!m_file) return;
	for (int32 i=0;i<m_indent;i++) fprintf(m_file,"\t");
	vfprintf(m_file,str,args);
	fprintf(m_file,"\n");
};

void SettingsFile::OpenSettingGroup(const char *str, ...)
{
	va_list args;
	va_start(args, str);
	
	if (!m_file) return;
	for (int32 i=0;i<m_indent;i++) fprintf(m_file,"\t");
	vfprintf(m_file,str,args);
	fprintf(m_file," {\n");
	m_indent++;
};

void SettingsFile::CloseSettingGroup()
{
	m_indent--;
	if (!m_file) return;
	for (int32 i=0;i<m_indent;i++) fprintf(m_file,"\t");
	fprintf(m_file,"}\n");
};

void SettingsFile::EndWritingSettings()
{
	if (!m_file) return;
	fclose(m_file);
};

/*-------------------------------------------------------------*/

extern char PreviousDevice[256];
void * DeviceName(void *token, FILE *args)
{
	fscanf(args, " %255s", PreviousDevice);
	PreviousDevice[255] = '\0';
	return NULL;
}

void * WorkspaceCount(void *token, FILE *args)
{
	fscanf(args,"%ld",&ws_count);
	if (ws_count > MAX_WORKSPACE) ws_count = MAX_WORKSPACE;
	return NULL;
};

void * WorkspaceToken(void *token, FILE *args)
{
	int32 workspace;
	fscanf(args,"%ld",&workspace);
	return (void*)workspace;
};

void * WorkspaceTiming(void *token, FILE *args)
{
	fscanf(args, "%lu  %hd %hd %hd %hd  %hd %hd %hd %hd  %lx",
		&ws_res[(int32)token].timing.pixel_clock,
		&ws_res[(int32)token].timing.h_display,
		&ws_res[(int32)token].timing.h_sync_start,
		&ws_res[(int32)token].timing.h_sync_end,
		&ws_res[(int32)token].timing.h_total,
		&ws_res[(int32)token].timing.v_display,
		&ws_res[(int32)token].timing.v_sync_start,
		&ws_res[(int32)token].timing.v_sync_end,
		&ws_res[(int32)token].timing.v_total,
		&ws_res[(int32)token].timing.flags
		);
	return NULL;
};

void * WorkspaceVirtual(void *token, FILE *args)
{
	fscanf(args, "%hd %hd",
		&ws_res[(int32)token].virtual_width,
		&ws_res[(int32)token].virtual_height);
	return NULL;
};

void * WorkspaceFlags(void *token, FILE *args)
{
	fscanf(args, "%lx", &ws_res[(int32)token].flags);
	return NULL;
};

void * WorkspaceDepth(void *token, FILE *args)
{
	fscanf(args,"%lx",&ws_res[(int32)token].space);
	return NULL;
};

void * WorkspaceColor(void *token, FILE *args)
{
	int32 r,g,b;
	fscanf(args,"%2lx%2lx%2lx",&r,&g,&b);
	desk_rgb[(int32)token].red = r;
	desk_rgb[(int32)token].green = g;
	desk_rgb[(int32)token].blue = b;
	desk_rgb[(int32)token].alpha = 255;
	return NULL;
};

/*-------------------------------------------------------------*/

void * ScrollbarProp(void *token, FILE *args)
{
	int32 i;
	fscanf(args,"%ld",&i);
	scroll_proportional = i;
	return NULL;
};

void * ScrollbarArrows(void *token, FILE *args)
{
	int32 i;
	fscanf(args,"%ld",&i);
	scroll_double_arrows = (i==2);
	return NULL;
};

void * ScrollbarKnob(void *token, FILE *args)
{
	fscanf(args,"%ld",&scroll_knob);
	if ((scroll_knob < 0) || (scroll_knob > 2))
		scroll_knob = 1;
	return NULL;
};

void * ScrollbarKnobSize(void *token, FILE *args)
{
	fscanf(args,"%ld",&scroll_knob_size);
	if ((scroll_knob_size < 8) || (scroll_knob_size > 50))
		scroll_knob_size = 15;
	return NULL;
};

/*-------------------------------------------------------------*/

void * FFMSetting(void *token, FILE *args)
{
	int32 i;
	fscanf(args,"%ld",&i);
	the_server->target_mouse = i;
	return NULL;
};

void * DecorSetting(void *token, FILE *args)
{
	char name[128];
	read_string(name, sizeof(name), args);
	SelectDecor(name, false);
	return NULL;
};

/*-------------------------------------------------------------*/

static const settings_enum_item antialiasing_enum[] = {
	{ "off", B_DISABLE_ANTIALIASING },
	{ "on", B_NORMAL_ANTIALIASING },
	{ "tv", B_TV_ANTIALIASING },
	{ NULL, 0 }
};

void * AntiAliasing(void *token, FILE *args)
{
	int32 val = read_enum(antialiasing_enum, args);
	if (val >= 0)
		SFont::SetPreferredAntialiasing(val);
	return NULL;
}

void * AliasingMin(void *token, FILE *args)
{
	int32 i;
	fscanf(args,"%ld",&i);
	SFont::SetAliasingMinLimit(i);
	return NULL;
}

void * AliasingMax(void *token, FILE *args)
{
	int32 i;
	fscanf(args,"%ld",&i);
	SFont::SetAliasingMaxLimit(i);
	return NULL;
}

void * HintingMax(void *token, FILE *args)
{
	int32 i;
	fscanf(args,"%ld",&i);
	SFont::SetHintingMaxLimit(i);
	return NULL;
}

void * LoadEntireFontFile(void *token, FILE *args)
{
	int32 i;
	fscanf(args,"%ld",&i);
	loadEntireFontFile = i;
	return NULL;
}

/*-------------------------------------------------------------*/

void * ReleasedCursorSetting(void *token, FILE *args)
{
	int32 i;
	fscanf(args,"%ld",&i);
	cursorReleasedMode = (unneeded_cursor_mode)i;
	return NULL;
};

void * PressedCursorSetting(void *token, FILE *args)
{
	int32 i;
	fscanf(args,"%ld",&i);
	cursorPressedMode = (unneeded_cursor_mode)i;
	return NULL;
};

/*-------------------------------------------------------------*/

void ReadSettings()
{
	settingsFile = new SettingsFile();
	settingsTop =
	new SettingNode("Device", DeviceName,NULL,
	new SettingNode("Workspaces",WorkspaceCount,
		new SettingNode("Workspace",WorkspaceToken,
			new SettingNode("timing", WorkspaceTiming,NULL,
			new SettingNode("colorspace",WorkspaceDepth,NULL,
			new SettingNode("virtual",WorkspaceVirtual,NULL,
			new SettingNode("flags",WorkspaceFlags,NULL,
			new SettingNode("color",WorkspaceColor,NULL,NULL))))),
		NULL),
	new SettingNode("Interface",NULL,
		new SettingNode("Scrollbars",NULL,
			new SettingNode("proportional",ScrollbarProp,NULL,
			new SettingNode("arrows",ScrollbarArrows,NULL,
			new SettingNode("knobtype",ScrollbarKnob,NULL,
			new SettingNode("knobsize",ScrollbarKnobSize,NULL,NULL)))),
		new SettingNode("Options",NULL,
			new SettingNode("ffm",FFMSetting,NULL,
			new SettingNode("decor_name",DecorSetting,NULL,
			new SettingNode("released_cursor_shown",ReleasedCursorSetting,NULL,
			new SettingNode("pressed_cursor_shown",PressedCursorSetting,NULL,NULL)))),
		NULL)),
	new SettingNode("Fonts",NULL,
		new SettingNode("antialiasing",AntiAliasing,NULL,
		new SettingNode("aliasing_min",AliasingMin,NULL,
		new SettingNode("aliasing_max",AliasingMax,NULL,
		new SettingNode("hinting_max",HintingMax,NULL,
		new SettingNode("load_entire_font_file", LoadEntireFontFile,NULL,NULL))))),
	NULL))));

	settingsFile->ReadSettings(settingsTop);
};

void WriteSettings()
{
	BString decorName;
	GetCurrentDecor(&decorName);
	
	int32 i;
	settingsFile->BeginWritingSettings();

	settingsFile->WriteSetting("Device %s", PreviousDevice);
	settingsFile->OpenSettingGroup("Workspaces %d",ws_count);
		for (i=0;i<32;i++) {
			settingsFile->OpenSettingGroup("Workspace %d",i);
				settingsFile->WriteSetting("timing %u  %d %d %d %d  %d %d %d %d  0x%08x",
					ws_res[i].timing.pixel_clock,
					ws_res[i].timing.h_display,
					ws_res[i].timing.h_sync_start,
					ws_res[i].timing.h_sync_end,
					ws_res[i].timing.h_total,
					ws_res[i].timing.v_display,
					ws_res[i].timing.v_sync_start,
					ws_res[i].timing.v_sync_end,
					ws_res[i].timing.v_total,
					ws_res[i].timing.flags);
				settingsFile->WriteSetting("colorspace 0x%08x",ws_res[i].space);
				settingsFile->WriteSetting("virtual %d %d",ws_res[i].virtual_width, ws_res[i].virtual_height);
				settingsFile->WriteSetting("flags 0x%08x",ws_res[i].flags);
				settingsFile->WriteSetting("color %02x%02x%02x",desk_rgb[i].red,desk_rgb[i].green,desk_rgb[i].blue);
			settingsFile->CloseSettingGroup();
		};
	settingsFile->CloseSettingGroup();
	settingsFile->OpenSettingGroup("Interface");
		settingsFile->OpenSettingGroup("Scrollbars");
			settingsFile->WriteSetting("proportional %d",(int32)scroll_proportional);
			settingsFile->WriteSetting("arrows %d",(int32)(scroll_double_arrows?2:1));
			settingsFile->WriteSetting("knobtype %d",scroll_knob);
			settingsFile->WriteSetting("knobsize %d",scroll_knob_size);
		settingsFile->CloseSettingGroup();
		settingsFile->OpenSettingGroup("Options");
			settingsFile->WriteSetting("ffm %d",(int32)the_server->target_mouse);
			if (decorName.Length() > 0)
				settingsFile->WriteSetting("decor_name \"%s\"",decorName.String());
			settingsFile->WriteSetting("released_cursor_shown %d",cursorReleasedMode);
			settingsFile->WriteSetting("pressed_cursor_shown %d",cursorPressedMode);
		settingsFile->CloseSettingGroup();
	settingsFile->CloseSettingGroup();
	settingsFile->OpenSettingGroup("Fonts");
		write_enum(antialiasing_enum, SFont::PreferredAntialiasing(), "antialiasing", settingsFile);
		settingsFile->WriteSetting("aliasing_min %d", (int32)SFont::AliasingMinLimit());
		settingsFile->WriteSetting("aliasing_max %d", (int32)SFont::AliasingMaxLimit());
		settingsFile->WriteSetting("hinting_max %d", (int32)SFont::HintingMaxLimit());
		settingsFile->WriteSetting("load_entire_font_file %d", (int32)loadEntireFontFile);
	settingsFile->CloseSettingGroup();
	settingsFile->EndWritingSettings();
};
