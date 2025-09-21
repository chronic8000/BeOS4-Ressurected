
#ifdef	DEBUG_LOG

typedef	struct {
	char		short_desc[16];
	long		pid;
	bigtime_t	when;
	long		category;
	long		opt1;
	long		opt2;
	char		comment[128];
} log_entry;	


//------------------------------------------------------------
// Bunch of renders used by the debugger
//------------------------------------------------------------


extern "C" void rect_fill_copy_24_np(	port		*a_port,
			 							rect		*r,
			 							rgb_color	color);
extern "C" void rect_fill_copy_8_np(	port		*a_port,
			 							rect		*r,
			 							rgb_color	color);


extern font_desc Kate;
#define SetPixel32(p,c)  (*(volatile uint32*)(p) = (c))


copy_block_1to81(long font_rowbyte, uchar *font_base, rect src, rect dst,
				 uchar fore_color, uchar back_color)
{
	uchar	*dst_base;
	uchar	*src_base;
	uchar	*tmp_base;
	uchar	*dst_base1;
	uchar	*dst_base2;
	short	y;
	short	dx;
	short	dx1;
	uchar	a_bw_byte;
	uchar	mask;
	uchar	mask0;
	int 	psiz;


	psiz = screenport.bit_per_pixel / 8;

	src_base = font_base + (src.top * font_rowbyte);
	dst_base2 = (uchar*)screenport.base +
	  (screenport.rowbyte * dst.top) + (psiz * dst.left);
	dx1 = dst.right - dst.left;

	dx = src.left;
	mask0 = 0x80 >> (dx & 0x07);
	src_base += dx >> 3;	

	for (y = dst.top; y <= dst.bottom; y++) {
		dst_base = dst_base2;
		dst_base2 += screenport.rowbyte;

		tmp_base = src_base;
		a_bw_byte = *tmp_base++;

		mask = mask0;
		
		dst_base1 = dst_base + psiz*(dx1 + 1);

		if(screenport.bit_per_pixel == 32 ) {		   
			uint32 fc = 0xffffffff;
			uint32 bc = 0x00000000;

			while (dst_base1 != dst_base) {
				if (a_bw_byte & mask) {
					SetPixel32( dst_base, fc );
					dst_base += sizeof(uint32);
				} else {	
					SetPixel32( dst_base, bc );
					dst_base += sizeof(uint32);
				}

				mask >>= 1;
				if (mask == 0) {
					mask = 0x80;
					a_bw_byte = *tmp_base++;
				}
			}
		}
		else
		{
			while (dst_base1 != dst_base) {
				if (a_bw_byte & mask)
					*dst_base++ = fore_color;
				else	
					*dst_base++ = back_color;

				mask >>= 1;
				if (mask == 0) {
					mask = 0x80;
					a_bw_byte = *tmp_base++;
				}
			}
		}

		src_base += font_rowbyte;
	}
}

//------------------------------------------------------------

long text	( long h,
	       	   long v,
	       	   char *str,
	       	   font_desc *desc, uchar fore_color, uchar back_color)
{
	rect		src_box;
	rect		dst_box;
	long		char_offset;
	long		char_width;
	long		dh;
	long		c, c1;
	long		w;
	long		h1;
	long		tw = 0;
	uchar		char_color;


	src_box.top = 0;
	src_box.bottom = desc->font_height - 1;
	
	dst_box.top = v;
	dst_box.bottom = v + desc->font_height - 1;
	
	
	while(c = *str++) {
		
		h1 = h;
		if (c > desc->lastchar) {
			w = desc->default_width;
			goto leave;
		}

		c1 = c - (desc->firstchar - 1);
		
		if (c1 < 0) {
			w = desc->default_width;
			goto leave;
		}

		char_offset = *(desc->offsets + c1);
		char_width  = *(desc->offsets + c1 + 1) - char_offset;

		if (char_width < 1) {
			w = desc->default_width;
			goto leave;
		}
		w = char_width;

		src_box.left = char_offset;
		src_box.right = char_offset + char_width;

		dst_box.left = h1;
		dst_box.right = h1 + char_width - 1;

		if (!desc->proportional) {
			dh = (desc->width_max - char_width) >> 1;
			h1 += dh;
			dst_box.left += dh;
			dst_box.right += dh;
		}

		char_color = fore_color;

		copy_block_1to81	(desc->bm_rowbyte,
							 desc->bitmap,
							 src_box,
							 dst_box,
							 char_color, back_color);
	leave:
		if (!desc->proportional)
			w = desc->width_max;
		w++;
		tw += w;
		h += w;
	}
	return(tw);
}

