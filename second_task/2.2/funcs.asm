IFDEF RAX

.code

extern func : proto

f_add PROC
    ; beg
    push rbp
    mov rbp, rsp

    push rax
    push rbx

    mov rax, [rbp + 48]
    mov rbx, [rbp + 56]

    push rbx
    push rax
    push r9
    ; passing int args
    push r8
    push rdx
    push rcx
    call func

    pop rbx
    pop rax

    ; end
    pop rbp
    ret
f_add ENDP

ELSE

.386
.MODEL FLAT

.CODE

extern _func@24 : proto
; fastcall function -> stdcall function
@f_add@24 PROC
    ; beg
    push ebp
    mov ebp, esp

    ; cause we are using this registers to save their value
    push eax
    push ebx
    push edi
    push esi

    ; get fastcall parameters
    mov eax, [ebp + 8 ]
    mov ebx, [ebp + 12]
    mov edi, [ebp + 16]
    mov esi, [ebp + 20]

    ; convert fastcall to stdcall
    push esi
    push edi
    push ebx
    push eax
    push edx
    push ecx
    call _func@24
    
    ; remember registers value
    pop esi
    pop edi
    pop ebx
    pop eax
    ; end
    pop ebp
    ret
@f_add@24 ENDP

ENDIF

END