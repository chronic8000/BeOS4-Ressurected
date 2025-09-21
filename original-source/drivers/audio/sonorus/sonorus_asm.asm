	BITS 32
	GLOBAL xfer_32_32 
	GLOBAL xfer_32_shr_32 
	GLOBAL xfer_32_shl_32

	SECTION .text

xfer_32_32:
	;save regs we trash
	push 	edi				
	push 	esi
	push    ecx
	
	;NOTE: CANNOT BE CALLED WITH COUNT 0
	
	;we are guaranteed to have 32bit data....
	mov		edi, [esp+16]	;destination
	mov		esi, [esp+20]	;source
	mov		ecx, [esp+24]	;count	(in samples)

	rep movsd

	pop     ecx
	pop 	esi
	pop 	edi
	ret	

xfer_32_shr_32:
	;save regs we trash
	push 	edi				
	push 	esi
	push    ecx
	
	;NOTE: CANNOT BE CALLED WITH COUNT 0
	
	;we are guaranteed to have 32bit data....
	mov		edi, [esp+16]	;destination
	mov		esi, [esp+20]	;source
	mov		ecx, [esp+24]	;count	(in samples)

.sloop:	
	mov		eax,[esi]
	add		esi,4
	shr		eax,8
	mov		[edi],eax
	add		edi,4
	dec		ecx
	jnz		.sloop	

	pop     ecx
	pop 	esi
	pop 	edi
	ret	

xfer_32_shl_32:
	;save regs we trash
	push 	edi				
	push 	esi
	push    ecx
	
	;NOTE: CANNOT BE CALLED WITH COUNT 0
	
	;we are guaranteed to have 32bit data....
	mov		edi, [esp+16]	;destination
	mov		esi, [esp+20]	;source
	mov		ecx, [esp+24]	;count	(in samples)

.sloop:	
	mov		eax,[esi]
	add		esi,4
	shl		eax,8
	mov		[edi],eax
	add		edi,4
	dec		ecx
	jnz		.sloop	

	pop     ecx
	pop 	esi
	pop 	edi
	ret	