//------------------------------------------------------------

rect	nr(long top, long left, long bottom, long right)
{
	rect	r;

	r.top = top;
	r.left = left;
	r.bottom = bottom;
	r.right = right;

	return r;
}

//------------------------------------------------------------

rgb_color	nc(long r, long g, long b)
{
	rgb_color	c;

	c.red = r;
	c.green = g;
	c.blue = b;

	return c;
}

//------------------------------------------------------------

void	fr(rect r, rgb_color c)
{
	if (screenport.bit_per_pixel == 8)
		rect_fill_copy_8_np(&screenport, &r, c);
	else
		rect_fill_copy_24_np(&screenport, &r, c);
}

//------------------------------------------------------------

#define	LOG_SIZE	4096					//about 400 KB	
#define	BACK_BUFFER	2048

//------------------------------------------------------------

long		log_count;
long		log_pos;
long		log_sem;
long		vp;
long		hp;
long		back_buf_p;
long		back_buf_cnt;
long		back;
long		filter = 0;
log_entry	the_log[LOG_SIZE];
char		back_buffer[BACK_BUFFER][100];

//------------------------------------------------------------

void	LOCK_LOG()
{
	acquire_sem(log_sem);
}

//------------------------------------------------------------

void	UNLOCK_LOG()
{
	release_sem(log_sem);
}

//------------------------------------------------------------

void	SET_BASE_NAME(char *short_name, long cat, long parm1, long parm2)
{

	if (filter) {
		if (filter != getpid())
			return;
	}
	the_log[log_pos].when = system_time();
	the_log[log_pos].pid = getpid();
	the_log[log_pos].category = cat;
	the_log[log_pos].opt1 = parm1;
	the_log[log_pos].opt2 = parm2;
	strncpy(the_log[log_pos].short_desc, short_name, 16);
	the_log[log_pos].short_desc[15] = 0;
	the_log[log_pos].comment[0] = 0;
	log_count++;
	log_count %= (LOG_SIZE);
}

//------------------------------------------------------------

void	SET_FORMAT(char *format, ...)
{
	va_list args;
	char buf[255];

	if (filter) {
		if (filter != getpid())
			return;
	}

	va_start(args,format);
	vsprintf (buf, format, args);
	va_end(args);
	strncpy(the_log[log_pos].comment, buf, 128);
	the_log[log_pos].short_desc[127] = 0;
	log_pos++;
	log_pos = log_pos % (LOG_SIZE);
}

//------------------------------------------------------------

void	init_log()
{
	vp = 0;
	hp = 0;
	log_count = 0;
	log_pos = 0;
	back = 20;
	back_buf_cnt = 0;
	back_buf_p = 0;
	log_sem = create_sem(1, "log_sem");
}

//------------------------------------------------------------
extern	"C" void debug_key(char key);
//------------------------------------------------------------


void	scroll_up(long dv)
{
	uchar	*base;
	long	rowbyte;
	ulong	*src;
	ulong	*dst;
	long	i;
	long	sy,dy;
	long	bpp;
	long	x;

	base = (uchar *)screenport.base;
	rowbyte = screenport.rowbyte;
	bpp = screenport.bit_per_pixel/8;

	for (sy = dv; sy < 449; sy++) {
		dy = sy - dv;
		src = (ulong*)(base + rowbyte * sy);
		dst = (ulong*)(base + rowbyte * dy);
		for (x = 0; x < (640*bpp)/16; x++) {
			*dst++ = *src++;
			*dst++ = *src++;
			*dst++ = *src++;
			*dst++ = *src++;
		}
	}
}

//------------------------------------------------------------

