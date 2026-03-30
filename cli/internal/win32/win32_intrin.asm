;; win32_intrin.asm
;;
;; These are intended as minimal implementations of miscellaneous intrinsic
;; functions
;;

.code

align(16)
BitScanForward64 proc
    bsf         rax, rcx
    ret
BitScanForward64 endp

align(16)
BitScanReverse64 proc
    bsr         rax, rcx
    ret
BitScanReverse64 endp

align(16)
PopCount64 proc
    popcnt      rax, rcx
    ret
PopCount64 endp

align(16)
MemZero proc
    push        rdi
    mov         rdi, rcx
    xor         eax, eax
    mov         rcx, rdx
    rep stosb
    pop         rdi
    bnd ret
MemZero endp

;; align(16)
;; PathFindFileNameW proc                      ;; NOTE: LPCWSTR PathFindFileNameW(LPCWSTR)
;;     mov         rax, rcx
;; 
;; @find_file_name_w_loop:
;;     movzx       edx, word ptr [rcx]
;;     test        dx, dx
;;     bnd jz      @find_file_name_w_done
;;     cmp         dx, '\'
;;     bnd je      @find_file_name_w_found
;;     cmp         dx, '/'
;;     bnd je      @find_file_name_w_found
;;     cmp         dx, ':'
;;     bnd je      @find_file_name_w_found
;;     add         rcx, 2
;;     bnd jmp     @find_file_name_w_loop
;; 
;; @find_file_name_w_found:
;;     add         rcx, 2
;;     mov         rax, rcx
;;     bnd jmp     @find_file_name_w_loop
;; 
;; @find_file_name_w_done:
;;     bnd ret
;; PathFindFileNameW endp
;; 
;; PathRemoveFileSpecW proc                    ;; NOTE: BOOL PathRemoveFileSpecW(LPWSTR pszPath)
;;     mov         r8, rcx
;;     xor         r9, r9
;; 
;; @remove_file_spec_w_loop:
;;     movzx       edx, word ptr [rcx]
;;     test        dx, dx
;;     bnd jz      @remove_file_spec_w_done
;;     cmp         dx, '\'
;;     bnd je      @remove_file_spec_w_found
;;     cmp         dx, '/'
;;     bnd je      @remove_file_spec_w_found
;;     add         rcx, 2
;;     bnd jmp     @remove_file_spec_w_loop
;; 
;; @remove_file_spec_w_found:
;;     mov         r9, rcx
;;     add         rcx, 2
;;     bnd jmp     @remove_file_spec_w_loop
;; 
;; @remove_file_spec_w_done:
;;     test        r9, r9
;;     bnd jz      @remove_file_spec_w_no_sep
;;     mov         word ptr [r9], 0
;;     mov         eax, 1
;;     bnd ret
;; 
;; @remove_file_spec_w_no_sep:
;;     xor         eax, eax
;;     bnd ret
;; PathRemoveFileSpecW endp

end
