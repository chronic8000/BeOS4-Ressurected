t32 i=0;i<subpics;i++) {
		a_session->sread(4, &token);
		result = tokens->grab_atom(token,(SAtom**)&subpic);
		if (result >= 0) {
			p->PicLib().AddItem(subpic);
		} else {
			SPicture *fakePict = new SPicture();
			fakePict->ServerLock();
			p->PicLib().AddItem(fakePict);
		};
	};
	a_session->sread(4, &pictureSize);
	p->SetDataLength(pictureSize);
	a_session->sread(pictureSize, p->Data());
	
	if (p->Verify()) {
		token = tokens->new_token(a_session->session_team,TT_PICTURE|TT_ATOM,p);
		p->ClientLock();
		p->SetToken(token);
		a_session->swrite_l(token);
	} else {
		delete p;
		a_session->swrite_l(NO_TOKEN);
	};

	a_session->sync();
}

/*-------------------------------------------------------------*/

void	do_delete_picture(TSession *a_session)
{
	SPicture	*p;
	int32		pictureToken,result;

	a_session->sread(4, &pictureToken);
	result = tokens->grab_atom(pictureToken,(SAtom**)&p);
	if (result >= 0) {
		p->SetToken(NO_TOKEN);
		tokens->delete_atom(pictureToken,p);
	};
}

// -------------------------------------------------------------
// #pragma mark -

void	gr_get_screen_bitmap(TSession *a_session)
{
	frect		src_pos;
	frect		dst_pos;
	long		bitmap_token;
	long		result;
    long		mask_cursor;
    SAtom *		atom;
    SBitmap *	bitmap;

	a_session->sread(4, &bitmap_token);
	a_session->sread(4, &mask_cursor);
	a_session->sread(sizeof(src_pos), &src_pos);	
	a_session->sread(sizeof(dst_pos), &dst_pos);

#if ROTATE_DISPLAY
	/* feature not supported with rotated screen */
#else
	if ((src_pos.right-src_pos.left != dst_pos.right-dst_pos.left) ||
		(src_pos.bottom-src_pos.top != dst_pos.bottom-dst_pos.top)) {
		_sPrintf("Cannot scale screen image yet\n");
	} else {
		result = tokens->grab_atom(bitmap_token, &atom);
		if (result >= 0) {
			bitmap = dynamic_cast<SBitmap*>(atom);
			if (!bitmap) {
				TOff *off = dynamic_cast<TOff*>(atom);
				bitmap = off->Bitmap();
				bitmap->ServerLock();
				off->ServerUnlock();
			};
		
			RenderContext renderContext;
			RenderPort renderPort;
			grInitRenderContext(&renderContext);
			grInitRenderPort(&renderPort);

			rect r;
			r.top = r.left = 0;
			r.right = bitmap->Canvas()->pixels.w-1;
			r.bottom = bitmap->Canvas()->pixels.h-1;
			set_region(renderPort.drawingClip,&r);
			set_region(renderPort.portClip,&r);
			renderPort.portBounds = renderPort.drawingBounds = r;

			grSetContextPort_NoLock(&renderContext,&renderPort);
			bitmap->SetPort(&renderPort);

			the_server->LockWindowsForRead();
			if (mask_cursor) {
				get_screen();
				rect ir;
				ir.top = (int32)floor(src_pos.top);
				ir.left = (int32)floor(src_pos.left);
				ir.bottom = (int32)ceil(src_pos.bottom);
				ir.right = (int32)floor(src_pos.right);
				the_cursor->ClearCursorForDrawing(ir);
			};
			BGraphicDevice *device = BGraphicDevice::GameDevice(0);
			if (device == NULL)
				device = BGraphicDevice::Device(0);
			grDrawPixels(&renderContext,src_pos,dst_pos,&device->Canvas()->pixels);
			if (mask_cursor) {
				release_screen();
				the_cursor->AssertVisibleIfShown();
			};
			the_server->UnlockWindowsForRead();

			grDestroyRenderContext(&renderContext);
			grDestroyRenderPort(&renderPort);
			bitmap->ServerUnlock();
		} else {
			_sPrintf("Bad token! (get_screen_bitmap)\n");
		};
	};
#endif	
	
fast_out:
	a_session->swrite_l(0);	/* we need sync */
	a_session->sync();
}

