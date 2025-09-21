/* drv.c */
extern status_t init_hardware(void);
extern status_t init_driver(void);
extern void uninit_driver(void);
extern const char **publish_devices(void);
extern device_hooks *find_device(const char *name);
/* dev.c */
/* init.c */
extern const struct supportedcard *findsupportedcard(register struct pci_info *pci);
extern void probe_devices(void);
extern status_t firstopen(struct devinfo *di);
extern void lastclose(struct devinfo *di);
/* nv_setup.c */
extern void NV1Setup(struct devinfo *di);
extern void NV3Setup(struct devinfo *di);
extern void NV4Setup(struct devinfo *di);
extern void NV10Setup(struct devinfo *di);
extern void NV20Setup(struct devinfo *di);
/* video.c */
extern status_t vid_selectmode(register struct devinfo *di, register struct gfx_card_info *ci, register display_mode *dm);
extern status_t calcmoderegs(register struct moderegs *mr, register display_mode *dm, register struct devinfo *di);
extern status_t setmode(register struct moderegs *mr, register struct devinfo *di, register struct gfx_card_info *ci);
extern void setpaletteentry(register union vgaregs *vregs, int32 idx, uint8 r, uint8 g, uint8 b);
extern uint32 colorspacebits(uint32 cs);
extern void screen_off(register union vgaregs *vregs);
extern void screen_on(register union vgaregs *vregs);
extern status_t propose_video_mode(struct devinfo *di, display_mode *target, const display_mode *low, const display_mode *high);
extern status_t vid_init(register struct devinfo *di);
extern void vid_uninit(register struct devinfo *di);
/* cursor.c */
extern int32 setcursorshape(struct devinfo *di, uint8 *xorbits, uint8 *andbits, uint16 curswide, uint16 curshigh, uint16 hot_x, uint16 hot_y);
extern void showcursor(register struct gfx_card_info *ci, bool on);
extern void movecursor(struct devinfo *di, int32 x, int32 y);
/* overlay.c */
extern uint32 allocateoverlaybuffer(struct devinfo *di, color_space cs, uint16 width, uint16 height);
extern status_t releaseoverlaybuffer(struct devinfo *di, uint32 idx);
extern overlay_token allocateoverlay(struct gfx_card_info *ci);
extern status_t releaseoverlay(struct gfx_card_info *ci, overlay_token ot);
extern status_t configureoverlay(struct gfx_card_info *ci, overlay_token ot, const overlay_buffer *ob, const overlay_window *ow, const overlay_view *ov);
/* irq.c */
extern void clearinterrupts(register struct gfx_card_info *ci);
extern void enableinterrupts(register struct gfx_card_info *ci, int enable);
extern int32 nvidia_interrupt(void *data);
/* debugcmd.c */
extern void init_debug(void);
extern void uninit_debug(void);
/* riva_hw.c */
extern int RivaGetConfig(RIVA_HW_INST *chip);
