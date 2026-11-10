;; win32_math_arm.asm
;;
;; These are intended as minimal implementations of some maths functions
;;

area |.code|, code, readonly, align=4

;; 16-bit______________________________________________________________________
Abs16 proc
    sxth    w0, w0
    cmp     w0, #0
    cneg    w0, w0, mi
    ret
Abs16 endp

;; 32-bit______________________________________________________________________
Abs32 proc
    cmp     w0, #0
    cneg    w0, w0, mi
    ret
Abs32 endp

Max32 proc
    cmp     w0, w1
    csel    w0, w1, w0, lt          ;; NOTE: csel <dest>, <true>, <false>, <op>
    ret
Max32 endp

FMAdd32 proc
    scvtf   d0, w0
    scvtf   d1, w1
    scvtf   d2, w2
    fmadd   d0, d0, d1, d2          ;; NOTE: d0 = (d0 * d1) + d2
    fcvtzs  w0, d0                  ;; NOTE: convert d0 (f64) to w0 (i32) and truncate
    ret
FMAdd32 endp

FMAddF32 proc
    fmadd   d0, d0, d1, d2
    ret
FMAddF32 endp

PowF32 proc
    fmov    w0, s0                  ;; w0 = (i32) s0
    mov     w1, w0                  ;; w1 = w0
    lsr     w0, w0, #23             ;; w0 = w0 >> 23
    sub     w0, w0, #127            ;; w0 = w0 - 127
    scvtf   s2, w0                  ;; s2 = (f32) w0
    ldr     w2, =0x007FFFFF         ;; w2 = 0x007FFFFF
    and     w1, w1, w2              ;; w1 = w1 & w2
    ldr     w2, =0x3F800000         ;; w2 = 0x3F800000
    orr     w1, w1, w2              ;; w1 = w1 | w2
    fmov    s0, w1                  ;; s0 = (f32) w1
    ldr     w2, =0x3F800000         ;; w2 = 0x3F800000
    fmov    s3, w2                  ;; s3 = (f32) w2
    fsub    s0, s0, s3              ;; s0 = s0 - s3
    ldr     w2, =0x3E93DD00         ;; w2 = 0x3E93DD00
    fmov    s3, w2                  ;; s3 = (f32) w2
    fmul    s3, s3, s0              ;; s3 = s3 * s0
    ldr     w2, =0xBEB8AA3B         ;; w2 = 0xBEB8AA3B
    fmov    s4, w2                  ;; s4 = (f32) w2
    fadd    s3, s3, s4              ;; s3 = s3 + s4
    fmul    s3, s3, s0              ;; s3 = s3 * s0
    ldr     w2, =0x3EF637C0         ;; w2 = 0x3EF637C0
    fmov    s4, w2                  ;; s4 = (f32) w2
    fadd    s3, s3, s4              ;; s3 = s3 + s4
    fmul    s3, s3, s0              ;; s3 = s3 * s0
    ldr     w2, =0xBF38AA3B         ;; w2 = 0xBF38AA3B
    fmov    s4, w2                  ;; s4 = (f32) w2
    fadd    s3, s3, s4              ;; s3 = s3 + s4
    fmul    s3, s3, s0              ;; s3 = s3 * s0
    ldr     w2, =0x3FB8AA3B         ;; w2 = 0x3FB8AA3B
    fmov    s4, w2                  ;; s4 = (f32) w2
    fadd    s3, s3, s4              ;; s3 = s3 + s4
    fmul    s3, s3, s0              ;; s3 = s3 * s0
    fadd    s3, s3, s2              ;; s3 = s3 + s2
    fmul    s3, s3, s1              ;; s3 = s3 * s1
    frintm  s4, s3                  ;; s4 = round_floor(s3)
    fcvtzs  w0, s4                  ;; w0 = (i32) s4
    fsub    s3, s3, s4              ;; s3 = s3 - s4
    ldr     w1, =0x3AAF76A1         ;; w1 = 0x3AAF76A1
    fmov    s5, w1                  ;; s5 = (f32) w1
    fmul    s5, s5, s3              ;; s5 = s5 * s3
    ldr     w1, =0x3C1D9539         ;; w1 = 0x3C1D9539
    fmov    s4, w1                  ;; s4 = (f32) w1
    fadd    s5, s5, s4              ;; s5 = s5 + s4
    fmul    s5, s5, s3              ;; s5 = s5 * s3
    ldr     w1, =0x3D635847         ;; w1 = 0x3D635847
    fmov    s4, w1                  ;; s4 = (f32) w1
    fadd    s5, s5, s4              ;; s5 = s5 + s4
    fmul    s5, s5, s3              ;; s5 = s5 * s3
    ldr     w1, =0x3E75FDF0         ;; w1 = 0x3E75FDF0
    fmov    s4, w1                  ;; s4 = (f32) w1
    fadd    s5, s5, s4              ;; s5 = s5 + s4
    fmul    s5, s5, s3              ;; s5 = s5 * s3
    ldr     w1, =0x3F317218         ;; w1 = 0x3F317218
    fmov    s4, w1                  ;; s4 = (f32) w1
    fadd    s5, s5, s4              ;; s5 = s5 + s4
    fmul    s5, s5, s3              ;; s5 = s5 * s3
    ldr     w1, =0x3F800000         ;; w1 = 0x3F800000
    fmov    s4, w1                  ;; s4 = (f32) w1
    fadd    s5, s5, s4              ;; s5 = s5 + s4
    add     w0, w0, #127            ;; w0 = w0 + 127
    lsl     w0, w0, #23             ;; w0 = w0 << 23
    fmov    s0, w0                  ;; s0 = (f32) w0
    fmul    s0, s0, s5              ;; s0 = s0 * s5
    ret
