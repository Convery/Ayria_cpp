; Visual Studio / CL doesn't support inline x64 assembly, so this file is needed.
; This is because a lot of games need some stack-alignment.
; -- Convery (tcn@ayria.se)

_TEXT SEGMENT

; Set by Appmain.cpp
EXTERN EPAddress:qword

; Called from Appmain.cpp
PUBLIC Return
Return PROC

and rsp, 0FFFFFFFFFFFFFFF0h
mov rax, 0DEADDEADDEADDEADh
push rax

mov rax, qword ptr [EPAddress]
jmp rax

Return ENDP
_TEXT ENDS
END