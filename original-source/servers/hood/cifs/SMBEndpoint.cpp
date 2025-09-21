#include "debug.h"
#include "SMBEndpoint.h"
#include <stdio.h>
#include <ctype.h>

SMBEndpoint::SMBEndpoint() {

	m_flags = 0x8;
	m_flags2 = 0x1;

	m_pid = 0;
	m_tid = 0;
	m_mid = 0;
	m_uid = 0;
	
	m_dopack[0] = 0;
	m_username[0] = 0;
	m_password[0] = 0;
	m_share[0] = 0;
	
	setRecvTimeout(5000); // Set recv timeouts to be 5 seconds.

}

SMBEndpoint::~SMBEndpoint() {

}

status_t
SMBEndpoint::logon(NBName &server_name, NLAddress &server_addr)
{
	status_t result;


	debug(2,("SMBEndpoint:: logon to %s\n", server_name.orig()));
	strcpy(m_Server, server_name.orig());

	// NBEndpoint::connect is exception ok
	result = NBEndpoint::connect(server_name, server_addr);
	if (result != B_OK) return result;
	
	debug(2,("SMBEndpoint:: connnected to %s\n", server_name.orig()));
	
	result = Negotiate();
	if (result != B_OK) return result;
	debug(2,("SMBEndpoint:: negotiated to %s\n", server_name.orig()));


	result = SessionSetup();
	if (result != B_OK) return result;
	debug(2,("SMBEndpoint:: SessionSetup succesful\n"));

	result = TreeConnect();
	if (result != B_OK) {
		debug(-1,("SMBEndpoint:: TreeConnect failed, returning %x\n", result));
		return result;
	}
	
	debug(2,("SMBEndpoint:: TreeConnect succesful\n"));
	return B_OK;
}

status_t
SMBEndpoint::p_FormHeader(NLPacket &head, int command)
{
	try {

	head.setorder(false);
	head.insert((char) 0xff);
	head.insert('S');
	head.insert('M');
	head.insert('B');	
	head.insert((char) command);

	head.insert((ulong) 0); // errors
	head.insert(m_flags); // flags
	head.insert(m_flags2); // flags2
	head.insert((ulong) 0); 
	head.insert((ulong) 0); 
	head.insert((ulong) 0);

	head.insert(m_tid);
	head.insert(m_pid);
	head.insert(m_uid);
	head.insert(m_mid);
	
	return B_OK;
	
	} catch (...) {
		return B_ERROR;
	}
}


status_t
SMBEndpoint::p_popHeader(NLPacket &head, int command, bool getIDs)
{
	status_t err;

	char dumbchar;
	uchar dumbuchar;
	ulong dumbulong;
	ushort dumbushort;
		
	try {
	
	head.setorder(false);
	head.remove(dumbuchar);
	if (dumbuchar != 0xff)
		return B_ERROR;
		
	head.remove(dumbchar);
	if (dumbchar != 'S')
		return B_ERROR;
		
	head.remove(dumbchar);
	if (dumbchar != 'M')
		return B_ERROR;
		
	head.remove(dumbchar);
	if (dumbchar != 'B')
		return B_ERROR;
		
	head.remove(dumbuchar);
	if (dumbuchar != command)
		return B_ERROR;
		
	head.remove(dumbulong); // errors
	if (dumbulong != 0) {
		return EACCES;
	}

	head.remove(dumbchar); // flags
	head.remove(dumbushort); // flags2
	head.remove(dumbulong); 
	head.remove(dumbulong); 
	head.remove(dumbulong);

	if (getIDs) {
		head.remove(m_tid);  //tid
		head.remove(m_pid); //pid
		head.remove(m_uid); //uid
		head.remove(m_mid); //mid
	} else {
		head.remove(dumbushort);  //tid
		head.remove(dumbushort); //pid
		head.remove(dumbushort); //uid
		head.remove(dumbushort); //mid
	}
	
	return B_OK;

	} catch(...) {
		return B_ERROR;
	}

}