PowF32 endp

ExpF32 proc
    ldr     w0, =0x3FB8AA3B
    fmov    s1, w0
    fmul    s0, s0, s1
    frintn  s1, s0
    fcvtzs  w0, s1
    fsub    s0, s0, s1
    ldr     w2, =0x3AAF047B
    fmov    s1, w2
    fmul    s1, s1, s0
    ldr     w2, =0x3C1D9539
    fmov    s2, w2
    fadd    s1, s1, s2
    fmul    s1, s1, s0
    ldr     w2, =0x3D635847
    fmov    s2, w2
    fadd    s1, s1, s2
    fmul    s1, s1, s0
    ldr     w2, =0x3E75FDF0
    fmov    s2, w2
    fadd    s1, s1, s2
    fmul    s1, s1, s0
    ldr     w2, =0x3F317218
    fmov    s2, w2
    fadd    s1, s1, s2
    fmul    s1, s1, s0
    ldr     w2, =0x3F800000
    fmov    s2, w2
    fadd    s1, s1, s2
    add     w0, w0, #127
    lsl     w0, w0, #23
    fmov    s0, w0
    fmul    s0, s0, s1
    ret
ExpF32 endp

LnF32 proc
    fmov    w0, s0
    mov     w2, w0
    lsr     w0, w0, #23
    sub     w0, w0, #127
    scvtf   s1, w0
    ldr     w1, =0x007FFFFF
    and     w2, w2, w1
    ldr     w1, =0x3F800000
    orr     w2, w2, w1
    fmov    s2, w2
    ldr     w0, =0x3F800000
    fmov    s0, w0
    fsub    s0, s0, s2
    ldr     w0, =0x3E93DD98
    fmov    s4, w0
    fmul    s4, s4, s0
    ldr     w0, =0xBE98AA3B
    fmov    s5, w0
    fadd    s4, s4, s5
    fmul    s4, s4, s0
    ldr     w0, =0x3EF6384F
    fmov    s5, w0
    fadd    s4, s4, s5
    fmul    s4, s4, s0
    ldr     w0, =0xBF38AA3B
    fmov    s5, w0
    fadd    s4, s4, s5
    fmul    s4, s4, s0
    ldr     w0, =0x3FB8AA3B
    fmov    s5, w0
    fadd    s4, s4, s5
    fmul    s4, s4, s0
    fadd    s1, s1, s4
    ldr     w0, =0x3F317218
    fmov    s0, w0
    fmul    s0, s0, s1
    ret
LnF32 endp

Log10F32 proc
    fmov    w0, s0
    mov     w1, w0
    lsr     w0, w0, #23
    sub     w0, w0, #127
    scvtf   s1, w0
    ldr     w2, =0x007FFFFF
    and     w1, w1, w2
    ldr     w2, =0x3F800000
    orr     w1, w1, w2
    fmov    s0, w1
    ldr     w0, =0x3F800000
    fmov    s2, w0
    fsub    s0, s0, s2
    ldr     w0, =0x3E93DD98
    fmov    s4, w0
    fmul    s4, s4, s0
    ldr     w0, =0xBE98AA3B
    fmov    s5, w0
    fadd    s4, s4, s5
    fmul    s4, s4, s0
    ldr     w0, =0x3EF6384F
    fmov    s5, w0
    fadd    s4, s4, s5
    fmul    s4, s4, s0
    ldr     w0, =0xBF38AA3B
    fmov    s5, w0
    fadd    s4, s4, s5
    fmul    s4, s4, s0
    ldr     w0, =0x3FB8AA3B
    fmov    s5, w0
    fadd    s4, s4, s5
    fmul    s4, s4, s0
    fadd    s1, s1, s4
    ldr     w0, =0x3E9A209B
    fmov    s0, w0
    fmul    s0, s0, s1
    ret
Log10F32 endp

