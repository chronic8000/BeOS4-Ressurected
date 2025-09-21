/*
    EXTERNAL SOURCE RELEASE on 05/16/2000 - Subject to change without notice.

*/
/*
    Copyright 2000, Broadcom Corporation
    All Rights Reserved.
    
    This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
    the contents of this file may not be disclosed to third parties, copied or
    duplicated in any form, in whole or in part, without the prior written
    permission of Broadcom Corporation.
    

*/
/*
 * Common [OS-independent] Certification/Diagnostic ILCP protocol.
 *
 * Copyright(c) 1999 Broadcom, Inc.
 * $Id: ilc_cert.c,v 13.40 2000/05/05 00:53:52 henryp Exp $
 */

#ifdef IL_CERT

#include <bcmendian.h>
#include <proto/ethernet.h>
#include <proto/iline.h>
#include <proto/ilcp.h>
#include <bcm42xx.h>
#ifdef IL_STATS
#include <il_stats.h>
#endif
#include <ilc.h>

#include <il_export.h>

#define DUMPBUFSIZ	4096

#define UNSUP		-1

#define CSMLEN 10

#define SMLEN 12
#define DMLEN 8

#ifndef offsetof
#define offsetof(type, memb)	((uint)&((type *)0)->memb)
#endif

struct cstype_map {
	uint16 cstype;
	uint16 len;
	int ioctl;
};

struct cstype_map configm[] = {
	{CDCF_V0_CSTYPE_TXPE,			1,	ILCTXBPB},
	{CDCF_V0_CSTYPE_TXPRI,			1,	ILCTXPRI},
	{CDCF_V0_CSTYPE_LINKINT,		1,	ILCLINKINT},
	{CDCF_V0_CSTYPE_TXDOWN,			0,	ILCTXDOWN},
	{CDCF_V0_CSTYPE_TXINHIBIT,		0,	UNSUP},
	{CDCF_V0_CSTYPE_TXUP,			0,	UNSUP},
	{CDCF_V0_CSTYPE_HPNAMODE,		1,	ILCHPNAMODE},
	{CDCF_V0_CSTYPE_LARQ,			1,	ILCLARQONOFF},
	{CDCF_V0_CSTYPE_CSA,			1,	ILCCSA},
	{CDCF_V0_CSTYPE_CSAHPNAMODE,	1,	ILCCSAHPNAMODE},
};

struct stats_map {
	uint16 cstype;
	uint offset;
};

struct stats_map statsm[SMLEN] = {
	{CDCF_V0_CSTYPE_XMTFRAMES,	offsetof(struct cert, xmt_nframes)},
	{CDCF_V0_CSTYPE_XMTBYTES,	offsetof(struct cert, xmt_nbytes)},
	{CDCF_V0_CSTYPE_XMTERR,		offsetof(struct cert, xmt_err)},
	{CDCF_V0_CSTYPE_XMTETIME,	offsetof(struct cert, xmt_etime)},
	{CDCF_V0_CSTYPE_RCVFRAMES,	offsetof(struct cert, rcv_nframes)},
	{CDCF_V0_CSTYPE_RCVBYTES,	offsetof(struct cert, rcv_nbytes)},
	{CDCF_V0_CSTYPE_RCVSEQMISS,	offsetof(struct cert, rcv_seq_miss)},
	{CDCF_V0_CSTYPE_RCVSEQERR,	offsetof(struct cert, rcv_seq_err)},
	{CDCF_V0_CSTYPE_RCVDATAERR,	offsetof(struct cert, rcv_data_err)},
	{CDCF_V0_CSTYPE_RCVERR,		offsetof(struct cert, rcv_err)},
	{CDCF_V0_CSTYPE_RCVETIME,	offsetof(struct cert, rcv_etime)},
	{CDCF_V0_CSTYPE_RCVXSUMERR,	offsetof(struct cert, rcv_xsum_err)},
};


/* prototypes */
static uint ilc_sendcertctl(ilc_info_t *ilc, struct ether_addr *dstid,
	uint8 op, uint16 dsid, uint info, uint16 seq, uint len, uchar *buf);
static uint ilc_sendcertdata(ilc_info_t *ilc, uint16 seq);


static int32
cert_validate_conf(ilcp_v0_cds_t *pcs, uint16 len, int32 *val) 
{
	int32 error = 0;
	if (ntoh16(pcs->CSLength) != len) {
		error = CDCF_V0_ERR_INVALID_PARAM;
		IL_ERROR(("ilc_cert: invalid param for cstype %d\n",
			hton16(pcs->CSType)));
	}
	ASSERT((len ==0) || (len == 1) || (len == 2) || (len == 4));
	switch (len) {
	case 0:
		*val = 0;
		break;
	case 1:
		*val = (int8) pcs->CSData.D1.CSData1;
		break;
	case 2:
		*val = (int16) ntoh16(pcs->CSData.D2.CSData2);
		break;
	case 4:
		*val = (int32) ntoh32(pcs->CSData.D4.CSData4);
		break;
	}
	return(error);
}

