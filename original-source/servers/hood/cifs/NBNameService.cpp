#include "NBNameService.h"
#include <settings/Settings.h>
#include <unistd.h>

#define debug(a,b) printf(b)

NBNameService::NBNameService()
{
	char semname[64];
	sprintf(semname,"nbnameservice %x\0",real_time_clock());
	server_sem = create_sem(1,semname);
}

NBNameService::~NBNameService()
{
	int32 dummy;
	delete_sem(server_sem);
	wait_for_thread(server_thread,&dummy);
}


// Server Functions

// Call Start, which calls thread_func, which calls _start.
// Wow.
status_t
NBNameService::Start(void)
{
	server_thread = spawn_thread(thread_func, "NBNameService", B_NORMAL_PRIORITY, this); 
	return (resume_thread(server_thread)); 
}

status_t
NBNameService::thread_func(void *arg)
{
	NBNameService *obj = (NBNameService*) arg;
	return (obj->_start());
}

status_t
NBNameService::_start(void)
{
	status_t err = B_NO_ERROR;	
	NLEndpoint comm(NLEndpoint::datagram);
	comm.setReusePort(true);
	comm.bind(NLAddress::udpNBname);
	
	// Setup our info
	SetInfo(comm);
	
	// Need to get these off the heap; stack allocation
	// was causing spurious crashes (stack overflow)
	NLPacket *pack = new NLPacket;
	NLPacket *reply = new NLPacket;
	NLAddress whence;
	
	// Listen for traffic, respond if necessary.
	while (acquire_sem(server_sem) == B_OK) {
		const int maxsize = 512;  // big enough for netbios packets
		if (comm.recvfrom(*pack, maxsize, whence) < 0) {
			printf("recvfrom in NBNameService failed\n");
			try {
				comm.Reset();
				comm.setReusePort(true);
				comm.bind(NLAddress::udpNBname);
			} catch (...) {
				printf("caught execption while resetting, continuing..\n");
				release_sem(server_sem);
				continue;
			}
			release_sem(server_sem);
			continue;
		}
		
		if (q_HandlePacket(*pack, *reply)) {
			comm.sendto(*reply, whence);
		}
		pack->reset();
		reply->reset();
		release_sem(server_sem);
	}
	
	delete pack;
	delete reply;
	
	return err;
}

// Should we reply to pack?  If so, put the reply in reply
bool
NBNameService::q_HandlePacket(NLPacket &pack, NLPacket &reply)
{
	ushort transid;
	ushort flags;
	
	pack.remove(transid);
	pack.remove(flags);
	
	bool is_req, is_query;
	is_req = !(flags&0x8000);
	is_query = !(flags&0x7800);
	if (is_query & is_req)
		return p_HandleReqQuery(transid, pack, reply);
		
	return false;	
}

// Someone has a question, see if they're asking about us and
// prepare a reply if so.
bool
NBNameService::p_HandleReqQuery(ushort transID, NLPacket &req, NLPacket &reply)
{
	status_t err;
	// Do they have a question?
	ushort question_count;
	req.remove(question_count);
	if (question_count == 0)
		return false;

	// Then they shouldnt have any answers
	ushort answer_count;
	req.remove(answer_count);
	if (answer_count > 0)
		return false;
		
	// These arent checked yet
	ushort ns_count;
	ushort record_count;
	req.remove(ns_count);
	req.remove(record_count);
	
	// Who do they want to know about?
	char pulled_name[34];
	req.remove(pulled_name, 34);
	NBName queryMachine;
	queryMachine.setNBForm(pulled_name);
	
	// Are they interested in us?
	if (m_name != queryMachine) {
		return false;
	}	

	// Ok, they want to know who we are.
	// Prepare the reply.	
	reply.insert(transID);
	reply.insert((ushort) 0x8500); // Response, Query, Success
	reply.insert((ushort) 0); // QuestionCount
	reply.insert((ushort) 1); // AnswerCount
	reply.insert((ushort) 0); // NameServiceCount
	reply.insert((ushort) 0); // Additional Record Count
	
	reply.insert(m_name.net());
	reply.insert((ushort) 0x20); // Netbios General Name Service
	reply.insert((ushort) 1); // Internet Class
	reply.insert((ulong) 0x493e0); // Time to live (How long?)
	reply.insert((ushort) 6); // Rdata Length
	reply.insert((ushort) 0); // Record Flags, meaning Unique Name

	in_addr _ipaddr;
	m_addr.get(_ipaddr);
	ulong ipaddr = ntohl(_ipaddr.s_addr);
	reply.insert(ipaddr); // Ip address
	return true;
}

