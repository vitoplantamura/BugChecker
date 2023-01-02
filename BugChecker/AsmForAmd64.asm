.CODE

extrn esOld : QWORD
extrn esNew : QWORD
extrn esFunction : QWORD

Amd64_ReadKeyb proc

		pushf

		in		al, 64h
		test	al, 1
		jz		_ReturnZero

		mov		ah, al
		in		al, 60h

		test	ah, 100000b		; is "second PS/2 port output buffer full"? Ref: https://wiki.osdev.org/%228042%22_PS/2_Controller
		jnz		_ReturnZero

		jmp		_Exit

_ReturnZero:

		mov		al, 0

_Exit:

		and		rax, 0FFh

		popf
		ret

Amd64_ReadKeyb endp

Amd64_sgdt proc

		sgdt	fword ptr [rcx]
		ret

Amd64_sgdt endp

BreakInAndDeleteBreakPoints proc

	int 3
	ret

BreakInAndDeleteBreakPoints endp

ESCall proc

	mov qword ptr [esOld], rsp
	mov rsp, qword ptr [esNew]
	call qword ptr [esFunction]
	mov rsp, qword ptr [esOld]
	ret

ESCall endp

END