static void
ilc_cert_stop_send(ilc_info_t *ilc)
{
		struct cert *cert = &ilc->cert;
		bcm42xxregs_t *regs = ilc->regs;
		int32 val = R_REG(&regs->devcontrol);

		val &= ~DC_FG;
		W_REG(&regs->devcontrol, val);
		W_REG(&regs->gptimer, 0);

		cert->send_dseq = cert->send_cseq = cert->send_numend = 0;

		cert->send_length = cert->send_dgen = cert->send_dseed = 0;
		cert->send_dxsum = cert->send_tgen = cert->send_tseed = 0;
		cert->send_msec = cert->send_nburst = cert->send_npkts = 0;
		cert->send_length = cert->send_txpri = cert->send_numacks = 0;
		cert->send_numaddr = 0;
		cert->send_txpe = -1;

		return;
}

static uchar*
ilc_cert_malloc_buf(ilc_info_t *ilc)
{
	uchar *buf;
	if ((buf = (uchar *)il_malloc(DUMPBUFSIZ)) == NULL) {
		IL_ERROR(("il%d: ilc_cert_malloc_buf: memory allocation error\n",
			ilc->unit));
		return(NULL);
	}
	return(buf);
}

void
ilc_recvcert(ilc_info_t *ilc, struct ether_header *eh)
{
	uint ackdone = 0, len = 0;
	int32 error = 0;
	uchar *buf = NULL;
	uint8 op;
	uint16 seq, csl, dsid = 0;
	int32 val;
	bcm42xxregs_t *regs = ilc->regs;
	ilcp_v0_cdcf_t *pcdcf;
	ilcp_v0_cds_t *pcs, cds;
	int length;
	uint k, numacks;
	struct cert *cert;

	cert = &ilc->cert;
	pcdcf = (ilcp_v0_cdcf_t *) &eh[1];

	ASSERT(ISCERT(eh));

	/*
	 * If interface is in promiscuous mode, we can get cert frames that
	 * belong to other stations.  Claim them as cert but don't act on them.
	 */
	if (bcmp(eh->ether_dhost, &ilc->cur_etheraddr, ETHER_ADDR_LEN))
		return;

	IL_TRACE(("il%d: ilc_recvcert\n", ilc->unit));

	op = pcdcf->OpCode;
    seq = ntoh16(pcdcf->Cert_Seq_Tag);

	if (op == CDCF_V0_OPCODE_ENABLE) {
		/* If we're already enabled, return an error to let the controller
			know it's a dup enable */
		if (cert->enabled != 0)
			error = CDCF_V0_ERR_INVALID_OP;
		cert->enabled = 1;
		goto parse_done;
	} else if (op == CDCF_V0_OPCODE_DISABLE) {
		ilc_cert_stop_send(ilc);
		cert->enabled = 0;
	}

	if (!cert->enabled)
		return;

	switch (op) {
	case CDCF_V0_OPCODE_OK:
	case CDCF_V0_OPCODE_ERROR:
		/* don't do anything with reply messages */
		ackdone = 1;
		break;

	case CDCF_V0_OPCODE_CONFIGNODE:
		/* total length of command segments */
		length = ntoh16(pcdcf->h.length) - (sizeof(ilcp_v0_cdcf_t) -
			sizeof(ilcp_v0_cds_t) - 4);
		pcs = &pcdcf->CommandSeg;
		while ((length > 0) && !error) {

			/* find the index into configm */
			for (k = 0; k < CSMLEN; k++)
				if (configm[k].cstype == ntoh16(pcs->CSType))
					break;

			if (k == CSMLEN) {
				error = CDCF_V0_ERR_UNSUP_CMDSEG;
				break;
			}

			/* validate the data */
			if ((error = cert_validate_conf(pcs, configm[k].len, &val)) != 0)
				break;

			/* special cases that need to be handled before calling
				ilc_ioctl() */
			switch (configm[k].cstype) {
			case CDCF_V0_CSTYPE_TXPE:
				if ((val >= (ILINE_MIN_BPB-1)) && (val <= (ILINE_MAX_BPB-1)))
					val += 1;
				break;

			case CDCF_V0_CSTYPE_TXDOWN:
		        cds.CSLength = hton16(CDCF_V0_CSLEN_NULL);
		        cds.CSType = hton16(CDCF_V0_CSTYPE_NULL);
		        cds.CSData.D4.CSData4 = 0;
				ilc_sendcertctl(ilc, (struct ether_addr *)&eh->ether_shost,
					CDCF_V0_OPCODE_OK, 0, 0, seq, sizeof(cds),
					(uchar *)&cds);

				ackdone = 1;
				break;
			} /* switch (configm[k].cstype) */

			switch (configm[k].ioctl) {
			case NULL:
				break;

			case UNSUP:
				error = CDCF_V0_ERR_UNSUP_CMDSEG;
				break;

			default:
				if (ilc_ioctl(ilc, configm[k].ioctl, &val))
					error = CDCF_V0_ERR_INVALID_PARAM;
				break;
			} /* switch (configm[k].ioctl) */

			/* command segments are padded to 4 byte multiples */
			csl = (ntoh16(pcs->CSLength) + 4 + 3) & ~3;
			(uchar *)pcs += csl;
			length -= csl;
			if (length < 0)
				error = CDCF_V0_ERR_INVALID_FRAME;

		} /* while */

		break;

	case CDCF_V0_OPCODE_CONFIGRECV:
		/* reset receive stats */
		cert->rcv_seq_exp = 0;
		cert->rcv_nframes = cert->rcv_nbytes = cert->rcv_seq_miss = 0;
		cert->rcv_seq_err = cert->rcv_data_err = cert->rcv_err = 0;
		cert->rcv_etime = cert->rcv_xsum_err = 0;

		/* configure receive data validation functions */

		break;

	case CDCF_V0_OPCODE_STOPRECV:
		error = CDCF_V0_ERR_UNSUP_OP;
		break;

	case CDCF_V0_OPCODE_CONFIGSEND:
		/* reset xmt stats */
		cert->xmt_nframes = cert->xmt_nbytes = cert->xmt_err = 0;
		cert->xmt_etime = 0;

		cert->send_dseq = cert->send_numend = 0;
		cert->send_cseq = seq;

		cert->send_dgen = cert->send_dseed = cert->send_dxsum = 0;
		cert->send_tgen = cert->send_tseed = cert->send_msec = 0;
		cert->send_nburst = cert->send_npkts = cert->send_length = 0;
		cert->send_txpri = cert->send_numaddr = 0;
		cert->send_txpe = -1;
		cert->send_numacks = 1;

		cert->send_ca = *(struct ether_addr *)&eh->ether_shost;

		/* total length of command segments */
		length = ntoh16(pcdcf->h.length) - (sizeof(ilcp_v0_cdcf_t) -
			sizeof(ilcp_v0_cds_t) - 4);
		pcs = &pcdcf->CommandSeg;
		while ((length > 0) && !error) {
			switch (ntoh16(pcs->CSType)) {

			case CDCF_V0_CSTYPE_SA:
				if (ntoh16(pcs->CSLength) != 6)
					error = CDCF_V0_ERR_INVALID_PARAM;
				else
					cert->send_sa = *(struct ether_addr *)&eh->ether_shost;
				break;

			case CDCF_V0_CSTYPE_NUM_DA:
				error = cert_validate_conf(pcs, 1, &val);
				cert->send_numaddr = (uint8) val;
				k = 0;
				while ((k < cert->send_numaddr) && !error) {
					/* command segments are padded to 4 byte multiples */
					csl = (ntoh16(pcs->CSLength) + 4 + 3) & ~3;
					(uchar *)pcs += csl;
					length -= csl;
					if (ntoh16(pcs->CSType) != CDCF_V0_CSTYPE_DA)
						error = CDCF_V0_ERR_INVALID_FRAME;
					else if (ntoh16(pcs->CSLength) != 6)
						error = CDCF_V0_ERR_INVALID_FRAME;
					else if (length < 0)
						error = CDCF_V0_ERR_INVALID_FRAME;
					else
						cert->send_da = *(struct ether_addr *)&pcs->CSData;
					k++;
				}
				break;

			case CDCF_V0_CSTYPE_DA:
				/* DA not preceeded by NUM_DA is not allowed */
				error = CDCF_V0_ERR_INVALID_FRAME;
				break;

			case CDCF_V0_CSTYPE_DGEN_TYPE:
				error = cert_validate_conf(pcs, 4, &val);
				cert->send_dgen = (uint32) val;
				break;

			case CDCF_V0_CSTYPE_DGEN_DATA:
				error = cert_validate_conf(pcs, 4, &val);
				cert->send_dseed = (uint32) val;
				break;

			case CDCF_V0_CSTYPE_LENGTH:
				error = cert_validate_conf(pcs, 2, &val);
				cert->send_length = (uint16) val;
				break;

			case CDCF_V0_CSTYPE_NPKTS:
				error = cert_validate_conf(pcs, 4, &val);
				cert->send_npkts = (uint32) val;
				break;

			case CDCF_V0_CSTYPE_BURST_INT:
				error = cert_validate_conf(pcs, 2, &val);
				cert->send_msec = (uint16) val;
				break;

			case CDCF_V0_CSTYPE_BURST_NPKTS:
				error = cert_validate_conf(pcs, 2, &val);
				cert->send_nburst = (uint16) val;
				break;

			case CDCF_V0_CSTYPE_NUMACKS:
				error = cert_validate_conf(pcs, 1, &val);
				cert->send_numacks = (uint8) val;
				break;

			case CDCF_V0_CSTYPE_TGEN_TYPE:
				error = cert_validate_conf(pcs, 4, &val);
				cert->send_tgen = (uint32) val;
				break;

			case CDCF_V0_CSTYPE_TGEN_DATA:
				error = cert_validate_conf(pcs, 4, &val);
				cert->send_tseed = (uint32) val;
				break;

			case CDCF_V0_CSTYPE_DATA_XSUM:
				error = cert_validate_conf(pcs, 1, &val);
				cert->send_dxsum = (uint8) val;
				break;
	
			case CDCF_V0_CSTYPE_TXPE:
				error = cert_validate_conf(pcs, 1, &val);
				cert->send_txpe = (int8) val;
				break;
	
			case CDCF_V0_CSTYPE_TXPRI:
				error = cert_validate_conf(pcs, 1, &val);
				cert->send_txpri = (int8) val;
				break;

			} /* switch (pcs->CSType) */

			/* command segments are padded to 4 byte multiples */
			csl = (ntoh16(pcs->CSLength) + 4 + 3) & ~3;
			(uchar *)pcs += csl;
			length -= csl;
			if (length < 0)
				error = CDCF_V0_ERR_INVALID_FRAME;

		}
		break;

	case CDCF_V0_OPCODE_STARTSEND:
		OR_REG(&regs->devcontrol, DC_FG);
		W_REG(&regs->gptimer, CERT_TO_MSEC * cert->send_msec);
		break;

	case CDCF_V0_OPCODE_STOPSEND:
		ilc_cert_stop_send(ilc);
		break;

	case CDCF_V0_OPCODE_ECHOREQUEST:
		if (ntoh16(pcdcf->CommandSeg.CSType) != CDCF_V0_CSTYPE_DATA) {
			error = CDCF_V0_ERR_INVALID_FRAME;
			break;
		}
		if ((buf = ilc_cert_malloc_buf(ilc)) == NULL) {
			error = CDCF_V0_ERR_UNK;
			break;
		}
		len = ntoh16(pcdcf->CommandSeg.CSLength) + 4;
		bcopy((uchar*)&pcdcf->CommandSeg, buf, len);
		break;

	case CDCF_V0_OPCODE_DUMPSTATS:
		if ((buf = ilc_cert_malloc_buf(ilc)) == NULL) {
			error = CDCF_V0_ERR_UNK;
			break;
		}

		pcs = (ilcp_v0_cds_t *)buf;
		for (k = 0; k < SMLEN; k++) {
			pcs->CSLength = hton16(4);
			pcs->CSType = hton16(statsm[k].cstype);
			pcs->CSData.D4.CSData4 =
				hton32(*(uint32 *)((uint8 *)cert + statsm[k].offset));
			pcs++;
		}

		len =  SMLEN * sizeof(ilcp_v0_cds_t);
		break;

	case CDCF_V0_OPCODE_DUMPCONFIG:
		if ((buf = ilc_cert_malloc_buf(ilc)) == NULL) {
			error = CDCF_V0_ERR_UNK;
			break;
		}

		pcs = (ilcp_v0_cds_t *)buf;
		pcs->CSLength = hton16(4);
		pcs->CSType = hton16(CDCF_V0_CSTYPE_TXPE);
		/* convert BPB to PE */
		if ((ilc->txbpb >= ILINE_MIN_BPB) && (ilc->txbpb <= ILINE_MAX_BPB))
			pcs->CSData.D4.CSData4 = hton32((uint32)(ilc->txbpb - 1));
		else
			pcs->CSData.D4.CSData4 = hton32((uint32)ilc->txbpb);

		pcs++;
		pcs->CSLength = hton16(4);
		pcs->CSType = hton16(CDCF_V0_CSTYPE_TXPRI);
		pcs->CSData.D4.CSData4 = hton32((uint32)ilc->txpri);

		pcs++;
		pcs->CSLength = hton16(4);
		pcs->CSType = hton16(CDCF_V0_CSTYPE_LINKINT);
		pcs->CSData.D4.CSData4 = hton32((uint32)ilc->linkint);

		pcs++;
		pcs->CSLength = hton16(4);
		pcs->CSType = hton16(CDCF_V0_CSTYPE_TXINHIBIT);
		pcs->CSData.D4.CSData4 = 0;

		pcs++;
		pcs->CSLength = hton16(4);
		pcs->CSType = hton16(CDCF_V0_CSTYPE_HPNAMODE);
		pcs->CSData.D4.CSData4 = hton32(ilc->configmode);

		pcs++;
		pcs->CSLength = hton16(4);
		pcs->CSType = hton16(CDCF_V0_CSTYPE_LARQ);
		pcs->CSData.D4.CSData4 = hton32(ilc->larqlevel);

		pcs++;
		pcs->CSLength = hton16(4);
		pcs->CSType = hton16(CDCF_V0_CSTYPE_CSA);
		pcs->CSData.D4.CSData4 = hton32((uint32)ilc->csa);

		pcs++;
		pcs->CSLength = hton16(4);
		pcs->CSType = hton16(CDCF_V0_CSTYPE_CSAHPNAMODE);
		pcs->CSData.D4.CSData4 = hton32(ilc->csa_configmode);

		len =  8 * sizeof(ilcp_v0_cds_t);
		break;

	case CDCF_V0_OPCODE_TESTDATA:
		/* data frame processing */
		if (seq < cert->rcv_seq_exp) {
			cert->rcv_seq_err++;
		} else {
			cert->rcv_nframes++;
			if (seq > cert->rcv_seq_exp) {
				cert->rcv_seq_miss += (seq - cert->rcv_seq_exp);
				cert->rcv_seq_exp = seq + 1;
			} else {
				cert->rcv_seq_exp++;
			}
		}

		/* FIXME: need to add the following counters:
			rcv_nbytes
			rcv_data_err
			rcv_err
			rcv_etime
			rcv_xsum_err */

		ackdone = 1;
		break;

	case CDCF_V0_OPCODE_DUMP:
		if ((buf = ilc_cert_malloc_buf(ilc)) == NULL) {
			error = CDCF_V0_ERR_UNK;
			break;
		}
		if (ilc_ioctl(ilc, ILCDUMP, buf))
			error = CDCF_V0_ERR_UNK;
		len = strlen(buf);
		dsid = CDCF_V0_CSTYPE_TEXT;
		break;

	
	default:
		error = CDCF_V0_ERR_UNSUP_OP;
	}

parse_done:
	if (error) {
		/* FIXME: what's the right behavior here if ackdone is set? */
        cds.CSLength = hton16(CDCF_V0_CSLEN_ERROR);
        cds.CSType = hton16(CDCF_V0_CSTYPE_ERROR);
        cds.CSData.D4.CSData4 = hton32((uint32)error);
		ilc_sendcertctl(ilc, (struct ether_addr *)&eh->ether_shost,
			CDCF_V0_OPCODE_ERROR, 0, 0, seq, sizeof(cds), (uchar *)&cds);
	} else if (len) {
		/* have some data to return */
		ASSERT(buf);
		ilc_sendcertctl(ilc, (struct ether_addr *)&eh->ether_shost,
		    CDCF_V0_OPCODE_OK, dsid, 1, seq, len, buf);
	} else {
		if (!ackdone) {
			if (cert->send_numacks)
				numacks = cert->send_numacks;
			else
				numacks = 1;

	        cds.CSLength = hton16(CDCF_V0_CSLEN_NULL);
	        cds.CSType = hton16(CDCF_V0_CSTYPE_NULL);
	        cds.CSData.D4.CSData4 = 0;
			for (k=0; k<numacks; k++) {
				ilc_sendcertctl(ilc, (struct ether_addr *)&eh->ether_shost,
					CDCF_V0_OPCODE_OK, 0, 0, seq, sizeof(cds), (uchar *)&cds);
			}
		}
	}

	if (buf)
		il_mfree(buf, DUMPBUFSIZ);
}