void	scroll_down(long dv)
{
	uchar	*base;
	long	rowbyte;
	ulong	*src;
	ulong	*dst;
	long	i;
	long	sy,dy;
	long	bpp;
	long	x;

	base = (uchar *)screenport.base;
	rowbyte = screenport.rowbyte;
	bpp = screenport.bit_per_pixel/8;

	for (sy = (449-dv); sy > 0; sy--) {
		dy = sy + dv;
		src = (ulong*)(base + rowbyte * sy);
		dst = (ulong*)(base + rowbyte * dy);
		for (x = 0; x < (640*bpp)/16; x++) {
			*dst++ = *src++;
			*dst++ = *src++;
			*dst++ = *src++;
			*dst++ = *src++;
		}
	}
}

//------------------------------------------------------------

void	line_up()
{
	long	line;
	long	i;

	back+=10;

	fr(nr(0,0,14*31, 639), nc(0,0,0));
	if (back > back_buf_cnt) {
		back = back_buf_cnt;
	}

	line = back_buf_p - back;
	line += BACK_BUFFER;
	line %= BACK_BUFFER;

	for (i = 0; i < 30; i++) {
   		back_buffer[line][99] = 0;
		text(10 + hp*8,
   		   	 i*14,
   		   	 &back_buffer[line][0],
   		 	 &Kate,255,0);
		line++;
		line %= BACK_BUFFER;
	}
}

//------------------------------------------------------------

void	line_down()
{
	long	line;
	long	i;

	back-=10;

	fr(nr(0,0,14*31, 639), nc(0,0,0));
	if (back < 20) {
		back = 20;
	}

	line = back_buf_p - back;
	line += BACK_BUFFER;
	line %= BACK_BUFFER;

	for (i = 0; i < 30; i++) {
   		back_buffer[line][99] = 0;
		text(10 + hp*8,
   		   	 i*14,
   		   	 &back_buffer[line][0],
   		 	 &Kate,255,0);
		line++;
		line %= BACK_BUFFER;
	}
}

//------------------------------------------------------------

void	next_vp()
{
	long	i;

	vp++;
	back_buf_p++;
	back_buf_cnt++;
	if (back_buf_cnt > BACK_BUFFER)
		back_buf_cnt = BACK_BUFFER;

	back_buf_p %= BACK_BUFFER;
	
	for (i = 0; i < 100; i++)
		back_buffer[back_buf_p][i] = ' ';

	if (vp == 30) {
		vp = 20;
		scroll_up(140);
		fr(nr(449-140,0,449,639), nc(0,0,0));
	}
}

//------------------------------------------------------------

void	output(char *string)
{
	char	c[2];

	c[1] = 0;

	while(c[0] = *string++) {
		if (c[0] == '\n') {
			hp = 0;
			next_vp();
		}
		else {
			back_buffer[back_buf_p][hp] = c[0];
			text(10 + hp*7,
	    	   	 vp*14,
	    	   	 c,
	    	 	 &Kate,255,0);
			hp++;
			if (hp == 90) {
				hp =  0;
				next_vp();
			}
		}
	}
}

//------------------------------------------------------------

void lprintf(char *format, ...)
{
	va_list args;
	char buf[255];

	va_start(args,format);
	vsprintf (buf, format, args);
	va_end(args);
	output(buf);
}

//------------------------------------------------------------
char			pad[4];
char			input_buffer[256];
long			ipos;
long			ilen;
char			new_key;
extern	bool	in_debug;
//------------------------------------------------------------

void	draw_input_line()
{
	char		tmp[256];
	rgb_color	c;

	memcpy(tmp, input_buffer, 256);

	fr(nr(451,0,479,639), nc(0,0,0));
	tmp[ilen] = 0;
	text(10,
	   	 460,
	   	 tmp,
	 	 &Kate,255,0);
	fr(nr(474,10+(ipos*7),475,10+(ipos*7)+8), nc(0,0,255));
	fr(nr(457,10+(ipos*7),458,10+(ipos*7)+8), nc(0,0,255));
}

//------------------------------------------------------------

void	init_debug_gr()
{
	ipos = 0;
	ilen = 0;
	hp = 0;
	vp = 0;
	back_buf_p = 0;
	fr(nr(0,0,479,639), nc(0,0,0));
	fr(nr(450,0,450,639), nc(255,0,0));
	lprintf("Welcome to the app_server debugger\n");
}

//------------------------------------------------------------


void	debug_key(char key)
{
	new_key = key;
}

//------------------------------------------------------------

