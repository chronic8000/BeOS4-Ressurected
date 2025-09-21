#include "cifs_globals.h"
#include "packet.h"
#include "netio.h"

// Forwards
status_t worker_func(void *_vol);
status_t start_worker(nspace *vol);
status_t ping_server(nspace *vol);
status_t send_ping(nspace *vol, char *data);
status_t recv_ping(nspace *vol, char *data);


status_t
worker_func(void *_vol) {

	nspace	*vol = (nspace*)_vol;
	status_t result;
	
	const bigtime_t wait_period = 60 * 1000 * 1000;
	const bigtime_t little_interval = 3 * 1000 * 1000;
	bigtime_t interval;
	bigtime_t timeout = wait_period;
		
	sem_id my_sem = vol->worker_sem;
	
	int i = 0;
	// Do something.

	while (true) {
		
		i++;
		snooze(little_interval);
		if (acquire_sem(my_sem) != B_OK) {
			return B_OK;
		}
		
		//dprintf("worker is yawning...");
		
		if ((i * little_interval) <= timeout) {
			//dprintf(" more sleep (im at %d, waiting for %d)\n", i, (timeout / little_interval));
			release_sem(my_sem);
			continue;
		}
		
		//dprintf("worker getting UP!\n");		
		i = 0;
				
		LOCK_VOL(vol);


		// Snooze.
		// Have we trafficked in the last 6 minutes?
		// If not, send and recv a ping.

		interval = system_time() - vol->last_traffic_time;
		//dprintf("Its been (dec) %d usecs since last traffic\n", interval);
		if (interval >= wait_period) {
			
			//dprintf("worker : pinging server\n");
			result = ping_server(vol);

			if (result != B_OK) {
				//dprintf("PING FAILURE\n");
				// Modify socket state, begin
				// reconnect
				UNLOCK_VOL(vol);
				release_sem(vol->worker_sem);
				break;
			}
			vol->last_traffic_time = system_time();
			
		} else {
			timeout = wait_period - interval;
		}

		UNLOCK_VOL(vol);
		release_sem(my_sem);
	}

	// not reached
	return B_OK;
}



status_t
start_worker(nspace *vol) {

	char name[64];

	if (0) {
		vol->last_traffic_time = system_time();

		sprintf(name, "cifs wrkr %Lx", system_time()); 
		vol->worker_sem = create_sem(1,name);
		set_sem_owner (vol->worker_sem,B_SYSTEM_TEAM);
	
		vol->worker_thread = spawn_kernel_thread(worker_func, name, B_LOW_PRIORITY, (void*)vol); 
		return(resume_thread(vol->worker_thread));
	} else {
		return B_OK;
	}
}

status_t
ping_server(nspace *vol) {

	// Volume is locked upon entrance.
	status_t result;
	char *echo_this;

	//dprintf("inside ping_server : 1\n");

	
	//echo_this = (char*)malloc(32 * sizeof(char));
	MALLOC(echo_this, char*, 32);

	strcpy(echo_this, "Ping!");
	
	
	result = send_ping(vol, echo_this);
	if (result != B_OK)
		return result;

	//dprintf("inside ping_server : 2\n");
		
	result = recv_ping(vol, echo_this);
	
	//dprintf("inside ping_server : 3\n");

	//dprintf("ping server returning %d\n", result);

	FREE(echo_this);
	
	return result;

}

status_t
send_ping(nspace *vol, char *data) {

	status_t result;
	packet *pkt = NULL;
	
	//dprintf("inside send_ping\n");
	
	new_smb_packet(vol, & pkt, SMB_COM_ECHO);
	
	//dprintf("made the new packet\n");
	
	//dprintf("getsize reports %d\n", getSize(pkt));
	
	insert_uchar(pkt, 1); // WordCount
	insert_ushort(pkt, 1); // Echo back once
	insert_ushort(pkt, strlen(data) + 1); // ByteCount
	insert_string(pkt, data);

	//dprintf("going to send this packet:\n");

	//dump_packet(pkt);
	
	// Send
	result = SendSMBPacket(vol, pkt);
	
	
	free_packet(& pkt);

	//dprintf("returngin succesful\n");

	return B_OK;
}


status_t
recv_ping(nspace *vol, char *data) {
	
	status_t result;
	packet *pkt = NULL;
	char	tmp[64];
	
	uchar wordcount;
	ushort seqnum;
	ushort bytecount;
	
	// Recv
	
	result = RecvNBPacket(vol, & pkt, 10000); //timeout = 10 sec
	if (result != B_OK) {
		//dprintf("RecvNBPacket returning %d\n", result);
		return result;
	}
	
	result = PopSMBHeader(vol, pkt, SMB_COM_ECHO, false);
	if (result != B_OK) {
		//dprintf("PopSMBHeader returning %d\n", result);
		return result;
	}
	
	
	set_order(pkt, false);
	remove_uchar(pkt, & wordcount);
	if (wordcount != 1) {
		//dprintf("ping failure : wordcount is %d, expecting 1", wordcount);
		return B_ERROR;
	}
	remove_ushort(pkt, & seqnum);
	remove_ushort(pkt, & bytecount);
	if (bytecount != (strlen(data) + 1)) {
		//dprintf("ping failure : bytecount is %d, expecting %d\n", bytecount, (strlen(data) + 1));
		return B_ERROR;
	}

	
	remove_string(pkt, tmp, 64);
	if (strcmp(tmp, data) != 0) {
		//dprintf(" %s != %s : echo failure\n", tmp, data);
		return B_ERROR;
	}	
	
	//dprintf(" Echo success!\n");
	
	free_packet(& pkt);
	return B_OK;

}






