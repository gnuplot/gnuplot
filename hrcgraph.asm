TITLE	Hercules graphics module

;	Michael Gordon - 8-Dec-86
;
; Certain routines were taken from the Hercules BIOS of	Dave Tutelman - 8/86
; Others came from pcgraph.asm included in GNUPLOT by Colin Kelley
;
; modified slightly by Colin Kelley - 22-Dec-86
;	added header.mac, parameterized declarations
; added dgroup: in HVmodem to reach HCh_Parms and HGr_Parms - 30-Jan-87
; modified by Russell Lang 3 Jun 1988
;	added H_init

include header.mac

if1
include lineproc.mac
endif


GPg1_Base equ 0B800h	; Graphics page 1 base address

_text	segment

	public _H_line, _H_color, _H_mask, _HVmode, _H_puts
	public _H_init

HCfg_Switch equ	03BFH	; Configuration Switch - software switch 
			; to select graphics card memory map

beginproc _H_init
	mov al, 03H	; allow graphics in b8000:bffff
	mov dx, HCfg_Switch
	out dx, al
	ret
_H_init endp

hpixel	proc near
	ror word ptr bmask,1
	jc cont
	ret
cont:
	push ax
	push bx
	push cx
	push dx
	push si
	mov cx,ax		; x
	mov dx,bx		; y