status_t
SMBEndpoint::Negotiate()
{
	status_t result;
	
	char dialect[] = "NT LM 0.12";

	try {
	
	{ // Send Negotiation Request	
		NLPacket req;
		req.setorder(false);
		p_FormHeader(req, SMB_COM_NEGOTIATE);
		req.insert((char) 0);
		req.insert((ushort) (strlen(dialect) + 1));
		req.insert((char) 0x2);
		req.insert(dialect);
		send(req);
	}

	// Get reply	
	NLPacket reply;
	result = NBEndpoint::recv(reply);
	if (result < 0) return result;
	
	p_popHeader(reply, SMB_COM_NEGOTIATE, false);
	reply.setorder(false);
	
	uchar wordcount;
	reply.remove(wordcount);
	
	ushort DialectIndex;
	reply.remove(DialectIndex);


	//reply.remove(m_SecurityMode);
	uchar SecurityMode;
	reply.remove(SecurityMode);
	m_UserLevelSecurity = SecurityMode & SHARE_IS_USER_MODE;
	if (m_UserLevelSecurity) {
		debug(2,("User level security\n"));
	} else {
		debug(2,("Share level Security\n"));
	}



	reply.remove(m_MaxMpxCount);
	reply.remove(m_MaxNumberVcs);
	reply.remove(m_MaxBufferSize);
	reply.remove(m_MaxRawSize);
	reply.remove(m_SessionKey);
	reply.remove(m_Capabilities);

	// if (m_Capabilities & 0x80000000) 
	
	// Skip SystemTimeLow, SystemTimeHigh, ServerTimeZone
	ulong dumbulong;
	ushort dumbushort;
	reply.remove(dumbulong);
	reply.remove(dumbulong);
	reply.remove(dumbushort);
		
	uchar SecurityBlobLength;
	reply.remove(SecurityBlobLength);
	
	if (SecurityBlobLength != 8) {
		debug(-1,("SMBEndpoint::Negotiate : SecurityBlobLength is (dec) %d !!!\n", SecurityBlobLength));
	}
	
	ushort bytecount;
	reply.remove(bytecount);
	debug(2,("SMBEndpoint::Negotiate : %s bytecount is (dec) %d\n", m_Server, bytecount));
	
	if (bytecount >= 8) {
		reply.remove((void *) m_Challenge, 8);
	}
	
	if (bytecount >= 24) {
		reply.remove_unicode(m_dopack, 16);
	}
		
	return B_OK;

	} catch(...) {
		return B_ERROR;
	}


}	



