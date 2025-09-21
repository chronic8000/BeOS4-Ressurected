/*  video.c  */
extern status_t
trident_init (register struct devinfo *di);
status_t
propose_video_mode (
struct devinfo		*di,
display_mode		*target,
const display_mode	*low,
const display_mode	*high
);
extern status_t
SetupCRTC (
register struct devinfo		*di,
register struct tri_card_info	*ci,
register display_mode		*dm
);
extern uint32
colorspacebits (uint32 cs);
extern void
setpaletteentry (
register union vgaregs	*vregs,
int32			idx,
uint8			r,
uint8			g,
uint8			b
);
extern void screen_off (register union vgaregs *vregs);
extern void screen_on (register union vgaregs *vregs);

/*  cursor.c  */
extern int32
setcursorshape (
struct devinfo	*di,
uint8		*xorbits,
uint8		*andbits,
uint16		curswide,
uint16		curshigh,
uint16		hot_x,
uint16		hot_y
);
extern void movecursor (struct devinfo *di, int32 x, int32 y);
extern void
showcursor (register struct tri_card_info *ci, bool on);
extern void setcursorcolors (struct devinfo *di);
