#include "cifs_globals.h"
#include "netio.h"
#include "packet.h"


#define DBG_RENAME 2

status_t
send_get_attributes(nspace *vol, vnode *dir, const char *name)
{
	status_t err;
	uchar wordcount;
	ushort bytecount;
	packet *pkt = NULL;
	char *fullname=NULL;

	
	err = get_full_name(vol, dir, name, &fullname);
	if (err != B_OK) return err;
	
	err = new_smb_packet(vol, & pkt, SMB_COM_QUERY_INFORMATION);
	if (err != B_OK) goto exit1;
	
	wordcount = 0;
	bytecount = 1 + 1 + strlen(name);
	insert_uchar(pkt, wordcount);
	insert_ushort(pkt, bytecount);
	insert_uchar(pkt, 0x04); // buffer format
	insert_string(pkt, fullname);
	
	err = SendSMBPacket(vol, pkt);
	
	DPRINTF(DBG_RENAME, ("send_get_attributes returning %s\n", strerror(err)));
	
	free_packet(& pkt);
exit1:
	FREE(fullname);
	return err;
}


status_t
recv_get_attributes(nspace *vol, ushort *_attr)
{
	status_t err;
	packet *pkt = NULL;
	ushort attr;
	uchar wordcount;
	
	err = RecvSMBPacket(vol, & pkt, SMB_COM_QUERY_INFORMATION);
	if (err != B_OK) {
		DPRINTF(DBG_RENAME, ("recv_get_attributes returning %s\n", strerror(err)));
		return err;
	}
		
	remove_uchar(pkt, & wordcount);
	remove_ushort(pkt, & attr);
	
	*_attr = attr;	
	
	free_packet(& pkt);
	return B_OK;
}


status_t
get_attributes(nspace *vol, vnode *dir, const char *name, ushort *_attr)
{
	status_t err;

	err = send_get_attributes(vol, dir, name);
	if (err != B_OK) return err;
	
	return recv_get_attributes(vol, _attr);
}


status_t
send_rename(nspace *vol, vnode *odir, const char *oldname,
						vnode *ndir, const char *newname,
						uint32 rename_flags)
{
	status_t 	err;
	uchar		WordCount;
	ushort		ByteCount;
	uchar 		SearchAttributes;
		
	packet *pkt = NULL;

	char		*fulloldname=NULL;
	char		*fullnewname=NULL;
	
	ushort		OpenFunction = 0x0012; // Overwrite the new file
	ushort		Flags = 0;

	debug(DBG_RENAME,("send_rename entering\n"));
	
	
	err = get_full_name(vol, odir, oldname, & fulloldname);
	if (err != B_OK) goto exit1;
	
	err = get_full_name(vol, ndir, newname, & fullnewname);
	if (err != B_OK) goto exit2;

	if (rename_flags & RENAME_SAMBA_STYLE) {
		WordCount = 1;
		ByteCount = strlen(fulloldname) + 1 + strlen(fullnewname) + 1 + 2;
		SearchAttributes = FILE_ATTR_HIDDEN | FILE_ATTR_SYSTEM;	
		if (rename_flags & RENAME_IS_DIR)
			SearchAttributes |= FILE_ATTR_DIRECTORY;

		err = new_smb_packet(vol, & pkt, SMB_COM_RENAME);
		if (err != B_OK)
			goto exit3;
		insert_uchar(pkt, WordCount);
		insert_ushort(pkt, SearchAttributes);
		insert_ushort(pkt, ByteCount);
		insert_uchar(pkt, 0x4);
		insert_string(pkt, fulloldname);
		insert_uchar(pkt, 0x4);
		insert_string(pkt, fullnewname);
	} else {
		WordCount = 3;
		ByteCount = strlen(fulloldname) + 1 + strlen(fullnewname) + 1;

		err = new_smb_packet(vol, & pkt, SMB_COM_MOVE);
		if (err != B_OK)
			goto exit3;
		insert_uchar(pkt, WordCount);
		insert_ushort(pkt, -1);
		insert_ushort(pkt, OpenFunction);
		insert_ushort(pkt, Flags);
		insert_ushort(pkt, ByteCount);
		insert_string(pkt, fulloldname);
		insert_string(pkt, fullnewname);
	}

	err = SendSMBPacket(vol,pkt);
	
	free_packet(& pkt);

exit3:
	FREE(fullnewname);
exit2:
	FREE(fulloldname);
exit1:		
	DPRINTF(DBG_RENAME, ("send_rename returning %%s\n",strerror(err)));
	return err;
}

status_t
recv_rename(nspace *vol, uint32 rename_flags) {

	status_t err;
	packet *pkt = NULL;
	
	//dprintf("recv_rename entering \n");
	err = RecvSMBPacket(vol, & pkt,
		(rename_flags&RENAME_SAMBA_STYLE)?SMB_COM_RENAME:SMB_COM_MOVE);
	if (err != B_OK) goto exit1;
		
exit2:
	free_packet(& pkt);
exit1:
	DPRINTF(DBG_RENAME, ("recv_rename returning %s\n", strerror(err)));
	return err;
}

status_t
do_rename(nspace *vol, vnode *odir, const char *oldname,
						vnode *ndir, const char *newname,
						uint32 rename_flags) {

	status_t err;

	err = send_rename(vol, odir, oldname, ndir, newname, rename_flags);
	if (err != B_OK) {
		debug(DBG_RENAME, ("send_rename failed\n"));
		goto exit1;
	}
	
	err = recv_rename(vol, rename_flags);
	if (err != B_OK) {
		debug(DBG_RENAME, ("recv_rename failed\n"));
	}
	
exit1:
	debug(DBG_RENAME,("do_rename exiting with result %s\n", strerror(err)));
	return err;
}