//------------------------------------------------------------

void	do_help(char *p)
{
	output("HELP                        -- this help\n");
	output("LIST (pid) (count)          -- List the operations done by <pid>\n");
	output("LIST_GR (pid) (count)       -- same as list, but no sem or port log\n");
	output("LIST_COMMAND (pid) (count)  -- list client commands\n");
	output("LIST_TIME (pid) (count)     -- list timing info\n");
	output("FILTER (pid)                -- Disable recording for all but pid\n");
	output("                            -- Use Filter 0 to reset\n");
	output("                            -- This command will clear old log info\n");
	output("SEMS                        -- Identify the pid(s) with screen/region sems\n");
	output("LT                          -- gives the list of thread active in the log\n");
	output("LISTWINDOWS                 -- list of windows + window infos\n");
	output("CONT                        -- exit the debugger\n");
	output("ES                          -- exit the debugger\n");
}

//------------------------------------------------------------

char	*map_cat(long v)
{
	switch(v) {
		case LOG_GENERAL:
			return("LOG_GENERAL");
			break;
		case LOG_SEM:
			return("LOG_SEM");
			break;
		case LOG_WINDOW:
			return("LOG_WINDOW");
			break;
		case LOG_MESSAGE:
			return("LOG_MESSAGE");
			break;
		case LOG_PORTS:
			return("LOG_PORTS");
			break;
		case LOG_DRAW:
			return("LOG_DRAW");
			break;
		case LOG_COMMAND:
			return("LOG_COMMAND");
			break;
		case LOG_TIME:
			return("LOG_TIME");
			break;
	}
	return("LOG_????");
}

//------------------------------------------------------------

void	display_entry(long n)
{
	char	buf[500];
	float	t;

	t = the_log[n].when / 1000.0;
	sprintf(buf, "%4ld %4.3f %16s %12s %8lx %8lx %s\n",
			the_log[n].pid,
			t,
			the_log[n].short_desc, 
			map_cat(the_log[n].category),
			the_log[n].opt1,
			the_log[n].opt2,
			the_log[n].comment);
	output(buf);
}

//------------------------------------------------------------

void	set_filter(char *cmd)
{
	long	i;
	long	v1;
	
	i = 0;
	while(cmd[i] && (cmd[i] != ' ')) 
		i++;
	cmd[i] = 0;
	if (strlen(cmd) == 0) {
		v1 = 0;
	}
	else
		v1 = atoi(cmd);

	
}

//------------------------------------------------------------

void	do_list_gr(char *cmd)
{
	long	first,last;
	long	i;
	long	v1,v2;
	long	cnt;
	char	*cmd1;
	long	type;

	i = 0;
	while(cmd[i] && (cmd[i] != ' ')) 
		i++;
	cmd[i] = 0;
	if (strlen(cmd) == 0) {
		v1 = 0;
	}
	else
		v1 = atoi(cmd);

	cmd1 = &(cmd[i+1]);

	i = 0;
	cmd = cmd1;
	while(cmd[i] && (cmd[i] != ' ')) 
		i++;
	
	cmd[i] = 0;
	if (strlen(cmd) == 0) {
		v2 = 0;
	}
	else
		v2 = atoi(cmd);

	cmd1 = &(cmd[i+1]);
	
	if (log_count < LOG_SIZE) {
		first = 0;
	}
	else {
		first = log_pos+1;
	}
	last = log_pos;

	if (v2 == 0)
		cnt = 10000;
	else
		cnt = v2;

	for (i = 0; i < log_count; i++) {
		type = the_log[(i+first)%LOG_SIZE].category;
		if ((type != LOG_SEM) && (type != LOG_PORTS)) {
			if ((v1 == 0) || (v1 == the_log[(i+first) % LOG_SIZE].pid)) {
				display_entry((i+first) % LOG_SIZE);
				cnt--;
				if (cnt <= 0)
					goto out;
			}
		}
	}
out:;
}

//------------------------------------------------------------

