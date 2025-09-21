/*  cursor.c  */
extern int32
setcursorshape (
register struct thdfx_card_info	*ci,
uint8				*xorbits,
uint8				*andbits,
uint16				curswide,
uint16				curshigh,
uint16				hot_x,
uint16				hot_y
);

extern void
movecursor (register struct thdfx_card_info *ci, int32 x, int32 y);
extern void
showcursor (register struct thdfx_card_info *ci, bool on);