status_t
SMBEndpoint::SessionSetup()
{
	status_t result;

	try {

	{ // Send packet	
		NLPacket req;
		req.setorder(false);
		
		p_FormHeader(req, SMB_COM_SESSION_SETUP_ANDX);
		char wordcount = 13;
		req.insert(wordcount);
		req.insert((char) 0xff); // AndXCommand, none
		req.insert((char) 0); //mbz
		req.insert((ushort) 0); //AndXOffset
		
		ushort Client_MaxBufferSize = 0xffff;
		ushort Client_MaxMpxCount = 0x2;
		req.insert(Client_MaxBufferSize);
		req.insert(Client_MaxMpxCount);
		
		req.insert((ushort) 0); // VcNumber
		req.insert((ulong) 0); // SessionKey (not the one from the server)
		
		// heres where it gets iffy, cause the format of the
		// header changes depending on whether you try to login or not.
		
		ushort passlen;

		if (! m_username[0]) {  // If we don't have login info	
			passlen = 0;
			req.insert((ushort) 1);	
			req.insert((ushort) 0); // Unicode Password Length
			req.insert((ulong) 0); // mbz
			req.insert((ulong) 0); // Client Capabilities
			
			
			ushort ByteCount = 1 + 1 + 1  \
									+ 1 + strlen("BeOS") + 1 + 1 \
									+ strlen("BeLM") + 1;
			req.insert(ByteCount);
			req.insert((char) 0); // This would be the password
			req.insert((char) 0); //Unicode Password, we dont use
			//req.insert(m_username);
			req.insert("");
			req.insert("");
			req.insert("BeOS");
			req.insert((char) 0); // mbz
			req.insert("BeLM");
	
		} else {
	
			if (m_UserLevelSecurity) {
				passlen = 24;
			} else {
				passlen = 2;
			}
		
			req.insert(passlen);
			req.insert((ushort) 0); // Unicode Password Length
			req.insert((ulong) 0); // mbz
			req.insert((ulong) 0); // Client Capabilities
			
			ushort ByteCount = strlen(m_username) + 1  \
									+ strlen(m_dopack) + 1 + strlen("BeOS") + 1
									+ strlen("BeLM") + 1;
	
			if (m_UserLevelSecurity) {
				ByteCount += 24; //Add room for password
			} else {
				ByteCount += 1;
			}
				
			req.insert(ByteCount);
	
			// Now we need to insert the 24 byte password, then the username,
			// but we have to process the challenge, and then figure out
			// which password/username to use - the one in the object or
			// the ones found in the network settings file.
			
			if (m_UserLevelSecurity) {
				// Let's do the Password encryption
				smb_encrypt((const unsigned char *)m_password,
							(unsigned char *)m_Challenge,
							m_PasswordResponse);
				req.insert((void*) m_PasswordResponse, 24);
			} else {
				req.insert((ushort) 0);
			}
	
			req.insert(m_username);
			req.insert(m_dopack);
			req.insert("BeOS");
			req.insert("BeLM");
		}

		send(req);
	}
	
	NLPacket reply;
	result = recv(reply);
	if (result < 0) return result;

	reply.setorder(false);
	return p_popHeader(reply, SMB_COM_SESSION_SETUP_ANDX, true);

	} catch (...) {
		return B_ERROR;
	}
}




status_t
SMBEndpoint::TreeConnect()
{
	status_t result;

	try {
	
	NLPacket req;
	req.setorder(false);
	p_FormHeader(req, SMB_COM_TREE_CONNECT_ANDX);
	char wordcount = 4;
	req.insert(wordcount);
	req.insert((char) 0xff); // AndXCommand, none
	req.insert((char) 0); //mbz
	req.insert((ushort) 0); //AndXOffset
	
	req.insert((ushort) 0); //TreeConnect Flags
	if (m_UserLevelSecurity) {
		req.insert((ushort) 0);
	} else {
		req.insert((ushort) 24);
	}

	ushort bytecount = 1 + 1 + strlen(m_Server) + 1 + \
						strlen(m_share) + 1;
						
	if (! m_UserLevelSecurity) {
		bytecount += 24; // add room for password
	}
	req.insert(bytecount);
	
	if (! m_UserLevelSecurity) {
		smb_encrypt((const unsigned char *)m_password,
					(unsigned char *)m_Challenge,
					m_PasswordResponse);
		req.insert((void*) m_PasswordResponse, 24);
	}
	
	char Path[256];
	sprintf(Path, "\\\\%s\\%s\0", m_Server, m_share);
	req.insert(Path);
	req.insert("?????"); // DeviceType
	send(req);
	
	
	NLPacket reply;
	result = recv(reply);
	if (result < 0) return result;

	return p_popHeader(reply, SMB_COM_TREE_CONNECT_ANDX, true);


	} catch (...) {
		return B_ERROR;
	}


}

static bool
ishiddenshare(const char *name)
{
	// Name is 13 chars long.
	for(int i=12;i--;i>=0) {
		if (name[i] == '$')
			return true;
	}
	return false;
}