/* if dsid is 0, expects buf to have appropriate data segment header. 
   if dsid is non-zero, adds ds header.  does all fragmentation
   and adds INFOREPLY ds
	returns:
	0	no errors are encountered
	1	'soft' error (e.g. transmit queue is full or out of xmt buffers,
		presumed to be temporary problems)
*/
static uint
ilc_sendcertctl(ilc_info_t *ilc, struct ether_addr *dstid,
	uint8 op, uint16 dsid, uint info, uint16 seq, uint len, uchar *buf)
{
	void *p;
	uint16 *nep;
	struct ether_header *eh;
	ilcp_v0_cdcf_t *pcdcf;
	ilcp_v0_cds_t *pcs;
	int k, nmtu, npkts, dlen;
	uint size, res;

	IL_TRACE(("il%d: ilc_sendcertctl\n", ilc->unit));

	/* do we need to fragment? */
	dlen = (ETHER_MAX_LEN - (sizeof(struct ether_header) +
		sizeof(ilcp_v0_cdcf_t) + ETHER_CRC_LEN)) & ~0x07;
	size = sizeof (struct ether_header) + sizeof (ilcp_v0_cdcf_t) + dlen;
	if (dsid)
		dlen -= (sizeof(ilcp_v0_cds_t) - 4);
	nmtu = len / dlen;
	npkts = nmtu;
	if (len % dlen) npkts++;

	IL_TRACE(("il%d: ilc_sendcertctl: len %d, nmtu %d, npkts %d\n",
		ilc->unit, len, nmtu, npkts));

	if (npkts > 256) {
		IL_ERROR(("il%d: ilc_sendcertctl: too many frames to send\n",
			ilc->unit));
		return(2);
	}

	for (k=0; k < nmtu; k++) {
		if ((p = il_pget((void*) ilc, TRUE, size, (uchar**) &eh, NULL)) == NULL) {
			IL_TRACE(("il%d: ilc_sendcertctl: out of packets\n", ilc->unit));
			COUNTER(ilc, IL_STATS_XMT_ERR_COUNT)++;
			COUNTER(ilc, IL_STATS_XMT_MISSED_ERR_COUNT)++;
			return(1);
		}

		/* fill in ether_header */
		EHINIT(eh, &ilc->cur_etheraddr, dstid, ETHERTYPE_ILCP);
	
		/* fill in cert frame */
		pcdcf = (ilcp_v0_cdcf_t *) &eh[1];
		/* make sure all pad and reserved bits are zero */
		bzero(pcdcf, sizeof(ilcp_v0_cdcf_t));
		pcdcf->h.subtype16 = hton16(ILCP_SUBTYPE_CERT);
		pcdcf->LSVersion = 0;
		pcdcf->OpCode = op;
		pcdcf->Cert_Xsum = 0;
		pcdcf->Cert_Seq_Tag = hton16(seq);

		pcs = (ilcp_v0_cds_t *)&(pcdcf->CommandSeg);
		pcs->CSLength = hton16(CDCF_V0_CSLEN_INFOREPLY);
		pcs->CSType = hton16(CDCF_V0_CSTYPE_INFOREPLY);
		pcs->CSData.D2.CSData2 = hton16((uint16)(((npkts-1) << 8) | k));
		pcs->CSData.D2.CSPad2 = 0;

		pcs++;
		if (dsid) {
			pcdcf->h.length = hton16((uint16)(CDCF_V0_MIN_LEN + dlen + 4));
			pcs->CSLength = hton16((uint16)dlen);
			pcs->CSType = hton16(dsid);
			bcopy(buf, &pcs->CSData, dlen);
			nep = (uint16 *)((uchar *)(&pcs->CSData) + dlen);
		} else {
			pcdcf->h.length = hton16((uint16)(CDCF_V0_MIN_LEN + dlen));
			bcopy(buf, pcs, dlen);
			nep = (uint16 *)((uchar *)pcs + dlen);
		}
		*nep = 0;

		buf += dlen;
		len -= dlen;

		/* send reply frame with pri=7, negotiated bpb */
		if ((res = il_sendctl((void*) ilc, p, 7, -1)) != 0) {
			IL_TRACE(("il%d: ilc_sendcertctl: il_sendctl returned error\n",
				ilc->unit));
			COUNTER(ilc, IL_STATS_XMT_ERR_COUNT)++;
			COUNTER(ilc, IL_STATS_XMT_MISSED_ERR_COUNT)++;
			return(res);
		}
	}

	size = sizeof (struct ether_header) + sizeof (ilcp_v0_cdcf_t) + len;
	if (dsid)
		size += (sizeof(ilcp_v0_cds_t) - 4);

	if ((p = il_pget((void*)ilc, TRUE, size, (uchar**) &eh, NULL)) == NULL) {
		IL_TRACE(("il%d: ilc_sendcertctl: out of packets\n", ilc->unit));
		COUNTER(ilc, IL_STATS_XMT_ERR_COUNT)++;
		COUNTER(ilc, IL_STATS_XMT_MISSED_ERR_COUNT)++;
		return(1);
	}

	/* fill in ether_header */
	EHINIT(eh, &ilc->cur_etheraddr, dstid, ETHERTYPE_ILCP);

	/* fill in cert frame */
	pcdcf = (ilcp_v0_cdcf_t *) &eh[1];
	/* make sure all pad and reserved bits are zero */
	bzero(pcdcf, sizeof(ilcp_v0_cdcf_t));
	pcdcf->h.subtype16 = hton16(ILCP_SUBTYPE_CERT);
	pcdcf->LSVersion = 0;
	pcdcf->OpCode = op;
	pcdcf->Cert_Xsum = 0;
	pcdcf->Cert_Seq_Tag = hton16(seq);

	pcs = (ilcp_v0_cds_t *)&(pcdcf->CommandSeg);

	if (info) {
		pcs->CSLength = hton16(CDCF_V0_CSLEN_INFOREPLY);
		pcs->CSType = hton16(CDCF_V0_CSTYPE_INFOREPLY);
		pcs->CSData.D2.CSData2 = hton16((uint16)(((npkts-1) << 8) | (npkts-1)));
		pcs->CSData.D2.CSPad2 = 0;
		pcs++;
	}

	if (dsid) {
		if (info)
			pcdcf->h.length = hton16((uint16)(CDCF_V0_MIN_LEN + len + 4));
		else
			pcdcf->h.length = hton16((uint16)(CDCF_V0_MIN_LEN + len - 4));
		pcs->CSLength = hton16((uint16)len);
		pcs->CSType = hton16(dsid);
		bcopy(buf, &pcs->CSData, len);
		nep = (uint16 *)((uchar *)(&pcs->CSData) + len);
	} else {
		if (info)
			pcdcf->h.length = hton16((uint16)(CDCF_V0_MIN_LEN + len));
		else
			pcdcf->h.length = hton16((uint16)(CDCF_V0_MIN_LEN + len - 8));
		bcopy(buf, pcs, len);
		nep = (uint16 *)((uchar *)pcs + len);
	}
	*nep = 0;

	/* send reply frame with pri=7, negotiated bpb */
	if ((res = il_sendctl((void*) ilc, p, 7, -1)) != 0) {
		IL_TRACE(("il%d: ilc_sendcertctl: il_sendctl returned error\n",
			ilc->unit));
		COUNTER(ilc, IL_STATS_XMT_ERR_COUNT)++;
		COUNTER(ilc, IL_STATS_XMT_MISSED_ERR_COUNT)++;
		return(res);
	}

	return(0);
}

