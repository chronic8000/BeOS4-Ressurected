void il_dump(void *_il, uchar *buf, uint len)
{
	il_info_t *il = (il_info_t*)_il;
	
	buf[0]='\0';
	
	/* big enough? */
	if (len < 4096) return;
	
	sprintf(
		buf, kDevName "/%d - %s %s version %s: cookie=%p reg_area=%ld\n",
		il->ilc.unit, __DATE__, __TIME__, EPI_VERSION_STR, il, il->reg_area
	);

#ifdef BCMDBG
	ilc_dump(&il->ilc, &buf[strlen(buf)], len - strlen(buf));
#endif
}

void il_assert(char *exp, char *file, uint line)
{
	char tempbuf[255];

	sprintf(tempbuf, "assertion \"%s\" failed: file \"%s\", line %d\n", exp, file, line);
	panic(tempbuf);
}

void* il_malloc(uint size) {return malloc(size);}
void  il_mfree(void *addr, uint size) {free(addr);}

static status_t il_acquire(il_info_t *il){ return lock_ben(&il->lock); }
static status_t il_release(il_info_t *il){ return unlock_ben(&il->lock); }
