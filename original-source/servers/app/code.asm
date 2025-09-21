
;void Rotate4x4x16( void *src, int32 srcStride, void *dst, int32 dstStride )
ALIGN 16
GLOBAL Rotate4x4x16
Rotate4x4x16:
		mov ecx, [esp+4]
		mov edx, [esp+12]

		movq mm0, [ecx]
		add ecx, [esp + 8]
		movq mm1, [ecx]
		add ecx, [esp + 8]
		movq mm2, [ecx]
		add ecx, [esp + 8]
		movq mm3, [ecx]

		movq mm4, mm1
		punpcklwd mm4, mm0
		movq mm5, mm3
		punpcklwd mm5, mm2
		
		movq mm6, mm5
		punpckldq mm5, mm4
		punpckhdq mm6, mm4
		
		
		movq [edx], mm5
		add edx, [esp + 16]
		movq [edx], mm6
		add edx, [esp + 16]

		
		punpckhwd mm1, mm0
		punpckhwd mm3, mm2
		
		movq mm0, mm3
		punpckldq mm0, mm1
		punpckhdq mm3, mm1		

		movq [edx], mm0
		add edx, [esp + 16]
		movq [edx], mm3

		ret		

;void Combo4x4x16( void *src, int32 srcStride, void *dst, void *src2, void *dst2)
ALIGN 16
GLOBAL Combo4x4x16
Combo4x4x16:
		push ebx
		push esi
		push edi		;; args offset by +12
		
		mov ebx, [esp + 16 +12]
		mov esi, [esp + 20 +12]
		mov ecx, [esp + 4 +12]
		mov edx, [esp + 12 +12]

.loop
		movq mm7, [ebx]
		add ebx, 8
		
		movq mm0, [ecx]
		add ecx, [esp + 8 +12]
		movq mm1, [ecx]
		add ecx, [esp + 8 +12]
		movq mm2, [ecx]
		add ecx, [esp + 8 +12]
		movq mm3, [ecx]

		movq [esi], mm7
		add esi, 8
		movq mm7, [ebx]
		add ebx, 8
		
		movq mm4, mm1
		punpcklwd mm4, mm0
		movq mm5, mm3
		punpcklwd mm5, mm2
		
		movq mm6, mm5
		punpckldq mm5, mm4
		punpckhdq mm6, mm4
		
		movq [esi], mm7
		add esi, 8
		movq mm7, [ebx]
		add ebx, 8
				
		movq [edx], mm5
		add edx, 32
		movq [edx], mm6
		add edx, 32
		
		punpckhwd mm1, mm0
		punpckhwd mm3, mm2
		
		movq [esi], mm7
		add esi, 8
		movq mm7, [ebx]
		add ebx, 8
		
		movq mm0, mm3
		punpckldq mm0, mm1
		punpckhdq mm3, mm1		

		movq [edx], mm0
		add edx, 32
		movq [edx], mm3

		movq [esi], mm7
		add esi, 8

		pop edi
		pop esi
		pop ebx
		
		ret		

;void Combo4x4x16x16( void *src, int32 srcStride, void *dst, int32 dstStride, void *src2, void *dst2, void *prefetch);
ALIGN 16
GLOBAL Combo4x4x16x16
Combo4x4x16x16:
		push ebp
		push esi
		push edi
		push ebx
		push eax		;; args offset by +20
		
		mov ebx, [esp + 20 +20]
		mov eax, [esp + 24 +20]
		mov ecx, [esp + 4 +20]
		mov edx, [esp + 12 +20]
		mov ebp, 16

.loop
	mov esi, [esp + 28 +20]
	mov edi, [esi]
		movq mm7, [ebx]
		add ebx, 8
			movq mm0, [ecx]
			add ecx, [esp + 8 +20]
			movq mm1, [ecx]
			add ecx, [esp + 8 +20]
			movq mm2, [ecx]
			add ecx, [esp + 8 +20]
			movq mm3, [ecx]
		movq [eax], mm7
		add eax, 8
			movq mm4, mm1
		movq mm7, [ebx]
		add ebx, 8
			punpcklwd mm4, mm0
			movq mm5, mm3
			punpcklwd mm5, mm2
			movq mm6, mm5
			punpckldq mm5, mm4
			punpckhdq mm6, mm4
			movq [edx], mm5
		movq [eax], mm7
		add eax, 8
			movq [edx+32], mm6
		movq mm7, [ebx]
		add ebx, 8
			punpckhwd mm1, mm0
			punpckhwd mm3, mm2
			movq mm0, mm3
			punpckldq mm0, mm1
			punpckhdq mm3, mm1		
			movq [edx+64], mm0
			movq [edx+96], mm3
		movq [eax], mm7
		add eax, 8