Log2F32 proc
    fmov    w0, s0
    mov     w1, w0
    lsr     w0, w0, #23
    sub     w0, w0, #127
    scvtf   s1, w0
    ldr     w0, =0x007FFFFF
    and     w1, w1, w0
    ldr     w0, =0x3F800000
    orr     w1, w1, w0
    fmov    s0, w1
    ldr     w0, =0x3F800000
    fmov    s2, w0
    fsub    s0, s0, s2
    ldr     w0, =0x3E93DD98
    fmov    s4, w0
    fmul    s4, s4, s0
    ldr     w0, =0xBE98AA3B
    fmov    s5, w0
    fadd    s4, s4, s5
    fmul    s4, s4, s0
    ldr     w0, =0x3EF6384F
    fmov    s5, w0
    fadd    s4, s4, s5
    fmul    s4, s4, s0
    ldr     w0, =0xBF38AA3B
    fmov    s5, w0
    fadd    s4, s4, s5
    fmul    s4, s4, s0
    ldr     w0, =0x3FB8AA3B
    fmov    s5, w0
    fadd    s4, s4, s5
    fmul    s4, s4, s0
    fadd    s1, s1, s4
    fmov    s0, s1
    ret
Log2F32 endp

;; SinF32 proc
;;     ; --- 1. Extract Sign and Absolute Value ---
;;     fmov    w0, s0
;;     mov     w1, w0
;;     and     w1, w1, #0x80000000     ; w1 = original sign bit
;;     and     w0, w0, #0x7FFFFFFF     ; w0 = abs(x)
;;     fmov    s0, w0                  ; s0 = abs(x)
 
;;     ; --- 2. Range Reduction (x * 2/pi) ---
;;     ldr     w3, =0x3F22F983
;;     fmov    s1, w3
;;     fmul    s1, s1, s0
;;     frintn  s2, s1                  ; roundss xmm2, xmm1, 0 (Round to nearest)
;;     fcvtzs  w2, s2                  ; cvtss2si edx, xmm2 (w2 = quadrant k)
 
;;     ; --- 3. Subtract k * (pi/2) ---
;;     ldr     w3, =0x3FC90FDB
;;     fmov    s3, w3
;;     fmul    s3, s3, s2
;;     fsub    s0, s0, s3
;;     
;;     fmov    s1, s0
;;     fmul    s1, s1, s0              ; s1 = x^2
 
;;     ; --- 4. Sine Polynomial (Odd powers) -> s4 ---
;;     ldr     w3, =0xB9500D01
;;     fmov    s4, w3
;;     fmul    s4, s4, s1
;;     ldr     w3, =0x3C088889
;;     fmov    s5, w3
;;     fadd    s4, s4, s5
;;     fmul    s4, s4, s1
;;     ldr     w3, =0xBE2AAAAB
;;     fmov    s5, w3
;;     fadd    s4, s4, s5
;;     fmul    s4, s4, s1
;;     ldr     w3, =0x3F800000
;;     fmov    s5, w3
;;     fadd    s4, s4, s5
;;     fmul    s4, s4, s0              ; Multiply by x
 
;;     ; --- 5. Cosine Polynomial (Even powers) -> s6 ---
;;     ldr     w3, =0xBAB60B61
;;     fmov    s6, w3
;;     fmul    s6, s6, s1
;;     ldr     w3, =0x3D2AAAAB
;;     fmov    s7, w3
;;     fadd    s6, s6, s7
;;     fmul    s6, s6, s1
;;     ldr     w3, =0xBF000000
;;     fmov    s7, w3
;;     fadd    s6, s6, s7
;;     fmul    s6, s6, s1
;;     ldr     w3, =0x3F800000
;;     fmov    s7, w3
;;     fadd    s6, s6, s7
 
;;     ; --- 6. Quadrant Branching ---
;;     and     w2, w2, #3              ; edx & 3
;;     cmp     w2, #0
;;     beq     sin_q0
;;     cmp     w2, #1
;;     beq     sin_q1
;;     cmp     w2, #2
;;     beq     sin_q2
 
;;     ; Quadrant 3
;;     fneg    s4, s6                  ; xorps xmm6, 80000000h (Flip sign of Cosine)
;;     b       sin_done
 
;; sin_q0:
;;     b       sin_done
 
;; sin_q1:
;;     fmov    s4, s6                  ; Use Cosine
;;     b       sin_done
 
;; sin_q2:
;;     fneg    s4, s4                  ; xorps xmm4, 80000000h (Flip sign of Sine)
;;     
;; sin_done:
;;     ; --- 7. Apply Original Sign ---
;;     fmov    w3, s4                  ; Move result to GPR
;;     eor     w3, w3, w1              ; XOR with original sign bit
;;     fmov    s0, w3                  ; Move back to return register
;;     
;;     ret
;; SinF32 endp

SinF32 proc
SinF32 endp

end ;; area |.code|, code, readonly, align=4
