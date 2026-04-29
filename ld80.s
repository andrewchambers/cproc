.text

.globl __cproc_ld80_add
__cproc_ld80_add:
	fldt (%rsi)
	fldt (%rdx)
	faddp %st, %st(1)
	fstpt (%rdi)
	ret

.globl __cproc_ld80_sub
__cproc_ld80_sub:
	fldt (%rsi)
	fldt (%rdx)
	fsubrp %st, %st(1)
	fstpt (%rdi)
	ret

.globl __cproc_ld80_mul
__cproc_ld80_mul:
	fldt (%rsi)
	fldt (%rdx)
	fmulp %st, %st(1)
	fstpt (%rdi)
	ret

.globl __cproc_ld80_div
__cproc_ld80_div:
	fldt (%rsi)
	fldt (%rdx)
	fdivrp %st, %st(1)
	fstpt (%rdi)
	ret

.globl __cproc_ld80_neg
__cproc_ld80_neg:
	fldt (%rsi)
	fchs
	fstpt (%rdi)
	ret

.globl __cproc_i32_to_ld80
__cproc_i32_to_ld80:
	subq $16, %rsp
	movl %esi, (%rsp)
	fildl (%rsp)
	fstpt (%rdi)
	addq $16, %rsp
	ret

.globl __cproc_u32_to_ld80
__cproc_u32_to_ld80:
	subq $16, %rsp
	movl %esi, %eax
	movq %rax, (%rsp)
	fildq (%rsp)
	fstpt (%rdi)
	addq $16, %rsp
	ret

.globl __cproc_i64_to_ld80
__cproc_i64_to_ld80:
	subq $16, %rsp
	movq %rsi, (%rsp)
	fildq (%rsp)
	fstpt (%rdi)
	addq $16, %rsp
	ret

.globl __cproc_u64_to_ld80
__cproc_u64_to_ld80:
	subq $16, %rsp
	movq %rsi, (%rsp)
	fildq (%rsp)
	testq %rsi, %rsi
	jns 1f
	fldt .Ltwo64(%rip)
	faddp %st, %st(1)
1:
	fstpt (%rdi)
	addq $16, %rsp
	ret

.globl __cproc_f32_to_ld80
__cproc_f32_to_ld80:
	subq $16, %rsp
	movss %xmm0, (%rsp)
	flds (%rsp)
	fstpt (%rdi)
	addq $16, %rsp
	ret

.globl __cproc_f64_to_ld80
__cproc_f64_to_ld80:
	subq $16, %rsp
	movsd %xmm0, (%rsp)
	fldl (%rsp)
	fstpt (%rdi)
	addq $16, %rsp
	ret

.globl __cproc_ld80_to_i32
__cproc_ld80_to_i32:
	subq $16, %rsp
	fnstcw 14(%rsp)
	movzwl 14(%rsp), %eax
	orw $0x0c00, %ax
	movw %ax, 12(%rsp)
	fldcw 12(%rsp)
	fldt (%rdi)
	fistpl 8(%rsp)
	fldcw 14(%rsp)
	movl 8(%rsp), %eax
	addq $16, %rsp
	ret

.globl __cproc_ld80_to_u32
__cproc_ld80_to_u32:
	subq $16, %rsp
	fnstcw 14(%rsp)
	movzwl 14(%rsp), %eax
	orw $0x0c00, %ax
	movw %ax, 12(%rsp)
	fldcw 12(%rsp)
	fldt (%rdi)
	fistpll (%rsp)
	fldcw 14(%rsp)
	movl (%rsp), %eax
	addq $16, %rsp
	ret

.globl __cproc_ld80_to_i64
__cproc_ld80_to_i64:
	subq $16, %rsp
	fnstcw 14(%rsp)
	movzwl 14(%rsp), %eax
	orw $0x0c00, %ax
	movw %ax, 12(%rsp)
	fldcw 12(%rsp)
	fldt (%rdi)
	fistpll (%rsp)
	fldcw 14(%rsp)
	movq (%rsp), %rax
	addq $16, %rsp
	ret

.globl __cproc_ld80_to_u64
__cproc_ld80_to_u64:
	subq $32, %rsp
	fnstcw 30(%rsp)
	movzwl 30(%rsp), %eax
	orw $0x0c00, %ax
	movw %ax, 28(%rsp)
	fldcw 28(%rsp)
	fldt (%rdi)
	fldt .Ltwo63(%rip)
	fucomip %st(1), %st
	jbe 1f
	fistpll 16(%rsp)
	movq 16(%rsp), %rax
	jmp 2f
1:
	fldt .Ltwo63(%rip)
	fsubrp %st, %st(1)
	fistpll 16(%rsp)
	movq 16(%rsp), %rax
	btsq $63, %rax
2:
	fldcw 30(%rsp)
	addq $32, %rsp
	ret

.globl __cproc_ld80_to_f32
__cproc_ld80_to_f32:
	subq $16, %rsp
	fldt (%rdi)
	fstps (%rsp)
	movss (%rsp), %xmm0
	addq $16, %rsp
	ret

.globl __cproc_ld80_to_f64
__cproc_ld80_to_f64:
	subq $16, %rsp
	fldt (%rdi)
	fstpl (%rsp)
	movsd (%rsp), %xmm0
	addq $16, %rsp
	ret

.macro cmp_begin
	fldt (%rdi)
	fldt (%rsi)
	fucomip %st(1), %st
	fstp %st(0)
.endm

.macro cmp_finish
	movzbl %al, %eax
	ret
.endm

.globl __cproc_ld80_eq
__cproc_ld80_eq:
	cmp_begin
	sete %al
	setnp %dl
	andb %dl, %al
	cmp_finish

.globl __cproc_ld80_ne
__cproc_ld80_ne:
	cmp_begin
	setne %al
	setp %dl
	orb %dl, %al
	cmp_finish

.globl __cproc_ld80_lt
__cproc_ld80_lt:
	cmp_begin
	seta %al
	cmp_finish

.globl __cproc_ld80_le
__cproc_ld80_le:
	cmp_begin
	setae %al
	cmp_finish

.globl __cproc_ld80_gt
__cproc_ld80_gt:
	cmp_begin
	setb %al
	setnp %dl
	andb %dl, %al
	cmp_finish

.globl __cproc_ld80_ge
__cproc_ld80_ge:
	cmp_begin
	setbe %al
	setnp %dl
	andb %dl, %al
	cmp_finish

.globl __cproc_ld80_bool
__cproc_ld80_bool:
	fldt (%rdi)
	fldz
	fucomip %st(1), %st
	fstp %st(0)
	setne %al
	setp %dl
	orb %dl, %al
	cmp_finish

.section .rodata
.p2align 4
.Ltwo63:
	.byte 0,0,0,0,0,0,0,0x80,0x3e,0x40
	.zero 6
.Ltwo64:
	.byte 0,0,0,0,0,0,0,0x80,0x3f,0x40
	.zero 6

.section .note.GNU-stack,"",@progbits
