#include "cifs_globals.h"
#include "netio.h"
#include "packet.h"

#define DBG_MOUNT 10000

status_t
send_negotiate_protocol(nspace *vol) {

	status_t	err;

	packet		*pkt=NULL;
	uchar		wordcount;
	ushort		bytecount;
	

	err = new_smb_packet(vol, & pkt, SMB_COM_NEGOTIATE);
	if (err != B_OK) return err;	

	wordcount = 0;
	bytecount = 1 + strlen(Dialects) + 1;

	insert_uchar(pkt, wordcount);
	insert_ushort(pkt, bytecount);
	insert_uchar(pkt, 0x02);
	insert_string(pkt, Dialects);
	
	err = put_smb_header(vol, pkt);
	if (err != B_OK) goto exit1;	

	err = SendNBPacket(vol, pkt, false);

exit1:
	free_packet(& pkt);
	return err;
}

//Encryption functions
extern void smb_encrypt(const unsigned char *Password, unsigned char *Challenge, char *Response);
//


status_t
recv_negotiate_protocol(nspace *vol) {

	status_t 	err;
	packet		*pkt=NULL;

	uchar		wordcount;
	ushort		DialectIndex;
	uchar		SecurityBlobLength;
	ushort		bytecount;
		
	uchar		dumbbuf[12];
	uchar		Challenge[8];
	
	err = RecvSMBPacket_nocheck(vol, & pkt, SMB_COM_NEGOTIATE);
	if (err != B_OK) {
		debug(-1, ("recv_negotiate_protocol : RecvSMBPacket failed : %d %s\n", err, strerror(err)));
		return err;
	}
	
	
	err = remove_uchar(pkt, & wordcount);				if (err != B_OK) goto exit1;
	if (wordcount != 17) {
		debug(-1, ("Recieved a command with wordcount %d when expecting one with wordcount of 17\n", wordcount));
		err = B_ERROR;
		goto exit1;
	}
	
	
	
	err = remove_ushort(pkt, & DialectIndex);			if (err != B_OK) goto exit1;
	if (DialectIndex != 0) {
		debug(-1, ("Got a dialectindex of %d, sorry.\n", DialectIndex));
		err = B_ERROR;
		goto exit1;
	}


	err = remove_uchar(pkt, & vol->SecurityMode);		if (err != B_OK) goto exit1;
	if (vol->SecurityMode & SHARE_IS_USER_MODE) {
		debug(DBG_MOUNT, ("user level security\n"));
	} else {
		debug(DBG_MOUNT, ("share level security\n"));
	}	

	err = remove_ushort(pkt, & vol->MaxMpxCount);		if (err != B_OK) goto exit1;
	debug(DBG_MOUNT, ("MaxMpxCount %d\n", vol->MaxMpxCount));
	err = remove_ushort(pkt, & vol->MaxNumberVcs);		if (err != B_OK) goto exit1;
	debug(DBG_MOUNT, ("MaxNumberVcs %d\n", vol->MaxNumberVcs));
	err = remove_ulong(pkt, & vol->MaxBufferSize);		if (err != B_OK) goto exit1;
	debug(DBG_MOUNT, ("MaxBufferSize %d\n", vol->MaxBufferSize));
	err = remove_ulong(pkt, & vol->MaxRawBufferSize);	if (err != B_OK) goto exit1;
	debug(DBG_MOUNT, ("MaxRawBufferSize %d\n", vol->MaxRawBufferSize));
	err = remove_ulong(pkt, & vol->SessionKey); 			if (err != B_OK) goto exit1;
	debug(DBG_MOUNT, ("SessionKey %d\n", vol->SessionKey));
	err = remove_ulong(pkt, & vol->Capabilities);		if (err != B_OK) goto exit1;
	debug(DBG_MOUNT, ("Capabilities %d\n", vol->Capabilities));

	
	err = remove_void(pkt, dumbbuf, 10); 					if (err != B_OK) goto exit1;

	err = remove_uchar(pkt, & SecurityBlobLength);		if (err != B_OK) goto exit1;

	err = remove_ushort(pkt, & bytecount);				if (err != B_OK) goto exit1;

	if (SecurityBlobLength == 0) {
		debug(-1, ("YO : SecurityBlobLength is 0.  Attempting clear text password\n"));
		err = B_OK;
		vol->PasswordResponse[0] = '\0';
		goto exit1;
	}

	if (SecurityBlobLength == 8) {
	
		err = remove_void(pkt, Challenge, 8);				if (err != B_OK) goto exit1;
	
		err = remove_unicode(pkt, vol->OemDomainName, OEMDOMAINNAME_LENGTH);		if (err != B_OK) goto exit1;
		//get_cstring_from_smb_unicode(hldr, (*_vol)->OemDomainName, OEMDOMAINNAME_LENGTH);	
			
		// Handle the challenge.

		smb_encrypt((const unsigned char *)vol->Params.Password,
				Challenge, vol->PasswordResponse);

	
#if 0
		memcpy(p21, vol->Params.Password, 21);
			{
				int i=0;
				dprintf("Pass from uspace is\n");
				for (i=0;i<22;i++){
					dprintf("%x:",p21[i]);
				}
				dprintf("\n");
			}
	
		memcpy(c8, Challenge, 8);
		E_P24(p21, c8, p24);
		memcpy(vol->PasswordResponse, p24, 24);
#endif

	} else {

		if (vol->SecurityMode & SHARE_IS_USER_MODE) {
			DPRINTF(-1, ("Server is returning a challenge of length %d.\n", SecurityBlobLength));
			DPRINTF(-1, ("And its in user mode.  This means that I'd have to send\n"));
			DPRINTF(-1, ("your password cleartext over the network.  No.\n"));
			free_packet(& pkt);
			return B_ERROR;
		}
	}

	

exit1:
	if (err != B_OK) DPRINTF(-1, ("negprot recv exiting with %d %s\n", err, strerror(err)));
	free_packet(& pkt);	
	return err;
}
	