status_t
SMBEndpoint::NetShareEnum(bool showhidden, share_info **shares, int *num)
{
	status_t result;

	try {
	
	// Request
	NLPacket request;
	NLPacket reqparam;
	NLPacket reqdata;
	reqparam.setorder(false);
	ushort func_num = 0;
	char param_desc[] = "WrLeh";
	char data_desc[] = "B13BWz";
	ushort level = 1; // Give us full info
	ushort recv_buffer_size = 0xffff;
	reqparam.insert(func_num);
	reqparam.insert(param_desc);
	reqparam.insert(data_desc);
	reqparam.insert(level);
	reqparam.insert(recv_buffer_size);
	p_FormRAPTransact(request, reqparam, reqdata);

	send(request);
	
	// Reply
	NLPacket reply;
	NLPacket repparam;
	NLPacket repdata;
	
	result = recv(reply);
	if (result < 0) return result;
	

	status_t parse;
	parse = p_ParseRAPReply(reply, repparam, repdata);
	if (parse != B_OK)
		return parse;


	
	repparam.setorder(false);
	ushort status;
	ushort converter;
	ushort num_entries;
	ushort total_entries;

	repparam.remove(status);
	repparam.remove(converter);
	repparam.remove(num_entries);
	repparam.remove(total_entries);
	
	//printf("Netshareenum got:\n");
	//printf("\tstatus        %d\n",status);
	//printf("\tccnverter     %d\n",converter);
	//printf("\tnum_entries   %d\n",num_entries);
	//printf("\ttotal_entries %d\n",total_entries);
	

	repdata.setorder(false);
	
	*shares = (share_info*) calloc(num_entries*sizeof(share_info),1);
	//*shares = new share_info[num_entries];

	int skip = 0;
	char dumbchar;
	ushort type;
	ulong dumbulong;

	for (int i=0; i < num_entries; ++i) {
		char netname[13];
		repdata.remove((void*) netname, 13);
		if ((!showhidden) && ishiddenshare(netname)) {
			repdata.remove(dumbchar);
			repdata.remove(type);
			repdata.remove(dumbulong);
			skip++;
			continue;
		}
		share_info *sh = &(*shares)[i-skip];
			
		memcpy(sh->name, netname, 13);
		(sh->name)[13] = '\0';
		repdata.remove(dumbchar);
		repdata.remove(type);
		sh->type = type;
		repdata.remove(dumbulong);
	}


	*num = num_entries - skip;
	
	return B_OK;

	} catch (...) {
		return B_ERROR;
	}

}

status_t
SMBEndpoint::NetServerEnum2(server_info **servers, int *num, const char* domain)
{
	status_t result;
	
	try {
	
	{// Request
		NLPacket reqparam;
		NLPacket reqdata;
		NLPacket *request;

		request = new NLPacket;
		if (!request) return ENOMEM;
		
		reqparam.setorder(false);
		ushort func_num = 104;
		char param_desc[] = "WrLehDO";
		char data_desc[] = "B16BBDz";
		ushort level = 1; // Give us full info
		ushort recv_buffer_size = 0xffff;
		
		reqparam.insert(func_num);
		reqparam.insert(param_desc);
		reqparam.insert(data_desc);
		reqparam.insert(level);
		reqparam.insert(recv_buffer_size);
		if (domain == NULL) { // Enumerate all Domains
			reqparam.insert((ulong) 0x80000000); // Enumerate Domains
			reqparam.insert((uchar) 0);  // null-terminated string
		} else {
			reqparam.insert((ulong) 0x00000003); // All workstations, all servers
			reqparam.insert(domain);  // null-terminated string
		}
			
		p_FormRAPTransact(*request, reqparam, reqdata);
		
		send(*request);
		delete request;
	}	
	
	// Reply
	NLPacket repparam;
	NLPacket repdata;
	NLPacket *reply;
	
	reply = new NLPacket;
	if (!reply) return ENOMEM;

	result = recv(*reply);
	if (result < 0) {
		delete reply;
		return result;
	}
	
	
	status_t parse;
	parse = p_ParseRAPReply(*reply, repparam, repdata);
	if (parse != B_OK) {
		delete reply;
		return parse;
	}

	repparam.setorder(false);

	ushort status;
	ushort converter;
	ushort num_entries;
	ushort total_entries;

	repparam.remove(status);
	repparam.remove(converter);
	repparam.remove(num_entries);
	repparam.remove(total_entries);
	
	// get listings

	*num = num_entries;
	*servers = (server_info *) calloc(num_entries * sizeof(server_info), 1);
		
	for (int i=0; i < num_entries; ++i) {
			char name[16];
			repdata.remove((void*) name, 16);
	
			server_info *si = &(*servers)[i];
			memcpy(si->name, name, 16);

			char dumbchar;
			repdata.remove(dumbchar); // version major
			repdata.remove(dumbchar); // version minor
			ulong dumbulong;
			repdata.remove(dumbulong);	// type
			repdata.remove(dumbulong); // No idea.  Not documented.
	}
	
	delete reply;
	return B_OK;

	} catch (...) {
		return B_ERROR;
	}	
}


