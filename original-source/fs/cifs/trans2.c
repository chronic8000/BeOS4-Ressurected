#include "cifs_globals.h"
#include "packet.h"
#include "netio.h"


status_t
send_trans2_req(nspace *vol, packet *Setup, packet *Param, packet *Data) {

	status_t result;
	packet *pkt = NULL;
	
	uchar			wordcount;
	const ushort 	MaxParameterCount = 20;
	const ushort	MaxDataCount = 65535;
	const ushort	MaxSetupRecvCount = 0;
	ushort			ParameterOffset;
	ushort			DataOffset;
	ushort			MaxSetupSendCount;
	ushort			bytecount;
	
	MaxSetupSendCount = (ushort)(getSize(Setup) / 2);
	ParameterOffset = 66 + (MaxSetupSendCount * 2);
	DataOffset = ParameterOffset + getSize(Param);
	
	
	result = new_smb_packet(vol, & pkt, SMB_COM_TRANSACTION2);
	if (result != B_OK) return result;
		
	wordcount = 14 + MaxSetupSendCount;
	insert_uchar(pkt, wordcount);  // wordcount
	
	insert_ushort(pkt,	(ushort) getSize(Param));
	if (Data != NULL)
		insert_ushort(pkt,	(ushort) getSize(Data));
	else
		insert_ushort(pkt, 0);
	insert_ushort(pkt, MaxParameterCount);
	insert_ushort(pkt, MaxDataCount);
	insert_ushort(pkt, MaxSetupRecvCount);
	
	insert_uchar(pkt,	0);//mbz
	insert_uchar(pkt,	0);// Flags : 0=dont disconnect
	insert_ulong(pkt, 	0);//Timeout 0=return asap
	insert_ushort(pkt,	0);//mbz
	insert_ushort(pkt, 	(ushort)getSize(Param));//parametercount
	insert_ushort(pkt, 	ParameterOffset);
	if (Data != NULL)
		insert_ushort(pkt,	(ushort)getSize(Data));//datacount
	else
		insert_ushort(pkt, 	0);
	insert_ushort(pkt,	DataOffset);
	insert_uchar(pkt,	MaxSetupSendCount);//setupcount (send)
	insert_uchar(pkt, 	0);//mbz
	insert_packet(pkt, 	Setup);
	
	bytecount = getSize(Param) + 3;
	if (Data != NULL)
		bytecount += getSize(Data);
	
	insert_ushort(pkt, bytecount);
	// Required padding, discovered through sniffing
	insert_uchar(pkt,	0);
	insert_uchar(pkt,	'D');
	insert_uchar(pkt,	' ');
	
	insert_packet(pkt,	Param);
	
	if (Data != NULL)
		insert_packet(pkt, Data);
	
	
	result = SendSMBPacket(vol, pkt);
	if (result != B_OK)
		DPRINTF(-1, ("Error on SendSMBPacket in send_trans2_req : %d %s\n", result, strerror(result)));
			
	free_packet(& pkt);
	
	return result;
}



status_t
recv_trans2_req(nspace *vol, packet **pSetup, packet **pParam, packet **pData) {
	
	status_t result;
	packet	*pkt=NULL;
	
	
	uchar		wordcount;
	ushort		bytecount;

	ushort		TotalParameterCount;
	ushort		TotalDataCount;
	ushort		dumbushort;
	ushort		ParameterCount;
	ushort		ParameterOffset;
	ushort		ParameterDisplacement;
	ushort		DataCount;
	ushort		DataOffset;
	ushort		DataDisplacement;
	uchar		SetupCount;
	uchar		dumbuchar;
		
	int32		param_rem=-1;
	int32		data_rem=-1;
	int32		skiplen;		

	if (pParam != NULL) {
		init_packet(pParam, -1);
		set_order(*pParam, false);
	}
	if (pData != NULL) {
		init_packet(pData, -1);
		set_order(*pData, false);
	}
	if (pSetup != NULL) {
		init_packet(pSetup, -1);
		set_order(*pSetup, false);
	}

	do {

		result = RecvSMBPacket(vol, & pkt, SMB_COM_TRANSACTION2);
		if (result != B_OK) {
			DPRINTF(-1, ("Failed on RecvSMBPacket in recv_trans2_req\n"));
			goto exit1;
		}
		remove_uchar(pkt, 	& wordcount);
		remove_ushort(pkt,	& TotalParameterCount);
		if (param_rem == -1) param_rem = TotalParameterCount;
		remove_ushort(pkt,	& TotalDataCount);
		if (data_rem == -1) data_rem = TotalDataCount;
		remove_ushort(pkt, 	& dumbushort);//mbz
		remove_ushort(pkt,	& ParameterCount);
		remove_ushort(pkt,	& ParameterOffset);
		remove_ushort(pkt,	& ParameterDisplacement);
		remove_ushort(pkt,	& DataCount);
		remove_ushort(pkt,	& DataOffset);
		remove_ushort(pkt,	& DataDisplacement);
	
		// Warn about doing multiple recieves:
		if (DataCount != TotalDataCount)
			DPRINTF(1, ("Data bytes: Recvd %d of %d total.\n", DataCount, TotalDataCount));
		if (ParameterCount != TotalParameterCount)
			DPRINTF(1, ("Parameter bytes: Recvd %d of %d total.\n", ParameterCount, TotalParameterCount));
	
			
		remove_uchar(pkt, 	& SetupCount);
		remove_uchar(pkt,	& dumbuchar);//mbz
		if (SetupCount != 0) {
			dprintf("SetupCount != 0\n");
		}
		
		remove_ushort(pkt, 	& bytecount);
		// Now you are at 55 bytes offset from the beginning
		// of the smb packet.

		if (ParameterCount != 0) {
			skiplen = ParameterCount + ParameterOffset - 55;
			skip_over_remove(pkt, (ParameterOffset - 55));
			result = remove_packet(pkt, *pParam, ParameterCount);
			if (result != B_OK) {
				DPRINTF(-1, ("problem getting params\n"));
				goto exit1;
			}
			param_rem -= ParameterCount;
		} else {
			skiplen = 0;
		}
		
		if (DataCount != 0) {
			skip_over_remove(pkt, (bytecount - DataCount - skiplen));
			//skip_over_remove(pkt, ( DataOffset - ParameterCount - ParameterOffset));
			result = remove_packet(pkt, *pData, DataCount);
			if (result != B_OK) {
				DPRINTF(-1, ("problem getting data\n"));
				goto exit1;
			}
			data_rem -= DataCount;
		}
	
		free_packet(& pkt);
		//dprintf("after free, pkt is %x\n", pkt);
	
		//dprintf("data_rem is %d param_rem is %d\n", data_rem, param_rem);

	} while ((data_rem != 0) || (param_rem != 0));	


	return B_OK;
	
	
exit1:
	DPRINTF(-1, ("Error in recv_trans2_req : %d %s\n", result, strerror(result)));
	if (pParam != NULL) free_packet(pParam);
	if (pData != NULL) free_packet(pData);
	if (pSetup != NULL) free_packet(pSetup);	
	return result;
}