status_t
send_session_setup(nspace *vol) {

	status_t	err;
	packet		*pkt=NULL;
	
	
	uchar 	wordcount=13;
	ushort	bytecount=0;
	char	NativeOS[] = {"BeOS"};
	char	NativeLanMan[] = {"LM"};
	ulong	Capabilities = 0;
	
	
	bytecount = strlen(vol->Params.LocalWorkgroup) + 1 + strlen(NativeOS) + 1 + strlen(NativeLanMan) \
					+ 1 + strlen(vol->Params.Username) + 1;

	if (share_in_user_mode(vol)) {
		if (use_cleartext_password(vol)) {
			bytecount += 24;  // Space for the password
		} else {
			// Cleartext pass
			bytecount += strlen(vol->Params.Password);
		}
	}


	err = new_smb_packet(vol, & pkt, SMB_COM_SESSION_SETUP_ANDX);
	if (err != B_OK) return err;
	
	insert_uchar(pkt, wordcount);
	insert_uchar(pkt, 0xff);
	insert_uchar(pkt, 0);
	insert_ushort(pkt, 0);
	insert_ushort(pkt, vol->MaxBufferSize);
		// Here we're telling the server that we can take
		// as much data as they're willing to.
	insert_ushort(pkt, 0); // MaxMpxCount
	insert_ushort(pkt, 0); // VcNumber
	insert_ulong(pkt, 0); // SessionKey
	
	if (share_in_user_mode(vol)) {
		if (use_cleartext_password(vol)) {
			insert_ushort(pkt, (ushort) (strlen(vol->Params.Password) + 1));
		} else {
			insert_ushort(pkt, 24);
		}
	} else {
		insert_ushort(pkt, 2);
	}

	insert_ushort(pkt, 0); // Unicode Password Length
	insert_ulong(pkt, 0); //mbz
	insert_ulong(pkt, Capabilities);

	insert_ushort(pkt, bytecount);

	if (share_in_user_mode(vol)) {
		if (use_cleartext_password(vol)) {
			insert_string(pkt, vol->Params.Password);
		} else {
			insert_void(pkt, vol->PasswordResponse, 24);
		}
	} else {
		insert_ushort(pkt, 0);
	}
	
	insert_string(pkt, vol->Params.Username);
	insert_string(pkt, vol->Params.LocalWorkgroup);
	insert_string(pkt, NativeOS);
	insert_string(pkt, NativeLanMan);

	err = put_smb_header(vol, pkt);
	if (err != B_OK) goto exit1;
	
	err = SendNBPacket(vol, pkt, false);	
		
exit1:
	free_packet(& pkt);
	return err;
}