void	do_list_command(char *cmd)
{
	long	first,last;
	long	i;
	long	v1,v2;
	long	cnt;
	char	*cmd1;
	long	type;

	i = 0;
	while(cmd[i] && (cmd[i] != ' ')) 
		i++;
	cmd[i] = 0;
	if (strlen(cmd) == 0) {
		v1 = 0;
	}
	else
		v1 = atoi(cmd);

	cmd1 = &(cmd[i+1]);

	i = 0;
	cmd = cmd1;
	while(cmd[i] && (cmd[i] != ' ')) 
		i++;
	
	cmd[i] = 0;
	if (strlen(cmd) == 0) {
		v2 = 0;
	}
	else
		v2 = atoi(cmd);

	cmd1 = &(cmd[i+1]);
	
	if (log_count < LOG_SIZE) {
		first = 0;
	}
	else {
		first = log_pos+1;
	}
	last = log_pos;

	if (v2 == 0)
		cnt = 10000;
	else
		cnt = v2;

	for (i = 0; i < log_count; i++) {
		type = the_log[(i+first)%LOG_SIZE].category;
		if ((type == LOG_COMMAND)) {
			if ((v1 == 0) || (v1 == the_log[(i+first) % LOG_SIZE].pid)) {
				display_entry((i+first) % LOG_SIZE);
				cnt--;
				if (cnt <= 0)
					goto out;
			}
		}
	}
out:;
}

//------------------------------------------------------------

void	do_list_time(char *cmd)
{
	long	first,last;
	long	i;
	long	v1,v2;
	long	cnt;
	char	*cmd1;
	long	type;

	i = 0;
	while(cmd[i] && (cmd[i] != ' ')) 
		i++;
	cmd[i] = 0;
	if (strlen(cmd) == 0) {
		v1 = 0;
	}
	else
		v1 = atoi(cmd);

	cmd1 = &(cmd[i+1]);

	i = 0;
	cmd = cmd1;
	while(cmd[i] && (cmd[i] != ' ')) 
		i++;
	
	cmd[i] = 0;
	if (strlen(cmd) == 0) {
		v2 = 0;
	}
	else
		v2 = atoi(cmd);

	cmd1 = &(cmd[i+1]);
	
	if (log_count < LOG_SIZE) {
		first = 0;
	}
	else {
		first = log_pos+1;
	}
	last = log_pos;

	if (v2 == 0)
		cnt = 10000;
	else
		cnt = v2;

	for (i = 0; i < log_count; i++) {
		type = the_log[(i+first)%LOG_SIZE].category;
		if ((type == LOG_TIME)) {
			if ((v1 == 0) || (v1 == the_log[(i+first) % LOG_SIZE].pid)) {
				display_entry((i+first) % LOG_SIZE);
				cnt--;
				if (cnt <= 0)
					goto out;
			}
		}
	}
out:;
}

//------------------------------------------------------------

void	do_list(char *cmd)
{
	long	first,last;
	long	i;
	long	v1,v2;
	long	cnt;
	char	*cmd1;

	i = 0;
	while(cmd[i] && (cmd[i] != ' ')) 
		i++;
	cmd[i] = 0;
	if (strlen(cmd) == 0) {
		v1 = 0;
	}
	else
		v1 = atoi(cmd);

	cmd1 = &(cmd[i+1]);

	i = 0;
	cmd = cmd1;
	while(cmd[i] && (cmd[i] != ' ')) 
		i++;
	
	cmd[i] = 0;
	if (strlen(cmd) == 0) {
		v2 = 0;
	}
	else
		v2 = atoi(cmd);

	cmd1 = &(cmd[i+1]);
	
	if (log_count < LOG_SIZE) {
		first = 0;
	}
	else {
		first = log_pos+1;
	}
	last = log_pos;

	if (v2 == 0)
		cnt = 10000;
	else
		cnt = v2;

	for (i = 0; i < log_count; i++) {
		if ((v1 == 0) || (v1 == the_log[(i+first) % LOG_SIZE].pid)) {
			display_entry((i+first) % LOG_SIZE);
			cnt--;
			if (cnt <= 0)
				goto out;
		}
	}
out:;
}

//------------------------------------------------------------