;;			prefetch_src -= srcBytesPerRow;
;;			copy_src += 16*2;
;;			copy_dst += dstBytesPerRow;
;;			rotate_src += 4*srcBytesPerRow;
;;			rotate_dst -= 4*2;
;;			if ((count & 3) == 3) {
;;				rotate_src -= 16*srcBytesPerRow - 4*2;
;;				rotate_dst += 16*2*4 + 16*2;
;;			}

		movq mm7, [ebx]
		add ebx, 8
			mov esi, [esp + 8 +20]
			add ecx, esi
			sub [esp + 28 +20], esi		
			sub	edx, 8
		
			mov	edi, ebp		
			and edi, 3
		movq [eax], mm7
			add eax, [esp + 16 +20]
			cmp edi, 1
			jne .no_row
		
			shl esi, 4
			sub ecx, esi
			add ecx, 8
			add edx, 128+32
.no_row
			dec ebp
			jnz near .loop
		
		pop eax
		pop ebx
		pop edi
		pop esi
		pop ebp
		
		ret		

;				           4          8                12         16               20            24
; void RotateNxNx16( void *src, int32 srcStride, void *dst, int32 dstStride, int32 hcount, int32 vcount )
ALIGN 16
GLOBAL RotateNxNx16
RotateNxNx16:
		push ebp
		push esi
		push edi
		push ebx		;; args offset by +16


		xor ebx, ebx
		mov edi, [esp + 12 + 16]			; *dst

.outer_loop:
		mov esi, [esp + 4 + 16]			; *src
		lea esi, [esi + ebx*8]
		

		mov ecx, [esp + 20 + 16]		; hcount
		mov eax, [esp + 8 + 16]			; srcStride
		push edi
.inner_loop:
		movq mm0, [esi]
		add esi, eax
		movq mm1, [esi]
		add esi, eax
		movq mm2, [esi]
		add esi, eax
		movq mm3, [esi]
		add esi, eax

		movq mm4, mm1
		punpcklwd mm4, mm0
		movq mm5, mm3
		punpcklwd mm5, mm2
		
		movq mm6, mm5
		punpckldq mm5, mm4
		punpckhdq mm6, mm4

		mov ebp, edi		
		movq [ebp], mm5
		add ebp, [esp + 16 + 20]		; dstStride
		movq [ebp], mm6
		add ebp, [esp + 16 + 20]		; dstStride

		punpckhwd mm1, mm0
		punpckhwd mm3, mm2
		
		movq mm0, mm3
		punpckldq mm0, mm1
		punpckhdq mm3, mm1		

		movq [ebp], mm0
		add ebp, [esp + 16 + 20]		; dstStride
		movq [ebp], mm3
		sub edi, 8
		
		dec ecx
		jnz .inner_loop
		pop edi
		mov eax, [esp + 16 + 16]
		shl eax, 2
		add edi, eax
						
		inc ebx
		cmp ebx, [esp + 24 + 16]
		jne .outer_loop
		
		pop ebx
		pop edi
		pop esi
		pop ebp

		ret

;				           4          8                12         16               20            24
; void Best( void *src, int32 srcStride, void *dst, int32 dstStride, int32 hcount, int32 vcount )
ALIGN 16
GLOBAL Best
Best:
.loop
	mov esi, [esp + 28 +20]
	mov edi, [esi]
		movq mm7, [ebx]
		add ebx, 8
			movq mm0, [ecx]
			add ecx, [esp + 8 +20]
			movq mm1, [ecx]
			add ecx, [esp + 8 +20]
			movq mm2, [ecx]
			add ecx, [esp + 8 +20]
			movq mm3, [ecx]
		movq [eax], mm7
		add eax, 8
			movq mm4, mm1
		movq mm7, [ebx]
		add ebx, 8
			punpcklwd mm4, mm0
			movq mm5, mm3
			punpcklwd mm5, mm2
			movq mm6, mm5
			punpckldq mm5, mm4
			punpckhdq mm6, mm4
			movq [edx], mm5
		movq [eax], mm7
		add eax, 8
			movq [edx+32], mm6
		movq mm7, [ebx]
		add ebx, 8
			punpckhwd mm1, mm0
			punpckhwd mm3, mm2
			movq mm0, mm3
			punpckldq mm0, mm1
			punpckhdq mm3, mm1		
			movq [edx+64], mm0
			movq [edx+96], mm3
		movq [eax], mm7
		add eax, 8

;;			prefetch_src -= srcBytesPerRow;
;;			copy_src += 16*2;
;;			copy_dst += dstBytesPerRow;
;;			rotate_src += 4*srcBytesPerRow;
;;			rotate_dst -= 4*2;
;;			if ((count & 3) == 3) {
;;				rotate_src -= 16*srcBytesPerRow - 4*2;
;;				rotate_dst += 16*2*4 + 16*2;
;;			}

		movq mm7, [ebx]
		add ebx, 8
			mov esi, [esp + 8 +20]
			add ecx, esi
			sub [esp + 28 +20], esi		
			sub	edx, 8
		
			mov	edi, ebp		
			and edi, 3
		movq [eax], mm7
			add eax, [esp + 16 +20]
			cmp edi, 1
			jne .no_row
		
			shl esi, 4
			sub ecx, esi
			add ecx, 8
			add edx, 128+32
.no_row
			dec ebp
			jnz near .loop
		
			ret
			