status_t
recv_session_setup(nspace *vol) {
	
	status_t err;
	packet 	*pkt=NULL;
	ushort	dumbushort, bytecount;
	uchar	dumbuchar;
	
	err = RecvNBPacket_nocheck(vol, & pkt, (CIFS_RECV_TIMEOUT * 1000)); //timeout = 10 sec
	if (err != B_OK) {
		DPRINTF(-1, ("recv_session_setup : RecvNBPacket_nocheck returning %d, %s\n", err, strerror(err)));
		return err;
	}
	
	err = PopSMBHeader(vol, pkt, SMB_COM_SESSION_SETUP_ANDX, true);
	if (err != B_OK) {
		DPRINTF(-1, ("recv_session_setup : PopSMBHeader returning %d, %s\n", err, strerror(err)));
		goto exit1;
	}

	remove_uchar(pkt, & dumbuchar);//wordcount
	remove_uchar(pkt, & dumbuchar);
	remove_uchar(pkt, & dumbuchar);
	remove_ushort(pkt, & dumbushort); //andxoffset
	remove_ushort(pkt, & dumbushort); //action
	remove_ushort(pkt, & bytecount);
	remove_string(pkt, vol->ServerOS, 64);
	remove_string(pkt, vol->ServerLM, 64);
	remove_string(pkt, vol->ServerDomain, 64);	


exit1:
	free_packet(& pkt);
	return err;
}

status_t
send_tree_connect(nspace *vol) {

	status_t err;
	packet	*pkt=NULL;
	
	uchar	wordcount = 4;
	ushort	bytecount;
	char	Service[] = {"?????"};
	
	
	bytecount = strlen(vol->Params.Unc) + 1 + strlen(Service) + 1;
	
	if (! share_in_user_mode(vol)) {
		if (use_cleartext_password(vol)) {
			// Cleartext pass
			bytecount += strlen(vol->Params.Password) + 1;
		} else {
			bytecount += 24;  // Space for the password
		}
	}
	
	
	err = new_smb_packet(vol, & pkt, SMB_COM_TREE_CONNECT_ANDX);
	if (err != B_OK) return err;
	
	insert_uchar(pkt, wordcount);
	insert_uchar(pkt, 0xff);
	insert_uchar(pkt, 0);//mbz
	insert_ushort(pkt, 0);
	insert_ushort(pkt, 0);//connect flags
	
	if (! share_in_user_mode(vol)) {
		if (use_cleartext_password(vol)) {
			insert_ushort(pkt, (ushort) (strlen(vol->Params.Password) + 1));
		} else {
			insert_ushort(pkt, 24);
		}
	} else {
		insert_ushort(pkt, 0);
	}

	insert_ushort(pkt, bytecount);

	if (! share_in_user_mode(vol)) {
		if (use_cleartext_password(vol)) {
			insert_string(pkt, vol->Params.Password);
		} else {
			insert_void(pkt, vol->PasswordResponse, 24);
		}
	}


	insert_string(pkt, vol->Params.Unc);
	insert_string(pkt, Service);

	err = put_smb_header(vol, pkt);
	if (err != B_OK) goto exit1;
	
	err = SendNBPacket(vol, pkt, false);			
	
exit1:
	free_packet(& pkt);
	return err;
}