void	do_sems()
{
	long	p;
	long	i;

	for (i = 0; i < log_count; i++) {
		p = log_pos - i;
		p = p + LOG_SIZE;
		p = p % LOG_SIZE;
		if (strcmp(the_log[p].short_desc, "release_screen") == 0) {
			lprintf("nobody has the screen sem\n");
			lprintf("it was last released by pid %ld\n", the_log[p].pid);
			goto next;
		}
		if ((strcmp(the_log[p].short_desc, "got_screen") == 0)) {
			lprintf("screen sem was taken by pid %ld\n", the_log[p].pid);
			goto next;
		}
	}
	lprintf("No screen sem operation found in log !\n");

next:;
	for (i = 0; i < log_count; i++) {
		p = log_pos - i;
		p = p + LOG_SIZE;
		p = p % LOG_SIZE;
		if ((strcmp(the_log[p].short_desc, "signal_regions") == 0)
			&& (the_log[p].opt2 <= 0)) {
			lprintf("nobody has the region sem\n");
			lprintf("it was last released by pid %ld\n", the_log[p].pid);
			goto done;
		}
		if ((strcmp(the_log[p].short_desc, "wait_regions_done") == 0)) {
			lprintf("region sem was taken by pid %ld\n", the_log[p].pid);
			goto done;
		}
	}
	lprintf("No region sem operation found in log !\n");

done:;
}

//------------------------------------------------------------

void	do_num()
{
	long	i;

	for (i = 0; i < 100; i++) {
		lprintf("%4ld\n", i); 
	}
}

//------------------------------------------------------------

char 	*pstate_names[] = {
	"",
	"run",
	"rdy",
	"msg",
	"zzz",
	"sus",
	"sem"
	};

//------------------------------------------------------------

void	do_lt()
{
	long		active[512];
	long		first,last;
	long		i;
	long		j;
	long		pid;
	long		max = 0;
	int32		tmcookie, thcookie;
	thread_info	*t;
	team_info	*tm;
	int			secs;
	char		buf[B_OS_NAME_LENGTH+16];
	ulong		pages_avail, pages_total;
	float		ratio;
	system_info	sinfo;
	sem_info 	si;
	
	if (log_count < LOG_SIZE) {
		first = 0;
	}
	else {
		first = log_pos+1;
	}
	last = log_pos;

	for (i = 0; i < 512; i++) {
		active[i] = 0;
	}
	for (i = 0; i < log_count; i++) {
		pid = the_log[(i+first) % LOG_SIZE].pid;
		for (j = 0; j < max; j++)  {
			if (active[j] == pid)
				goto out;
		}
		active[max] = pid;
		max++;
out:;				
	}			
	//for (i = 0; i < max; i++) {
		//lprintf("pid %4ld\n", active[i]); 
	//}


	t  = (thread_info *) malloc (sizeof (thread_info));
	tm = (team_info *) malloc (sizeof (team_info));

	lprintf("\n thread           name      state prio   user  kernel semaphore\n");
	lprintf(  "-----------------------------------------------------------------------\n");
	
	tmcookie = 0;
	while (get_next_team_info(&tmcookie, tm) == B_NO_ERROR) {
		thcookie = 0;
		while (get_next_thread_info(tm->team, &thcookie, t) == B_NO_ERROR) {
			for (i = 0; i < max; i++) {
				if (active[i] == t->thread)
					goto found;
			}
			goto skip;
found:;
			if (t->state == B_THREAD_WAITING) {
				get_sem_info(t->sem, &si);
				sprintf(buf, "%s(%d)", si.name, t->sem);
			} else
				buf[0] = '\0';

			lprintf("%7d %20s%5s%4d%8Ld%8Ld %s\n",
				   t->thread, t->name,
				   pstate_names[t->state], t->priority,
					t->user_time/1000,
					t->kernel_time/1000,
					buf);
			skip:;
		}
	}
	
	free (t);
	free (tm);

}

//------------------------------------------------------------

void	do_listwindows()
{
	extern 		TWindow *windowlist;
	
	TWindow	*a_window;

	a_window = windowlist;

	while(a_window) {
		lprintf("window_ptr = %8lx\n    window_name=%s\n", a_window, a_window->window_name);
		lprintf("    bound = %ld %ld %ld %ld\n",
		a_window->window_bound.left,
		a_window->window_bound.top,
		a_window->window_bound.right,
		a_window->window_bound.bottom);

		lprintf("    task_id = %4ld\n    server_token=%4lx\n    flags=%lx\n    state=%lx\n    nextwindow=%lx\n",
		a_window->task_id,
		a_window->server_token,
		a_window->flags,
		a_window->state,
		a_window->nextwindow);

		a_window = (TWindow *)a_window->nextwindow;
	}
	lprintf("------------------------------------------------------\n");
}

