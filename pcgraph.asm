TITLE	PC graphics module
;	Colin Kelley
;	December 22, 1986
;	modified January 1987 to use LINEPROC.MAC

include header.mac

if1
include lineproc.mac
endif


_text	segment

	public _PC_line, _PC_color, _PC_mask, _PC_curloc, _PC_puts, _Vmode
	public _save_stack, _ss_interrupt
	extrn _interrupt:near

pcpixel proc near
	ror word ptr linemask,1
	jc cont
	ret
cont:
	push ax
	push bx
	push cx
	push dx
	push bp
	mov cx,ax		; x
	mov dx,bx		; y
	mov ah,0ch		; ah = write pixel
	mov al,byte ptr color

	mov bh, 0		; page 0
	int 10h
	pop bp
	pop dx
	pop cx
	pop bx
	pop ax
	ret
pcpixel endp

lineproc _PC_line, pcpixel

_PC_color proc near
	push bp
	mov bp,sp
	mov al,[bp+X]			; color
	mov byte ptr color,al
	pop bp
	ret
_PC_color endp

_PC_mask proc near
	push bp
	mov bp,sp
	mov ax,[bp+X]			; mask
	mov word ptr linemask,ax
	pop bp
	ret
_PC_mask endp

_Vmode	proc near
	push bp
	mov bp,sp
	push si
	push di
	mov ax,[bp+X]
	int 10h
	pop di
	pop si
	pop bp
	ret
_Vmode	endp

_PC_curloc proc near
	push bp
	mov bp,sp
	mov dh, byte ptr [bp+X] ; row number
	mov dl, byte ptr [bp+X+2] ; col number
	mov bh, 0
	mov ah, 2
	int 10h
	pop bp
	ret
_PC_curloc endp

;
; thanks to watale!broehl for finding a bug here--I wasn't pushing BP
;   and reloading AH before INT 10H, which is necessary on genuine IBM
;   boards...
;
_PC_puts proc near
	push bp
	mov bp,sp
	push si
	mov bl,byte ptr color
	mov si,[bp+X]		; offset

ifdef LARGE_DATA
	mov es,[bp+X+2]		; segment if large or compact data model
endif

puts2:

ifdef LARGE_DATA
	mov al,es:[si]
else
	mov al,[si]
endif
	or al,al
	jz puts3
	mov ah,0eh		; write TTY char
	int 10h
	inc si
	jmp short puts2
puts3:	pop si
	pop bp
	ret
_PC_puts endp


; int kbhit();
;   for those without MSC 4.0
; Use BIOS interrupt 16h to determine if a key is waiting in the buffer.
; Return nonzero if so.
;

beginproc _kbhit
	mov ah, 1		; function code 1 is keyboard test
	int 16h			; keyboard functions
	jnz kbfin		; Exit if char available
	xor ax, ax		; No char:  return zero.
kbfin:	ret
_kbhit	endp


; _save_stack and _ss_interrupt are needed due to a bug in the MSC 4.0
; code when run under MS-DOS 3.x.  Starting with 3.0, MS-DOS automatically
; switches to an internal stack during system calls.  This leaves SS:SP
; pointing at MS-DOS's stack when the ^C interrupt (INT 23H) is triggered.
; MSC should restore its own stack before calling the user signal() routine,
; but it doesn't.
;
; Presumably this code will be unnecessary in later releases of the compiler.
;

; _save_stack saves the current SS:SP to be loaded later by _ss_interrupt.
;

beginproc _save_stack
	mov ax,ss
	mov cs:save_ss,ax
	mov ax,sp
	mov cs:save_sp,ax
	ret
_save_stack endp


; _ss_interrupt is called on ^C (INT 23H).  It restores SS:SP as saved in
; _save_stack and then jumps to the C routine interrupt().
;
beginproc _ss_interrupt
	cli			; no interrupts while the stack is changed!
	mov ax,-1		; self-modifying code again
save_ss	equ this word - 2
	mov ss,ax
	mov sp,-1		; here too
save_sp equ this word - 2
	sti
	jmp _interrupt		; now it's safe to call the real routine
_ss_interrupt endp


_text	ends


const	segment
linemask dw -1
color	db 1
const	ends

	end