status_t
recv_tree_connect(nspace *vol) {
	
	status_t err;
	packet 	*pkt=NULL;

	err = RecvNBPacket_nocheck(vol, & pkt, (CIFS_RECV_TIMEOUT * 1000)); //timeout = 10 sec
	if (err != B_OK) {
		DPRINTF(-1, ("recv_session_setup : RecvSMBPacket_nocheck returning %d, %s\n", err, strerror(err)));
		return err;
	}
	
	err = PopSMBHeader(vol, pkt, SMB_COM_TREE_CONNECT_ANDX, true);
	if (err != B_OK) {
		DPRINTF(-1, ("recv_session_setup : PopSMBHeader returning %d, %s\n", err, strerror(err)));
		goto exit1;
	}

exit1:
	free_packet(& pkt);
	return err;
}


status_t
InitNetworkConnection(nspace *vol) {

	status_t err;
	
	vol->connect_state = 0;
	
	err = init_socket();
	if (err != B_OK) return err;
	vol->connect_state |= SOCKET_INIT;

	err = init_netbios_connection(vol);	
	if (err != B_OK) goto exit2;
	vol->connect_state |= NETBIOS_INIT;

	err = send_negotiate_protocol(vol);
	if (err != B_OK) goto exit3;
	
	err = recv_negotiate_protocol(vol);
	if (err != B_OK) goto exit3;

	err = send_session_setup(vol);
	if (err != B_OK) goto exit3;
	
	err = recv_session_setup(vol);
	if (err != B_OK) goto exit3;
	
	err = send_tree_connect(vol);
	if (err != B_OK) goto exit3;
	
	err = recv_tree_connect(vol);
	if (err != B_OK) goto exit3;
	vol->connect_state |= TREE_INIT;	
	
	return B_OK;
	//wow
	

exit3:
	kill_netbios_connection(vol);
	vol->connect_state &= ~NETBIOS_INIT;
exit2:
	kill_socket();
	vol->connect_state &= ~SOCKET_INIT;
exit1:
	return err;
}

void
KillNetworkConnection(nspace *vol) {
	
	if (! vol)
		return;

	if (vol->connect_state & TREE_INIT) {
		tree_disconnect(vol);
		vol->connect_state &= ~TREE_INIT;
	}
	if (vol->connect_state & NETBIOS_INIT) {
		kill_netbios_connection(vol);
		vol->connect_state &= ~NETBIOS_INIT;
	}
	if (vol->connect_state & SOCKET_INIT) {
		kill_socket();
		vol->connect_state &= ~SOCKET_INIT;
	}
}


// Copy parameters passed from mount
int
copy_parms(nspace* vol, parms_t* parms) {
	
#if 0
	strncpy(vol->Params.Username, parms->Username, 255);
	memcpy(vol->Params.Password, parms->Password, 255);
	strcpy(vol->Params.Unc, parms->Unc);
	strncpy(vol->Params.Server, parms->Server, 255);
	strncpy(vol->Params.Share, parms->Share, 255);
	strncpy(vol->Params.LocalShareName, parms->LocalShareName, 255);
	strncpy(vol->Params.LocalWorkgroup, parms->LocalWorkgroup, 64);
	memcpy(& vol->Params.server_inetaddr, & parms->server_inetaddr, sizeof(ulong));
#endif

	memcpy(& vol->Params, parms, sizeof(parms_t));

	if (! parms->server_inetaddr) {
		debug(-1,("No server address\n"));
		return B_ERROR;
	}
	
	if (!parms->Unc || !parms->Server || !parms->Share) {
		debug(-1,("No server info\n"));
		return B_ERROR;
	}

	


	//dprintf("Are you sure you want to use caseless pathnames?\n");
	//vol->Flags = 0x8;  // XXX using caseless pathnames
	
	vol->Flags = 0;
	vol->Flags2 = 0x1;

	return B_OK;
}