/*-------------------------------------------------------------*/

void	gr_get_sync_info(TSession *a_session)
{
	void    *buf;
	long    token;
	long    size, error=-1;
	
	a_session->sread(4, &token);

	a_session->swrite_l(0);
	a_session->swrite_l(error);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void gr_get_clone_info(TSession *a_session)
{
	void    *buf;
	long    index;
	long    size, error=-1;
	
	a_session->sread(4, &index);

	a_session->swrite_l(0);
	a_session->swrite_l(error);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void gr_lock_direct_screen(TSession *a_session)
{
	long    index;
	long	window_token;
	long	err, state;
	
	a_session->sread(4, &index);
	a_session->sread(4, &state);	
	a_session->sread(4, &window_token);
	
	err = lock_direct_screen(index, state);

	a_session->swrite_l(err);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void gr_get_driver_info(TSession *a_session)
{
	long				result=-1;
	uint32				dd_token;
	direct_driver_info1	info;   
	
	a_session->sread(4, &dd_token);

	/* XXXXXXXXXXXXX Game Kit broken XXXXXXXXXXXXXXXXX */
	
//	result = get_driver_info(dd_token, (void*)&info);
	
//	a_session->swrite_l(sizeof(direct_driver_info1));
//	a_session->swrite(sizeof(direct_driver_info1),&info);
	a_session->swrite_l(result);
	a_session->sync();
}

/*-------------------------------------------------------------*/

void gr_control_graphics_card(TSession *a_session)
{
	void    *buf;
	long    index,cmd;
	long    size,error=-1;
	
	a_session->sread(4, &index);
	a_session->sread(4, &cmd);
	BGraphicDevice *device = BGraphicDevice::GameDevice(index);
	error = B_ERROR;
	if (device) {
		switch (cmd) {
			case B_MOVE_DISPLAY: {
				uint16 x,y;
				a_session->sread(sizeof(uint16), &x);
				a_session->sread(sizeof(uint16), &y);
				// do the move
				error = device->MoveDisplay(x,y);
				a_session->swrite_l(error);
			} break;
			case B_PROPOSE_DISPLAY_MODE: {
				display_mode target, low, high;
				a_session->sread(sizeof(display_mode), &target);
				a_session->sread(sizeof(display_mode), &low);
				a_session->sread(sizeof(display_mode), &high);
				error = device->ProposeDisplayMode(&target, &low, &high);
				a_session->swrite_l(error);
				a_session->swrite(sizeof(display_mode), &target);
			} break;
			default:
				a_session->swrite_l(error);
				break;
		}
	}
	a_session->sync();
}

/*-------------------------------------------------------------*/

void gr_set_color_map(TSession *a_session)
{
	uint8	*buf;
	long	index, from, to, i,j;
	long	size;
	rgb_color tmp;

	a_session->sread(4, &index);
	a_session->sread(4, &from);
	a_session->sread(4, &to);
	if ((from >= 0) && (to <= 255) && (from <= to)) {
//		xprintf("Set color map called [%d-%d]!!\n", from, to);
		size = (to+1-from)*3;
		buf = (uint8*)grMalloc(size,"cmap_buf",MALLOC_LOW);
		j = 0;
		for (i=from; i<=to; i++) {
			a_session->sread(sizeof(rgb_color), &tmp);
			if (buf) {
				buf[j++] = tmp.red;
				buf[j++] = tmp.green;
				buf[j++] = tmp.blue;
			};
		}
		if (!buf) return;
		BGraphicDevice *device = BGraphicDevice::GameDevice(index);
		if (device) {
			/* set the colors */
			device->SetIndexedColors(to+1-from, from, buf, 0);
		}
		grFree(buf);
	}
}

/*-------------------------------------------------------------*/

void gr_set_standard_space(TSession *a_session)
{
	display_mode dmode;
	long    index, error, save;
	
	a_session->sread(4, &index);
	a_session->sread(sizeof(display_mode), &dmode);
	
	BGraphicDevice *device = BGraphicDevice::GameDevice(index);
	error = B_ERROR;
	if (device) {
//		get_screen();
		DebugPrintf(("gr_set_stand