// Gather up hostname,address,domain
status_t
NBNameService::SetInfo(NLEndpoint &comm)
{
	char hostname[255];
	gethostname(hostname, 255);
	m_name.setNormal(hostname, 0);
	
	m_addr = comm.getAddr();
	
	void *cookie;
	char domain[64] = "";
	if (get_config("da_hood", "ms_sharing", &cookie) == B_OK) {
		if (lock_config(cookie) == B_OK) {
			get_config_value(cookie, "MSWORKGROUP", domain, sizeof(domain), "");
			unlock_config(cookie);
		}
		put_config(cookie);
	}
	else
	{
		FILE	*f;
		
		if( (f = fopen( "/etc/msworkgroup", "r" )) != NULL )
		{
			fgets(domain, 64, f);
			for( int i=0; domain[i]; i++ )
			{
				if( domain[i] == '\n' )
				{
					domain[i] = 0;
					break;
				}
			}
			fclose(f);
		}
		else
			strcpy( domain, "WORKGROUP" );
	}

	if (domain[0] != 0x00) {
		//we successfully retrieved our domain - remember it
		m_domain.setNormal(domain,0);

	} else {
		debug(-1, ("Couldnt obtain domain from setttings file, must set explicitly\n"));
	}
	
	return B_OK;
}
	
//Explicitly set domain
void
NBNameService::SetDomain(const char *domain)
{
	m_domain.setNormal(domain,0);
}

// Query functions
status_t
NBNameService::Resolve(NBName &name, NLAddress &addr)
{
	status_t err;
	

	err = SearchCache(name, addr);
	if (err == B_OK) {
		debug(-1,("found %s in cache\n", name.orig()));
		return B_OK;
	}
	
	err = NetResolve(name,addr);
	if (err != B_OK) {
		debug(-1,("couldn't resolve %s", name.orig()));
		return err;
	}
	
	// Remember for later
	AddCache(name,addr);
	return B_OK;
}

status_t
NBNameService::NetResolve(NBName &name, NLAddress &addr)
{
	status_t err;
	const int NETRESOLVETIMEOUT=3000; // 3 seconds
	try {

	// Book of Armorments
	NLEndpoint comm(NLEndpoint::datagram);
	comm.setReusePort(true);
	comm.bind(NLAddress::udpNBname);
	NLAddress bcast(NLAddress::addr_bcast, NLAddress::udpNBname);

	// Prepare the holy hand grenade
	NLPacket req;
	ushort transid = NewTransid();
	req.insert(transid);
	req.insert((ushort) 0x0010); // Flags
	req.insert((ushort) 1); // Question Count
	req.insert((ushort) 0); // Answer Count
	req.insert((ushort) 0); // Name Service Count
	req.insert((ushort) 0); // Additional Record Count
	req.insert(name.net()); // The name we want to resolve
	req.insert((ushort) 0x20); // General Name Service
	req.insert((ushort) 0x1); // Internet Class

	// Three sir!	
	comm.sendto(req, bcast);

	
	NLPacket reply;
	NLAddress whence;
	for (int retry=3;retry>0;retry--) {
		err = WaitForNBPacket(NETRESOLVETIMEOUT, transid,
				true, false, comm, reply, whence);
		if (err != B_OK) {
			reply.reset();
			continue;
		}
		err = ParseNameReply(name, addr, reply);
		if (err != B_OK) {
			reply.reset();
			continue;
		} else {
			break;
		}
	}
	return err;

	} catch (NLException exc) {
		printf("NetResolve caught an exception on %s\n", name.orig());
		char str[1024];
		exc.print(str);
		return B_ERROR;
	}
	//not an exit
	return B_ERROR;
}