/* sends a cert data frame.
	returns:
	0	no errors are encountered
	1	'soft' error (e.g. transmit queue is full or out of xmt buffers,
		presumed to be temporary problems)
	2	'hard' error (not used yet)
*/
static uint
ilc_sendcertdata(ilc_info_t *ilc, uint16 seq)
{
	struct cert *cert = &ilc->cert;
	void *p;
	struct ether_header *eh;
	ilcp_v0_cdcf_t *pcdcf;
	uchar *pdata;
	uint size;

	IL_TRACE(("il%d: ilc_sendcertdata\n", ilc->unit));

	size = sizeof (struct ether_header) + (sizeof (ilcp_v0_cdcf_t) - 4) +
		cert->send_length;

	if ((p = il_pget((void*) ilc, TRUE, size, (uchar**) &eh, NULL)) == NULL) {
		IL_TRACE(("il%d: ilc_sendcertdata: out of packets\n", ilc->unit));
		COUNTER(ilc, IL_STATS_XMT_ERR_COUNT)++;
		COUNTER(ilc, IL_STATS_XMT_MISSED_ERR_COUNT)++;
		return(1);
	}

	/* fill in ether_header */
	EHINIT(eh, &ilc->cur_etheraddr, &cert->send_da, ETHERTYPE_ILCP);

	/* fill in cert frame */
	pcdcf = (ilcp_v0_cdcf_t *) &eh[1];
	/* make sure all pad and reserved bits are zero */
	bzero(pcdcf, sizeof(ilcp_v0_cdcf_t));
	pcdcf->h.subtype16 = hton16(ILCP_SUBTYPE_CERT);
	pcdcf->h.length = hton16((uint16)(cert->send_length +
		(CDCF_V0_MIN_LEN - 4)));
	pcdcf->LSVersion = 0;
	pcdcf->OpCode = CDCF_V0_OPCODE_TESTDATA;
	pcdcf->Cert_Xsum = 0;
	pcdcf->Cert_Seq_Tag = hton16(seq);

	pcdcf->CommandSeg.CSLength = hton16(cert->send_length);
	pcdcf->CommandSeg.CSType = CDCF_V0_CSTYPE_DATA;

    pdata = (uint8 *)&(pcdcf->CommandSeg.CSData);
	/* FIXME: need to fill in valid data */
	/* zero out data and next ethertype fields */
    bzero(pdata, cert->send_length + ETHER_TYPE_LEN);


	/* send reply frame with requested priority and bpb*/
	if ((cert->send_txpe >= (ILINE_MIN_BPB-1)) &&
		(cert->send_txpe <= (ILINE_MAX_BPB-1))) {
		if (il_sendctl((void*) ilc, p, cert->send_txpri, cert->send_txpe + 1)) {
			IL_TRACE(("il%d: ilc_sendcertctl: il_sendctl returned error\n",
				ilc->unit));
			COUNTER(ilc, IL_STATS_XMT_ERR_COUNT)++;
			COUNTER(ilc, IL_STATS_XMT_MISSED_ERR_COUNT)++;
			return(1);
		}
	} else {
		if (il_sendctl((void*) ilc, p, cert->send_txpri, cert->send_txpe)) {
			IL_TRACE(("il%d: ilc_sendcertctl: il_sendctl returned error\n",
				ilc->unit));
			COUNTER(ilc, IL_STATS_XMT_ERR_COUNT)++;
			COUNTER(ilc, IL_STATS_XMT_MISSED_ERR_COUNT)++;
			return(1);
		}
	}

	return(0);
}

