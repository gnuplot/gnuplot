TITLE	Corona graphics module
;	Colin Kelley
;	January 1987

include header.mac

if1
include lineproc.mac
endif


_text	segment

public	_GrInit,_GrReset,_GrOnly,_TxOnly,_GrandTx,_Cor_line,_Cor_mask

corpixel proc near
	ror word ptr linemask,1
	jc cont
	ret
cont:	push bp
	mov bp,sp
	push ax
	push bx
	push cx
	mov es,ScSeg
	shl bx,1			; y
	mov bx,word ptr LookUp[bx] ; bx has y mem address
	mov cl,al			; x
	and cl,7
	shr ax,1
	shr ax,1
	shr ax,1			; ax /= 8
	add bx,ax
	mov al,1
	shl al,cl			; al contains bit mask
	or byte ptr es:[bx],al
	pop cx
	pop bx
	pop ax
	pop bp
	ret

lineproc _Cor_line, corpixel

beginproc _GrInit
	push bp
	mov bp,sp
	push di
	mov ax, [bp+X]			; screen number (0 - 7)
	mov cl,11
	shl ax,cl			; multiply by 2048 to get segment
	mov ScSeg,ax			; save segment for later
	push ax
	mov es, ax
	xor ax,ax
	mov di,ax
	mov cx, 4000h
	cld
	rep stosw
	pop cx
	call near ptr GrAddr
	mov ax,es
	pop di
	pop bp
	ret
_GrInit	endp

beginproc _GrReset
	mov cx, 0
	call near ptr GrAddr
	ret
_GrReset endp

GrAddr	proc near
	mov dx,3b4h			; address of 6845
	mov al,0ch			; register 12
	out dx,al
	inc dx
	mov al,ch			; Graphics Segment High
	out dx,al
	dec dx
	mov al,0dh			; register 13
	out dx,al
	mov al,cl			; Graphics Segment Low
	inc dx
	out dx,al
	ret
GrAddr	endp

beginproc _GrOnly
	mov dx,3b8h
	mov al,0a0h
	out dx,al
	ret
_GrOnly	endp

beginproc _TxOnly
	mov dx,3b8h
	mov al,28h
	out dx,al
	ret
_TxOnly	endp

beginproc _GrandTx
	mov dx,3b8h
	mov al,0a8h
	out dx,al
	ret
_GrandTx endp

beginproc _Cor_mask
	push bp
	mov bp,sp
	mov ax,[bp+x]			; mask
	mov linemask,ax
	pop bp
	ret
_Cor_mask endp

corpixel endp

_text	ends
 

_data	segment
linemask dw -1
ScSeg	dw 0
_data	ends

const	segment

K	equ 1024

mem_mac	MACRO x
	dw x,2*K+x,4*K+x,6*K+x,8*K+x,10*K+x,12*K+x,14*K+x,16*K+x
	dw 18*K+x,20*K+x,22*K+x,24*K+x
	ENDM
LookUp	equ $
	mem_mac 0
	mem_mac 80
	mem_mac (80*2)
	mem_mac (80*3)
	mem_mac (80*4)
	mem_mac (80*5)
	mem_mac (80*6)
	mem_mac (80*7)
	mem_mac (80*8)
	mem_mac (80*9)
	mem_mac (80*10)
	mem_mac (80*11)
	mem_mac (80*12)
	mem_mac (80*13)
	mem_mac (80*14)
	mem_mac (80*15)
	mem_mac (80*16)
	mem_mac (80*17)
	mem_mac (80*18)
	mem_mac (80*19)
	mem_mac (80*20)
	mem_mac (80*21)
	mem_mac (80*22)
	mem_mac (80*23)
	mem_mac (80*24)

const	ends

	end