status_t
SMBEndpoint::p_ParseRAPReply(NLPacket &reply, NLPacket &param, NLPacket &data)
{
	try {

	reply.setorder(false);
	size_t base = reply.getPos();  // Need this for parameter and data offsets
	
	status_t parse;
	parse = p_popHeader(reply, SMB_COM_TRANSACT, false);
	if (parse != B_OK)
		return parse;
		
	uchar wordcount;
	ushort total_param_bytes;
	ushort total_data_bytes;
	ushort dumb;
	ushort param_bytes;
	ushort param_offset;
	ushort param_displacement;
	ushort data_bytes;
	ushort data_offset;
	ushort data_displacement;
	uchar max_setup_words;
	uchar dumbuchar;
	ushort bytecount;
	
	reply.remove(wordcount);
	if (wordcount != 10)
		return B_ERROR;
		

	reply.remove(total_param_bytes);
	reply.remove(total_data_bytes);
	reply.remove(dumb);
	reply.remove(param_bytes);
	reply.remove(param_offset);
	reply.remove(param_displacement);
	reply.remove(data_bytes);
	reply.remove(data_offset);
	reply.remove(data_displacement);
	reply.remove(max_setup_words);
	reply.remove(dumbuchar);
	reply.remove(bytecount);

	size_t skipthis = base + param_offset - reply.getPos();
	reply.skip(skipthis);

	char *param_buf = new char[param_bytes];
	reply.remove((void*) param_buf, param_bytes);
	param.insert((void*) param_buf, param_bytes);
	delete [] param_buf;

	
	// may need to account for padding before popping the data section
	skipthis = base + data_offset - reply.getPos();
	reply.skip(skipthis);
	
	//printf("data_bytes is %d\n", data_bytes);
	//printf("remaining is %d\n", reply.getBytesRemaining());
	
	char *data_buf = new char[data_bytes];
	reply.remove((void*) data_buf, data_bytes);
	data.insert((void*) data_buf, data_bytes);
	delete [] data_buf;

	return B_OK;

	} catch(...) {
		return B_ERROR;
	}
}


status_t
SMBEndpoint::p_FormRAPTransact(NLPacket &pack, NLPacket &param, NLPacket &data)
{

	int header_len = 76;

	try {

	// SMB Transact
	p_FormHeader(pack, SMB_COM_TRANSACT);

	pack.setorder(false);
	pack.insert((uchar) 0x0e); //wordcount
	pack.insert((ushort) param.getSize()); // total parm bytes
	pack.insert((ushort) data.getSize());  // total data bytes
	pack.insert((ushort) 128); // max parm bytes
	pack.insert((ushort) 0xffff); // max data bytes
	pack.insert((uchar) 0); // max setup words
	pack.insert((uchar) 0); // mbz
	
	pack.insert((ushort) 0); // transact flags : leave session intact, response required
	pack.insert((ulong) 0); // transact timeout
	pack.insert((ushort) 0); // mbz
	pack.insert((ushort) param.getSize()); // paramater bytes
	pack.insert((ushort) header_len); // parameter offset : this right?
	pack.insert((ushort) data.getSize()); // data bytes
	pack.insert((ushort) (header_len + data.getSize())); // data offset
	pack.insert((uchar) 0); // max setup words
	pack.insert((uchar) 0); // mbz

	char file[] = "\\PIPE\\LANMAN";
	ushort bytecount = strlen(file) + 1 + param.getSize() + data.getSize();

	pack.insert(bytecount);
	pack.insert(file);
	pack.insert(param);
	pack.insert(data);

	return B_OK;
	
	} catch (...) {
		return B_ERROR;
	}

}


