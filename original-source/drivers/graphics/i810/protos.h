/* drv.c */
extern status_t init_hardware(void);
extern status_t init_driver(void);
extern void uninit_driver(void);
extern const char **publish_devices(void);
extern device_hooks *find_device(const char *name);
/* dev.c */
/* init.c */
extern void probe_devices(void);
extern status_t firstopen(struct devinfo *di);
extern void lastclose(struct devinfo *di);
/* video.c */
extern int isowner(struct openerinfo *oi);
extern status_t setopenertomode(register struct openerinfo *oi, register struct gfx_card_info *ci, register display_mode *dm);
extern status_t setopenertoGLmode(struct openerinfo *oi, struct gfx_card_info *ci, struct i810drv_set_gl_disp *sgd);
extern status_t movedisplay(struct openerinfo *oi, uint16 x, uint16 y);
extern void setopenerfbbaseregs(struct openerinfo *oi);
extern status_t calcdispconfig(struct dispconfig *cl_dc, register const display_mode *dm, register struct devinfo *di);
extern void writemode(const struct dispconfig *dc, register struct devinfo *di, register struct gfx_card_info *ci);
extern void calcfbbaseregs(struct dispconfig *dc);
extern void writefbbase(register const struct dispconfig *dc, register struct gfx_card_info *ci);
extern void writepaletterange(register union vgaregs *vregs, register const uint8 *colors, int first, int last);
extern void writepaletteentry(register union vgaregs *vregs, int32 idx, uint8 r, uint8 g, uint8 b);
extern void screen_off(register union vgaregs *vregs);
extern void screen_on(register union vgaregs *vregs);
extern status_t propose_video_mode(struct devinfo *di, display_mode *target, const display_mode *low, const display_mode *high);
extern status_t vid_init(register struct devinfo *di);
extern void vid_uninit(register struct devinfo *di);
/* cursor.c */
extern int32 setcursorshape(struct devinfo *di, uint8 *xorbits, uint8 *andbits, uint16 curswide, uint16 curshigh, uint16 hot_x, uint16 hot_y);
extern void movecursor(struct devinfo *di, int32 x, int32 y);
extern void calcshowcursor(struct dispconfig *dc, struct devinfo *di, int write);
/* irq.c */
extern void clearinterrupts(register struct gfx_card_info *ci);
extern void enableinterrupts(register struct gfx_card_info *ci, int enable);
extern int32 i810_interrupt(void *data);
/* gfxbuf.c */
extern status_t allocbymemspec(struct openerinfo *oi, struct BMemSpec *ms);
extern status_t freebymemspec(struct openerinfo *oi, struct BMemSpec *ms);
extern status_t formatmemspec(struct openerinfo *oi, register struct GfxBuf *gb);
extern status_t allocgfxbuf(struct openerinfo *oi, register struct GfxBuf *gb);
extern status_t freegfxbuf(struct openerinfo *oi, register struct GfxBuf *gb);
extern status_t control_gfxbuf(openerinfo *oi, uint32 msg, void *buf, size_t len);
extern uint32 colorspacebits(uint32 cs);
extern int highbit(uint32 val);
/* debugcmd.c */
extern void init_debug(void);
extern void uninit_debug(void);
/* overlay.c */
extern uint32 allocateoverlaybuffer(struct openerinfo *oi, color_space cs, uint16 width, uint16 height);
extern status_t releaseoverlaybuffer(struct openerinfo *oi, uint32 idx);
extern overlay_token allocateoverlay(struct gfx_card_info *ci);
extern status_t releaseoverlay(struct gfx_card_info *ci, overlay_token ot);
extern status_t configureoverlay(struct gfx_card_info *ci, overlay_token ot, const overlay_buffer *ob, const overlay_window *ow, const overlay_view *ov);
extern void initOverlay(struct devinfo *di);
