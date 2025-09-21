/* ++++++++++
	FILE:	loadelf.c
	REVS:	$Revision: 1.4 $
	NAME:	herold
	DATE:	Sun May 21 19:59:21 PDT 1995
	Copyright (c) 1995 by Be Incorporated.  All Rights Reserved.
+++++ */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <elf.h>
#include <image.h>

#include "cwindow.h"
#include "symtable.h"
#include "path.h"

#define PRINT(x) /* _sPrintf x */


/* ----------
	load_elf_symbols loads in the text and data symbols from a symbol table's
	underlying ELF file.
----- */

int
load_symbols (image_info *info, symtable *t)
{
	int			fd;
	Elf32_Shdr	*sects = NULL;
	Elf32_Shdr	*strsect = NULL;
	Elf32_Shdr	*symsect = NULL;
	char		*elfnames = NULL;
	Elf32_Sym	*elfsyms = NULL;
	Elf32_Sym	*s;
	int			n_elfsyms;
	Elf32_Ehdr	h;
	Elf32_Phdr	pheaders[3];
	int			i, j;
	int			type;
	int			retval = B_ERROR;
	uint32		delta;
#if _SUPPORTS_COMPRESSED_ELF
	int			err;
	bool		compressed;
	void		*cookie;
#endif

	PRINT(("!!! load_elf_symbols\n"));

	fd = -1;
#if _SUPPORTS_COMPRESSED_ELF
	compressed = TRUE;
	cookie = NULL;
#endif

	fd = open (info->name, O_RDONLY);
	if (fd < 0)
		goto file_err_exit;
	
	/* ---
	   read in ELF header, ensure it is an ELF file
	--- */

#if _SUPPORTS_COMPRESSED_ELF
	err = open_celf_file(fd, &cookie, NULL, 0);
	if (err)
		compressed = FALSE;
#endif


#if _SUPPORTS_COMPRESSED_ELF
	if (compressed) {
		err = get_celf_elf_header(cookie, &h);
		if (err)
			goto file_err_exit;
	} else {
		lseek(fd, 0, SEEK_SET);
		if ( read (fd, &h, sizeof(h)) != sizeof (h) )
			goto file_err_exit;
	}
#else
	lseek(fd, 0, SEEK_SET);
	if ( read (fd, &h, sizeof(h)) != sizeof (h) )
		goto file_err_exit;
#endif

	/* check magic number */
	if (   h.e_ident[EI_MAG0] != ELFMAG0
		|| h.e_ident[EI_MAG1] != ELFMAG1
		|| h.e_ident[EI_MAG2] != ELFMAG2
		|| h.e_ident[EI_MAG3] != ELFMAG3)
	{
		PRINT(("load_elf_symbols: bad magic, got 0x%2x%2x%2x%2x\n",
			h.e_ident[EI_MAG0], h.e_ident[EI_MAG1],
			h.e_ident[EI_MAG2], h.e_ident[EI_MAG3]));
		goto exit;
	}


	/* check file class */
	if (h.e_ident[EI_CLASS] != ELFCLASS32) {
		PRINT(("load_elf_symbols: bad class, got 0x%x\n", h.e_ident[EI_CLASS]));
		goto exit;
	}

	/* check data encoding */
	if (h.e_ident[EI_DATA] != ELFDATA2LSB) {
		PRINT(("load_elf_symbols: bad data, got 0x%x\n", h.e_ident[EI_DATA]));
		goto exit;
	}

	/* check format version */
	if (h.e_ident[EI_VERSION] != EV_CURRENT) {
		PRINT(("load_elf_symbols: bad version, got 0x%x\n", h.e_ident[EI_VERSION]));
		goto exit;
	}

	/* ---
	   read in program header array
	--- */

	if (h.e_phnum > 3) {
		PRINT(("load_elf_symbols: too many program headers!"));
		goto exit;
	}
	i = h.e_phnum * h.e_phentsize;

#if _SUPPORTS_COMPRESSED_ELF
	if (compressed) {
		err = get_celf_program_headers(cookie, &pheaders[0]);
		if (err)
			goto file_err_exit;
	} else {
		lseek (fd, h.e_phoff, SEEK_SET);
		if (read(fd, &pheaders[0], i) != i)
			goto file_err_exit;
	}
#else
	lseek (fd, h.e_phoff, SEEK_SET);
	if (read(fd, &pheaders[0], i) != i)
		goto file_err_exit;
#endif

	delta = (uint32) info->text - (uint32) pheaders[0].p_vaddr;

	/* ---
	   read in section header array
	--- */

	i = h.e_shnum * h.e_shentsize;
	if (!(sects = (Elf32_Shdr *) malloc (i)))
		goto mem_err_exit;
	

#if _SUPPORTS_COMPRESSED_ELF
	if (compressed) {
		err = get_celf_section_headers(cookie, sects);
		if (err)
			goto file_err_exit;
	} else {
		lseek (fd, h.e_shoff, SEEK_SET);
		if (read(fd, sects, i) != i)
			goto file_err_exit;
	}
#else
	lseek (fd, h.e_shoff, SEEK_SET);
	if (read(fd, sects, i) != i)
		goto file_err_exit;
#endif

	/* ---
	   locate elf symbol table section, read it in
	--- */

	for (i = 0, symsect = NULL; i < h.e_shnum && !symsect; i++)
		if (sects[i].sh_type == SHT_SYMTAB)
			symsect = sects + i;

	if (!symsect) {
		PRINT(("load_elf_symbols: no symtable section!\n"));
		goto exit;
	}

	n_elfsyms = symsect->sh_size / sizeof (Elf32_Sym);

	PRINT(("load_elf_symbols: n_elfsyms = %d\n", n_elfsyms));

	if (!(elfsyms = (Elf32_Sym *) malloc (symsect->sh_size)))
		goto mem_err_exit;
	
#if _SUPPORTS_COMPRESSED_ELF
	if (compressed) {
		err = get_celf_section(cookie, symsect - sects, elfsyms, symsect->sh_size);
		if (err)
			goto file_err_exit;
	} else {
		lseek(fd, symsect->sh_offset, SEEK_SET);
		if (read (fd, elfsyms, symsect->sh_size) != symsect->sh_size )
			goto file_err_exit;
	}
#else
	lseek(fd, symsect->sh_offset, SEEK_SET);
	if (read (fd, elfsyms, symsect->sh_size) != symsect->sh_size )
		goto file_err_exit;
#endif

	/* ---
	   locate elf string table section, read it in
	--- */

	strsect = sects + symsect->sh_link;	/* sym sect has link to string table */

	if (!(elfnames = (char *) malloc (strsect->sh_size)))
		goto mem_err_exit;
	
#if _SUPPORTS_COMPRESSED_ELF
	if (compressed) {
		err = get_celf_section(cookie, strsect - sects, elfnames, strsect->sh_size);
		if (err)
			goto file_err_exit;
	} else {
		lseek(fd, strsect->sh_offset, SEEK_SET);
		if (read (fd, elfnames, strsect->sh_size) != strsect->sh_size)
			goto file_err_exit;
	}
#else
	lseek(fd, strsect->sh_offset, SEEK_SET);
	if (read (fd, elfnames, strsect->sh_size) != strsect->sh_size)
		goto file_err_exit;
#endif

	/* ---
	   extract the 'useful' symbols, put into new symbol table
	--- */

	for (i = 0; i < n_elfsyms; i++) {
		s = elfsyms + i;
		type = ELF32_ST_TYPE (s->st_info);
		if ((type == STT_OBJECT || type == STT_FUNC || type == STT_NOTYPE)) {
			j = ELF32_ST_BIND (s->st_info);
			if ((j == STB_LOCAL || 
			     j == STB_WEAK ||
			     j == STB_GLOBAL))
			{
				if (type == STT_OBJECT)
					type = symDataLocation;
				else if (type == STT_FUNC)
					type = symTextLocation;
				else if (!strcmp (elfnames + s->st_name, "_end"))
					type = symDataLocation;
				else if ((type == STT_NOTYPE) && strcmp(elfnames + s->st_name, "gcc2_compiled.")) /* get around bug in nasm */
					type = symDataLocation; 
				else	
					type = symNotFound;
				if (type != symNotFound)
					add_symbol (t, elfnames + s->st_name, (int)s->st_value + delta, type);
			} else {
				PRINT(("load_elf_symbols: sym #%d has weird binding 0x%x\n", i, j));
			}
		}
	}

	/* ---
	   set up all the various indexes
	--- */

	if (!t->n_syms) {
		PRINT(("load_elf_symbols: nothing in symtable!\n"));
		goto exit;
	}

	PRINT(("symtable has %d syms\n", t->n_syms));

	t->loaded = 1;

	retval = B_NO_ERROR;
	goto exit;

mem_err_exit:
	PRINT (("load_elf_symbols: out of memory\n"));
	goto exit;

file_err_exit:
	PRINT (("load_elf_symbols: error accessing file\n"));
	
exit:

#if _SUPPORTS_COMPRESSED_ELF
	if (cookie)
		close_celf_file(cookie);
#endif
	if (fd >= 0) close (fd);
	if (sects) free(sects);
	if (elfsyms) free(elfsyms);
	if (elfnames) free(elfnames);
	shrink_symtable (t);
	PRINT (("load_elf_symbols: retval=%d\n", retval));
	return retval;
}

// stub needed by libuncrush

#if _SUPPORTS_COMPRESSED_ELF
#undef dprintf

void
dprintf(void)
{
}
#endif