// Takes a packet and grabs the name and IP address from it.
status_t
NBNameService::ParseNameReply(NBName &name, NLAddress &addr, NLPacket &reply)
{
	ushort question_count;
	reply.remove(question_count);
	if (question_count != 0)
		return B_ERROR;
		
	ushort answer_count;
	reply.remove(answer_count);
	if (answer_count != 1)
		return B_ERROR;

	ushort ns_count;
	ushort record_count;
	reply.remove(ns_count);
	reply.remove(record_count);
	if ((ns_count != 0) | (record_count != 0))
		return B_ERROR;	

	char _respname[34];
	reply.remove(_respname, 34);
	NBName respname;
	respname.setNBForm(_respname);
	if (respname != name)
		return B_ERROR;
		
	ushort record_type;
	ushort record_class;
	reply.remove(record_type);
	reply.remove(record_class);
	if ((record_type != 0x20) | (record_class != 0x1))
		return B_ERROR;

	ulong ttl;
	ushort rdata_len;
	ushort rflags;
	reply.remove(ttl);
	reply.remove(rdata_len);
	reply.remove(rflags);
	
	ulong ip_addr;
	reply.remove(ip_addr);
	in_addr _ipaddr;
	_ipaddr.s_addr = htonl(ip_addr);
	addr.set(_ipaddr);
	return B_OK;
}

// Wait for a total of waittime microseconds, grabbing netbios packets
// off of comm.  If their transid,query,and req fields match
// those called, return the packet.  Otherwise, return B_TIMED_OUT
// If returned, packet has had transid and flags popped off
// If called with waittime < 0, will wait indefinitely
status_t
NBNameService::WaitForNBPacket(	long waittime,
								ushort transid,
								bool query,
								bool req,
								NLEndpoint &comm,
								NLPacket &pack,
								NLAddress &whence )
{
	bigtime_t start_time = system_time();
	bigtime_t end_time = start_time + waittime;
	long timeleft;
	status_t err;

	for (timeleft = waittime; ; timeleft = end_time - system_time()) {
		comm.setRecvTimeout((long) timeleft);
		const int maxsize = 512;
		err = comm.recvfrom(pack,maxsize,whence);
		if (err == B_TIMED_OUT) {
			printf("waitforpacket returning b_timed_out\n");
			return err;
		}
		if (err < 0) {
			printf("waitforpacket returning error %s\n",strerror(err));
			return err;
		}
		
		//printf("waitforpacket got packet\n");
			
		ushort pack_transid;
		 pack.remove(pack_transid);
		if(pack_transid != transid) {
			//printf("transid doesnt match : want %d got %d\n", transid, pack_transid);
			pack.reset();
			continue;
		}
		ushort pack_flags;
		bool pack_query,pack_req;
		pack.remove(pack_flags);
		pack_req = !(pack_flags&0x8000);
		pack_query = !(pack_flags&0x7800);
		//printf("pack_flags is 0x%x\n", pack_flags);
		//printf("pack_req is %d pack_query is %d\n", pack_req, pack_query);
		if ( (pack_req != req) || (pack_query != query)) {
			//printf("transid doesnt match req or query \n");
			pack.reset();
			continue;
		}
		
		//printf("waitforpacket returning B_OK");
		return B_OK;
	}
	//not an exit
	return B_ERROR;
}

ushort
NBNameService::NewTransid(void)
{
	return (ushort) rand();
}


status_t
NBNameService::SearchCache(NBName &name, NLAddress &addr)
{
	for(int i=0;i<CacheSize;i++) {
		if (cache[i].name == name) {
			addr = cache[i].addr;
			return B_OK;
		}
	}
	return ENOENT;
}

void
NBNameService::AddCache(NBName &name, NLAddress &addr)
{
	static int32 phase = 0;
	int32 current;
	current = 1 + atomic_add(&phase,1);
	cache[current % CacheSize].name = name;
	cache[current % CacheSize].addr = addr;
	return;
}