//------------------------------------------------------------

void	exit_debug()
{
	in_debug = 0;
}

//------------------------------------------------------------

void	handle_line(char *cmd)
{
	long	i;
	char	*cmd1;

	for (i = 0; i < strlen(cmd); i++)
		cmd[i] = toupper(cmd[i]);

	i = 0;

	while(cmd[i] && (cmd[i] != ' ')) 
		i++;
	cmd[i] = 0;
	cmd1 = &(cmd[i+1]);

	if (strcmp(cmd, "LIST")==0) {
		do_list(cmd1);
	}
	if (strcmp(cmd, "LIST_GR")==0) {
		do_list_gr(cmd1);
	}
	if (strcmp(cmd, "LIST_COMMAND")==0) {
		do_list_command(cmd1);
	}
	if (strcmp(cmd, "HELP")==0) {
		do_help(cmd1);
	}
	if (strcmp(cmd, "FILTER")==0) {
		set_filter(cmd1);
	}
	if (strcmp(cmd, "LT")==0) {
		do_lt();
	}
	if (strcmp(cmd, "NUM")==0) {
		do_num();
	}
	if (strcmp(cmd, "LISTWINDOWS")==0) {
		do_listwindows();
	}
	if (strcmp(cmd, "LIST_TIME")==0) {
		do_list_time(cmd1);
	}
	if (strcmp(cmd, "CONT")==0) {
		exit_debug();
	}
	if (strcmp(cmd, "ES")==0) {
		exit_debug();
	}
	if (strcmp(cmd, "SEMS")==0) {
		do_sems();
	}
}

//------------------------------------------------------------

void	reset_back()
{
	long	line;
	long	i;
	
	if (back != 20) {
		back = 20;
		fr(nr(0,0,14*31, 639), nc(0,0,0));

		line = back_buf_p - back;
		line += BACK_BUFFER;
		line %= BACK_BUFFER;

		for (i = 0; i < 30; i++) {
   			back_buffer[line][99] = 0;
			text(10 + hp*8,
   			   	 i*14,
   			   	 &back_buffer[line][0],
   			 	 &Kate,255,0);
			line++;
			line %= BACK_BUFFER;
		}
		vp = 29;
	}
}

//------------------------------------------------------------


long	db_task(void *p)
{
	long	i;

	acquire_sem(log_sem);
	init_debug_gr();

	while(1) {
		if (!in_debug) 
			goto out_debug;
		if (new_key) {
			
			if (new_key == 8) {	//bs	
				reset_back();
				if (ipos>0)
				for (i = ipos; i <= ilen; i++) {
					input_buffer[i-1] = input_buffer[i-0];
				}
				ipos--;
				if (ipos < 0) ipos = 0;
				ilen--;
				if (ilen < 0) ilen = 0;
			}
			else
			if (new_key == 30) {
				line_up();
			}
			else
			if (new_key == 31) {
				line_down();
			}
			else
			if (new_key == 28) {
				reset_back();
				ipos--;
				if (ipos < 0) ipos = 0;
			}
			else
			if (new_key == 29) {
				ipos++;
				if (ipos > ilen) ipos = ilen;
			}
			else {
				reset_back();
				if (new_key == '\n') {
					input_buffer[ilen] = 0;
					lprintf("> %s\n", input_buffer);
					handle_line(input_buffer);
					ipos = 0;
					ilen = 0;
					for (i = 0; i < 256; i++)
						input_buffer[i] = 0;
					draw_input_line();
					new_key = 0;
					goto out;
				}
				else {
					reset_back();
					input_buffer[ipos] = new_key;	
					ipos++;
				}
			}
			if (ipos > ilen)
				ilen = ipos;
			new_key = 0;
			draw_input_line();
		}
		out:;
		snooze(50000);
	}
out_debug:;
	release_sem(log_sem);
}

//------------------------------------------------------------
extern "C" void start_debugger();

void	start_debugger()
{
	xspawn_thread(db_task, "db_task", B_DISPLAY_PRIORITY, 0);
}

//------------------------------------------------------------

#endif