;
; [couldn't this be done faster with a lookup table? -cdk]
;
	; first compute the address of byte to be modified
	; = 90*[row/4] + [col/8] + 2^D*[row/4] + 2^F*page
	mov	bh,cl		; col (low order) in BH
	mov	bl,dl		; row (low order) in BL
	and	bx,0703H	; mask the col & row remainders
IFDEF iAPX286
	shr	cx,3		; col / 8
	shr	dx,2		; row / 4
	mov	al,90
	mul	dx		; AX = 90*[ row/4 ]
	add	ax,cx		;  ... + col/8
	shl	bl,5		; align row remainder
ELSE			; same as above, obscure but fast for 8086
	shr	cx,1		; divide col by 8
	shr	cx,1
	shr	cx,1
	shr	dx,1		; divide row by 4
	shr	dx,1
	shl	dx,1		; begin fast multiply by 90 (1011010 B)
	mov	ax,dx
	shl	dx,1
	shl	dx,1
	add	ax,dx
	shl	dx,1
	add	ax,dx
	shl	dx,1
	shl	dx,1
	add	ax,dx		; end fast multiply by 90
	add	ax,cx		; add on the col/8
	shl	bl,1		; align row remainder
	shl	bl,1
	shl	bl,1
	shl	bl,1
	shl	bl,1
ENDIF
	add	ah,bl		; use aligned row remainder
end_adr_calc:			; address of byte is now in AX
	mov	dx,GPg1_Base	; base of pixel display to DX
	mov	es,dx		; ...and thence to segment reg
	mov	si,ax		; address of byte w/ pixel to index reg
	mov	cl,bh		; bit addr in byte
	mov	al,80H		; '1000 0000' in AL 
	shr	al,cl		; shift mask to line up with bit to read/write
set_pix:			; set the pixel
	or	es:[si],al	; or the mask with the right byte
	pop si
	pop dx
	pop cx
	pop bx
	pop ax
	ret
hpixel endp

lineproc _H_line, hpixel

;
; clear - clear page 1 of the screen buffer to zero (effectively, blank
;	the screen)
;
clear   proc near
	push es
	push ax
	push cx
	push di
	mov ax, GPg1_Base
	mov es, ax
	xor di, di
	mov cx, 4000h
	xor ax, ax
	cld
	rep stosw			; zero out screen page
	pop di
	pop cx
	pop ax
	pop es
	ret
clear	endp

beginproc _H_color
	push bp
	mov bp,sp
	mov al,[bp+X]			; color
	mov byte ptr color,al
	pop bp
	ret
_H_color endp

beginproc _H_mask
	push bp
	mov bp,sp
	mov ax,[bp+X]			; mask
	mov word ptr bmask,ax
	pop bp
	ret
_H_mask endp

HCtrl_Port	equ	03B8H	; Hercules 6845 control port IO addr
HIndx_Port	equ	03B4H	; Hercules 6845 index port IO addr
HScrn_Enable	equ	008h	; Control port bit to enable video
HCh_Mode	equ	020h	; Character output mode
HGr_Mode	equ	082h	; Graphics output mode page 1

parm_count equ 12

beginproc _HVmode
	push bp
	mov bp, sp
	push si
	mov ax, [bp+X]
	or ah, al
	mov al, HCh_Mode		; Assume character mode is wanted
	mov si, offset dgroup:HCh_Parms
	cmp ah, 0			; nonzero means switch to graphics
	jz vmode_ok
	call near ptr clear		; clear the graphics page
	mov al, HGr_Mode
	mov si, offset dgroup:HGr_Parms
vmode_ok:
	mov dx, HCtrl_Port
	out dx, al			; Set Hercules board to proper mode
	call near ptr setParms		; Set the 6845 parameters
	or al, HScrn_Enable		; Enable the video output
	out dx, al
	pop si
	pop bp
	ret
_HVmode	endp

setParms proc near		; Send 6845 parms to Hercules board
	push ax
	push dx
	push si			
	mov dx, HIndx_Port	; Index port addr -> DX
	mov ah, 0		; 0 -> parameter counter
sp_loop:
	mov al, ah
	out dx, al		; output to 6845 addr register
	inc dx			; next output to data register
	mov al, [si]		; next control byte -> al
	inc si
	out dx, al		; output control byte
	dec dx			; 6845 index addr -> dx
	inc ah			; bump addr
	cmp ah, parm_count
	jnz sp_loop
	pop si
	pop dx
	pop ax
	ret
setParms endp

; H_puts - print text in graphics mode
;
;	cx = row
;	bx = column
;	si = address of string (null terminated) to print

beginproc _H_puts
	push bp
	mov bp, sp
	push si
	push ds
	mov si, [bp+X]			; string offset

ifdef LARGE_DATA
	mov ds, [bp+X+2]		; string segment
	mov cx, [bp+X+4]		; row
	mov bx, [bp+X+6]		; col
else
	mov cx, [bp+X+2]		; row
	mov bx, [bp+X+4]		; col
endif

ploop:	lodsb				; get next char
	or	al, al			; end of display?
	je	pdone
	call near ptr display
	inc	bx			; bump to next column
	jmp	ploop
pdone:	pop ds
	pop si
	pop bp
	ret
_H_puts	endp

;
; display - output an 8x8 character from the IBM ROM to the Herc board
;
; AX = char, BX = column (0-89), CX = row(0-42)  ** all preserved **
;
CON8	db	8
CON180	db	180
IBMROM	equ	0F000h
CHARTAB	equ	0FA6Eh

display	proc near
	push	ds			; save the lot
	push	es
	push	ax
	push	bx
	push	cx
	push	dx
	push	si
	push	di

; setup ds -> IBM ROM, and si -> index into IBM ROM character table located
;	at 0fa6eh in the ROM

	and	ax, 07fh
	mul	cs:CON8			; mult by 8 bytes of table per char
	mov	si, ax
	mov	ax, IBMROM
	mov	ds, ax
	assume	ds:nothing
	add	si, CHARTAB		; add offset of character table

; compute index into Hercules screen memory for scan line 0.  The remaining
;	seven scan lines are all at fixed offsets from the first.
;
;	Since graphics mode treats the screen as sets of 16x4 "characters",
;	we need to map an 8x8 real character onto the front or back of
;	a pair of graphics "characters".  The first four scan lines of our
;	8x8 character will map to the top graphics "character", and the second
;	four scan lines map to the graphics character on the "line" (4 scan
;	lines high) below it.
;
;	For some exotic hardware reason (probably speed), all scan line 0
;	bits (i.e. every fourth scan line) are stored in memory locations
;	0-2000h in the screen buffer.  All scan line 1 bits are stored
;	2000h-4000h.  Within these banks, they are stored by rows.  The first
;	scan line on the screen (scan line 0 of graphics character row 0)
;	is the first 45 words of memory in the screen buffer.  The next 45
;	words are the first scan line graphics row 1, and since graphics
;	"characters" are 4 bits high, this second scan line is physically
;	the fifth scan line displayed on the screen.
;
;	SO, to display an 8x8 character, the 1st and 5th rows of dots are
;	both scan line 0 of the graphics "character", the 2nd and 6th are
;	scan line 1, and so on.
;
;	The column (0-89) tells which byte in a scan line we need to load.
;	Since it takes two rows of graphics characters to hold one row of
;	our characters, column+90 is a index to scan line 4 rows of pixels
;	higher (n+4).  Thus 180 bytes of screen memory in any bank (0h, 2000h,
;	4000h, 6000h) represent a row of 8x8 characters.
;	
;	The starting location in screen memory for the first scan line of
;	a character to be displayed will be:  	(row*180)+column
;	The 5th scan line will be at:		(row*180)+column+90
;
;	The second and 6th scan lines will be at the above offsets plus
;	the bank offset of 2000h.  The third and 7th, add 4000h and finally
;	the 4th and 8th, add 6000h.
;
	mov	ax, GPg1_Base
	mov	es, ax			; es = hercules page 0
	mov	ax, cx			; get row
	mul	cs:CON180		; mult by 180(10)
	mov	di, ax			; di = index reg
	cld				; insure right direction

;output 8 segments of character to video ram

	lodsb				; line 0
	mov	es:[di+bx], al
	lodsb
	mov	es:[di+bx+2000h], al	; line 1
	lodsb
	mov	es:[di+bx+4000h], al	; line 2
	lodsb
	mov	es:[di+bx+6000h], al	; line 3
	lodsb
	mov	es:[di+bx+90], al	; line 4
	lodsb
	mov	es:[di+bx+2000h+90], al	; line 5
	lodsb
	mov	es:[di+bx+4000h+90], al	; line 6
	lodsb
	mov	es:[di+bx+6000h+90], al	; line 7

	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	pop	es
	pop	ds
	ret
display	endp

_text	ends

_data	segment
bmask	dw -1
color	db 1
_data	ends

const	segment
HCh_Parms db 	61H, 50H, 52H, 0FH, 19H, 06H, 19H, 19H, 02H, 0DH, 0BH, 0CH
HGr_Parms db	35H, 2DH, 2EH, 07H, 5BH, 02H, 57H, 57H, 02H, 03H, 00H, 00H
const	ends

	end