// Is there a std c++ lib call for this?
void SMBEndpoint::strtoupper(char* str) {
int len = strlen(str);
	for(int i = 0; i <= len; i++) {
		str[i] = toupper(str[i]);
	}
}


status_t
SMBEndpoint::SendPrintJob(int fd, const char* fname)
{
	status_t result;
	
	uint16 servFD;
	result = OpenPrintFile(servFD, fname);
	if (result != B_OK) return result;
	
	result = SendPrintFile(fd, servFD);
	if (result != B_OK) return result;
	
	result = ClosePrintFile(servFD);
	return result;

}



status_t
SMBEndpoint::OpenPrintFile(uint16 &servFD, const char* fname)
{
	status_t result;

	if (fname == NULL)
		fname = "Network Print from BeOS";

	try {

	NLPacket request;
	request.setorder(false);
	p_FormHeader(request, SMB_COM_OPEN_PRINT_FILE);
	
	request.insert((uchar) 2); //wordcount
	request.insert((ushort) 0); //SetupLength
	request.insert((ushort) 1); //Do not translate tab's to spaces
	request.insert((ushort) (1 + 1 + strlen(fname))); //bytecount
	request.insert((uchar) 4); //BufferType: zstring
	request.insert(fname);
	send(request);
	
	
	NLPacket reply;
	reply.setorder(false);
	
	result = recv(reply);
	if (result < 0) return result;
	
	result = p_popHeader(reply, SMB_COM_OPEN_PRINT_FILE, false);
	if (result) return result;
	
	uchar wordcount;
	reply.remove(wordcount);
	if (wordcount != 1) return B_ERROR;
	
	reply.remove(servFD);
	
	return B_OK;

	} catch(...) {
		return B_ERROR;
	}

}

static status_t full_read(int fd, void *buf, int bufsize)
{
	int left = bufsize;
	uchar *cp = (uchar *) buf;
	int len;
	
	while (left > 0) {
		len = read(fd,cp,left);
		if (len <= 0) return len;
		
		left -= len;
		cp += len;
	}
	return bufsize;
}

status_t
SMBEndpoint::SendPrintFile(int fd, uint16 servFD)
{

	int32 len;
	uint16 len16;
	status_t result;
	uchar *buf = NULL;

	try {

	uint32 bufsize = m_MaxBufferSize - 64;	
	//printf("Sending Print File, in (dec) %d chunks\n", bufsize);
	buf = new uchar[bufsize];

	while ((len=full_read(fd,buf,bufsize)) > 0) {
		uint16 bytecount = len + 3;
		uchar wordcount = 1;
		len16 = len;

		NLPacket packet(64 + bufsize);  // Size of chunk plus enough for SMB header
		packet.setorder(false);

		p_FormHeader(packet, SMB_COM_WRITE_PRINT_FILE);
		packet.insert(wordcount);
		packet.insert(servFD);
		packet.insert(bytecount);
		packet.insert((uchar) 1); //BufferType: data
		packet.insert(len16);
		packet.insert((void*) buf, len16);
		send(packet);
		
		NLPacket reply;
		result = recv(reply);
		if (result < 0) break; else result = 0;
		
		result = p_popHeader(reply, SMB_COM_WRITE_PRINT_FILE, false);
		if (result) break;
	}

	if (len < 0) result = len;
	
	delete[] buf;
	
	return result;

	} catch (...) {
		return B_ERROR;
	}
}


status_t
SMBEndpoint::ClosePrintFile(uint16 servFD)
{
	status_t result;

	try {
		
	NLPacket request;
	request.setorder(false);
	p_FormHeader(request, SMB_COM_CLOSE);
	
	request.insert((uchar) 3); //wordcount
	request.insert(servFD);
	request.insert((ulong) 0); //utime, uselese
	request.insert((ushort) 0); //bytecount
	send(request);
	
	
	NLPacket reply;
	reply.setorder(false);

	result = recv(reply);
	if (result < 0) return result;
	
	result = p_popHeader(reply, SMB_COM_CLOSE, false);
	
	return B_OK;

	} catch(...) {
		return B_ERROR;
	}
}


