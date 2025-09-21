
extern void ReadSettings();
extern void WriteSettings();
extern void settings_path(const char *filename, char *path, int32 pathLen);

extern bool		scroll_proportional,scroll_double_arrows;
extern int32	scroll_knob,scroll_knob_size;
extern bool		loadEntireFontFile;

enum unneeded_cursor_mode {
	kCursorAlwaysHidden = -1,
	kUnneededCursorHidden = 0,
	kUnneededCursorPoint = 1,
	kUnneededCursorShown = 2
};
extern unneeded_cursor_mode		cursorReleasedMode, cursorPressedMode;