void
ilc_intrcert(ilc_info_t *ilc)
{
	struct cert *cert = &ilc->cert;
	uint res, nsent = 0;
	ilcp_v0_cds_t cds;

	if (!cert->enabled)
		return;

	if ((cert->send_npkts) &&
		(cert->xmt_nframes < cert->send_npkts)) {
		IL_TRACE(("il%d: ilc_intrcert: sending cert data frames\n",
			ilc->unit));
		/* send data frames */
		while ((nsent < cert->send_nburst) &&
			(cert->xmt_nframes < cert->send_npkts)) {
			res = ilc_sendcertdata(ilc, cert->send_dseq);
			if (res) {
				if (res == 2)
					cert->xmt_err++;
				IL_TRACE(("il%d: ilc_intrcert: ilc_sendcertdata returned %d\n",
					ilc->unit, res));
				return;
			}
			nsent++;
			cert->xmt_nframes++;
			cert->send_dseq++;
		}
	}
	if ((cert->send_numacks) &&
		(cert->send_numend < cert->send_numacks) &&
		(cert->xmt_nframes >= cert->send_npkts)) {
		/* send end frames */
		IL_TRACE(("il%d: ilc_intrcert: sending cert end frames\n",
			ilc->unit));
        cds.CSLength = hton16(CDCF_V0_CSLEN_NULL);
        cds.CSType = hton16(CDCF_V0_CSTYPE_NULL);
        cds.CSData.D4.CSData4 = 0;
		while ((nsent < cert->send_nburst) &&
			(cert->send_numend < cert->send_numacks)) {
			res = ilc_sendcertctl(ilc, (struct ether_addr *)&cert->send_da,
				CDCF_V0_OPCODE_OK, 0, 0, cert->send_dseq++, sizeof(cds),
				(uchar *)&cds);
			if (res) {
				if (res == 2)
					cert->xmt_err++;
				IL_TRACE(("il%d: ilc_intrcert: ilc_sendcertdata returned %d\n",
					ilc->unit, res));
				return;
			}
			cert->send_numend++;
			nsent++;
		}
	}
	if ((cert->send_numacks) &&
		(cert->send_numend >= cert->send_numacks) &&
		(cert->xmt_nframes >= cert->send_npkts)) {
		/* send ack frames */
		IL_TRACE(("il%d: ilc_intrcert: sending cert final ack frames\n",
			ilc->unit));
        cds.CSLength = hton16(CDCF_V0_CSLEN_NULL);
        cds.CSType = hton16(CDCF_V0_CSTYPE_NULL);
        cds.CSData.D4.CSData4 = 0;
		while ((nsent < cert->send_nburst) &&
			cert->send_numacks) {
			res = ilc_sendcertctl(ilc, (struct ether_addr *)&cert->send_ca,
				CDCF_V0_OPCODE_OK, 0, 0, cert->send_cseq, sizeof(cds),
				(uchar *)&cds);
			if (res) {
				if (res == 2)
					cert->xmt_err++;
				IL_TRACE(("il%d: ilc_intrcert: ilc_sendcertdata returned %d\n",
					ilc->unit, res));
				return;
			}
			cert->send_numacks--;
			nsent++;
		}
	}
	if (cert->send_npkts && !cert->send_numacks) {
		/* clear state */
		bcm42xxregs_t *regs;
		uint32 val;

		IL_TRACE(("il%d: ilc_intrcert: clearing send state\n",
			ilc->unit));
  
		regs = ilc->regs;
		val = R_REG(&regs->devcontrol);
		val &= ~DC_FG;
		W_REG(&regs->devcontrol, val);
		W_REG(&regs->gptimer, 0);

		cert->send_dseq = cert->send_cseq = cert->send_numend = 0;

		cert->send_dgen = cert->send_dseed = cert->send_dxsum = 0;
		cert->send_tgen = cert->send_tseed = cert->send_msec = 0;
		cert->send_nburst = cert->send_npkts = cert->send_length = 0;
		cert->send_txpri = cert->send_numacks = cert->send_numaddr = 0;
		cert->send_txpe = -1;
	}
}

#endif	/* IL_CERT */
