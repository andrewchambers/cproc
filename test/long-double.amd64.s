	.data
	.globl global
	.balign 16
global:
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 192
	.byte 255
	.byte 63
	.zero 6
	.data
	.balign 16
table:
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 128
	.byte 0
	.byte 64
	.zero 6
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 128
	.byte 254
	.byte 191
	.zero 6
	.text
	.globl identity
	.balign 16
identity:
	pushq %rbp
	movq %rsp, %rbp
	subq $32, %rsp
.LB1:
	leaq -16(%rbp), %r10
	movq 16(%rbp), %rax
	movq %rax, 0(%r10)
	movq 24(%rbp), %rax
	movq %rax, 8(%r10)
.LB2:
	leaq -16(%rbp), %r11
	fldt (%r11)
	leaq -32(%rbp), %r11
	fstpt (%r11)
	movw $0, 10(%r11)
	movl $0, 12(%r11)
	fldt -32(%rbp)
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .rodata
	.balign 16
.LCF1:
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 128
	.byte 255
	.byte 63
	.zero 6
	.text
	.globl arith
	.balign 16
arith:
	pushq %rbp
	movq %rsp, %rbp
	subq $208, %rsp
.LB3:
	leaq -16(%rbp), %r10
	movq 16(%rbp), %rax
	movq %rax, 0(%r10)
	movq 24(%rbp), %rax
	movq %rax, 8(%r10)
	leaq -32(%rbp), %r10
	movq 32(%rbp), %rax
	movq %rax, 0(%r10)
	movq 40(%rbp), %rax
	movq %rax, 8(%r10)
.LB4:
	leaq -16(%rbp), %r11
	fldt (%r11)
	leaq -48(%rbp), %r11
	fstpt (%r11)
	movw $0, 10(%r11)
	movl $0, 12(%r11)
	leaq -32(%rbp), %r11
	fldt (%r11)
	leaq -64(%rbp), %r11
	fstpt (%r11)
	movw $0, 10(%r11)
	movl $0, 12(%r11)
	fldt -48(%rbp)
	fldt -64(%rbp)
	faddp %st, %st(1)
	leaq -80(%rbp), %r11
	fstpt (%r11)
	movw $0, 10(%r11)
	movl $0, 12(%r11)
	leaq -16(%rbp), %r11
	fldt (%r11)
	leaq -96(%rbp), %r11
	fstpt (%r11)
	movw $0, 10(%r11)
	movl $0, 12(%r11)
	leaq -32(%rbp), %r11
	fldt (%r11)
	leaq -112(%rbp), %r11
	fstpt (%r11)
	movw $0, 10(%r11)
	movl $0, 12(%r11)
	fldt -96(%rbp)
	fldt -112(%rbp)
	fsubrp %st, %st(1)
	leaq -128(%rbp), %r11
	fstpt (%r11)
	movw $0, 10(%r11)
	movl $0, 12(%r11)
	fldt -80(%rbp)
	fldt -128(%rbp)
	fmulp %st, %st(1)
	leaq -144(%rbp), %r11
	fstpt (%r11)
	movw $0, 10(%r11)
	movl $0, 12(%r11)
	leaq -16(%rbp), %r11
	fldt (%r11)
	leaq -160(%rbp), %r11
	fstpt (%r11)
	movw $0, 10(%r11)
	movl $0, 12(%r11)
	fldt -160(%rbp)
	fldz
	fucomip %st(1), %st
	fstp %st(0)
	jne .LB5
	jmp .LB6
.LB5:
	leaq -16(%rbp), %r11
	fldt (%r11)
	leaq -192(%rbp), %r11
	fstpt (%r11)
	movw $0, 10(%r11)
	movl $0, 12(%r11)
	fldt -192(%rbp)
	leaq -176(%rbp), %r11
	fstpt (%r11)
	movw $0, 10(%r11)
	movl $0, 12(%r11)
	jmp .LB7
.LB6:
	fldt .LCF1(%rip)
	leaq -176(%rbp), %r11
	fstpt (%r11)
	movw $0, 10(%r11)
	movl $0, 12(%r11)
	jmp .LB7
.LB7:
	fldt -144(%rbp)
	fldt -176(%rbp)
	fdivrp %st, %st(1)
	leaq -208(%rbp), %r11
	fstpt (%r11)
	movw $0, 10(%r11)
	movl $0, 12(%r11)
	fldt -208(%rbp)
	jmp .Lret2
.Lret2:
	leave
	ret
	.text
	.globl mixed
	.balign 16
mixed:
	pushq %rbp
	movq %rsp, %rbp
	subq $160, %rsp
.LB8:
	movq %rdi, %rax
	leaq -4(%rbp), %r11
	movl %eax, (%r11)
	movq %rsi, %rax
	leaq -8(%rbp), %r11
	movl %eax, (%r11)
	movq %rdx, %rax
	leaq -12(%rbp), %r11
	movl %eax, (%r11)
	movq %rcx, %rax
	leaq -16(%rbp), %r11
	movl %eax, (%r11)
	movq %r8, %rax
	leaq -20(%rbp), %r11
	movl %eax, (%r11)
	movq %r9, %rax
	leaq -24(%rbp), %r11
	movl %eax, (%r11)
	movq 16(%rbp), %rax
	leaq -28(%rbp), %r11
	movl %eax, (%r11)
	leaq -48(%rbp), %r10
	movq 24(%rbp), %rax
	movq %rax, 0(%r10)
	movq 32(%rbp), %rax
	movq %rax, 8(%r10)
	leaq -64(%rbp), %r10
	movq 40(%rbp), %rax
	movq %rax, 0(%r10)
	movq 48(%rbp), %rax
	movq %rax, 8(%r10)
.LB9:
	leaq -48(%rbp), %r11
	fldt (%r11)
	leaq -80(%rbp), %r11
	fstpt (%r11)
	movw $0, 10(%r11)
	movl $0, 12(%r11)
	leaq -64(%rbp), %r11
	fldt (%r11)
	leaq -96(%rbp), %r11
	fstpt (%r11)
	movw $0, 10(%r11)
	movl $0, 12(%r11)
	fldt -80(%rbp)
	fldt -96(%rbp)
	faddp %st, %st(1)
	leaq -112(%rbp), %r11
	fstpt (%r11)
	movw $0, 10(%r11)
	movl $0, 12(%r11)
	leaq -28(%rbp), %r11
	movslq (%r11), %rax
	leaq -116(%rbp), %r11
	movl %eax, (%r11)
	leaq -116(%rbp), %r11
	movslq (%r11), %rax
	leaq -128(%rbp), %r11
	movq %rax, (%r11)
	fildq -128(%rbp)
	leaq -144(%rbp), %r11
	fstpt (%r11)
	movw $0, 10(%r11)
	movl $0, 12(%r11)
	fldt -112(%rbp)
	fldt -144(%rbp)
	faddp %st, %st(1)
	leaq -160(%rbp), %r11
	fstpt (%r11)
	movw $0, 10(%r11)
	movl $0, 12(%r11)
	fldt -160(%rbp)
	jmp .Lret3
.Lret3:
	leave
	ret
	.text
	.globl callmixed
	.balign 16
callmixed:
	pushq %rbp
	movq %rsp, %rbp
	subq $144, %rsp
.LB10:
.LB11:
	leaq global(%rip), %r11
	fldt (%r11)
	leaq -16(%rbp), %r11
	fstpt (%r11)
	movw $0, 10(%r11)
	movl $0, 12(%r11)
	movq $0, %rax
	leaq -24(%rbp), %r11
	movq %rax, (%r11)
	leaq -24(%rbp), %r11
	movq (%r11), %rax
	movq $16, %rcx
	imulq %rcx, %rax
	leaq -32(%rbp), %r11
	movq %rax, (%r11)
	leaq table(%rip), %rax
	leaq -32(%rbp), %r11
	movq (%r11), %rcx
	addq %rcx, %rax
	leaq -40(%rbp), %r11
	movq %rax, (%r11)
	leaq -40(%rbp), %r11
	movq (%r11), %r11
	fldt (%r11)
	leaq -64(%rbp), %r11
	fstpt (%r11)
	movw $0, 10(%r11)
	movl $0, 12(%r11)
	subq $48, %rsp
	movq $1, %rax
	movq %rax, %rdi
	movq $2, %rax
	movq %rax, %rsi
	movq $3, %rax
	movq %rax, %rdx
	movq $4, %rax
	movq %rax, %rcx
	movq $5, %rax
	movq %rax, %r8
	movq $6, %rax
	movq %rax, %r9
	movq $7, %rax
	movq %rax, 0(%rsp)
	fldt -16(%rbp)
	fstpt 8(%rsp)
	movw $0, 18(%rsp)
	movl $0, 20(%rsp)
	fldt -64(%rbp)
	fstpt 24(%rsp)
	movw $0, 34(%rsp)
	movl $0, 36(%rsp)
	movb $0, %al
	call mixed
	addq $48, %rsp
	leaq -80(%rbp), %r11
	fstpt (%r11)
	movw $0, 10(%r11)
	movl $0, 12(%r11)
	movq $1, %rax
	leaq -88(%rbp), %r11
	movq %rax, (%r11)
	leaq -88(%rbp), %r11
	movq (%r11), %rax
	movq $16, %rcx
	imulq %rcx, %rax
	leaq -96(%rbp), %r11
	movq %rax, (%r11)
	leaq table(%rip), %rax
	leaq -96(%rbp), %r11
	movq (%r11), %rcx
	addq %rcx, %rax
	leaq -104(%rbp), %r11
	movq %rax, (%r11)
	leaq -104(%rbp), %r11
	movq (%r11), %r11
	fldt (%r11)
	leaq -128(%rbp), %r11
	fstpt (%r11)
	movw $0, 10(%r11)
	movl $0, 12(%r11)
	fldt -80(%rbp)
	fldt -128(%rbp)
	faddp %st, %st(1)
	leaq -144(%rbp), %r11
	fstpt (%r11)
	movw $0, 10(%r11)
	movl $0, 12(%r11)
	fldt -144(%rbp)
	jmp .Lret4
.Lret4:
	leave
	ret
	.text
	.globl compare
	.balign 16
compare:
	pushq %rbp
	movq %rsp, %rbp
	subq $336, %rsp
.LB12:
	leaq -16(%rbp), %r10
	movq 16(%rbp), %rax
	movq %rax, 0(%r10)
	movq 24(%rbp), %rax
	movq %rax, 8(%r10)
	leaq -32(%rbp), %r10
	movq 32(%rbp), %rax
	movq %rax, 0(%r10)
	movq 40(%rbp), %rax
	movq %rax, 8(%r10)
.LB13:
	leaq -16(%rbp), %r11
	fldt (%r11)
	leaq -64(%rbp), %r11
	fstpt (%r11)
	movw $0, 10(%r11)
	movl $0, 12(%r11)
	leaq -32(%rbp), %r11
	fldt (%r11)
	leaq -80(%rbp), %r11
	fstpt (%r11)
	movw $0, 10(%r11)
	movl $0, 12(%r11)
	fldt -64(%rbp)
	fldt -80(%rbp)
	fucomip %st(1), %st
	fstp %st(0)
	seta %al
	movzbq %al, %rax
	leaq -84(%rbp), %r11
	movl %eax, (%r11)
	leaq -84(%rbp), %r11
	movslq (%r11), %rax
	testq %rax, %rax
	jne .LB14
	jmp .LB21
.LB21:
	leaq -16(%rbp), %r11
	fldt (%r11)
	leaq -112(%rbp), %r11
	fstpt (%r11)
	movw $0, 10(%r11)
	movl $0, 12(%r11)
	leaq -32(%rbp), %r11
	fldt (%r11)
	leaq -128(%rbp), %r11
	fstpt (%r11)
	movw $0, 10(%r11)
	movl $0, 12(%r11)
	fldt -112(%rbp)
	fldt -128(%rbp)
	fucomip %st(1), %st
	fstp %st(0)
	setae %al
	movzbq %al, %rax
	leaq -132(%rbp), %r11
	movl %eax, (%r11)
	leaq -132(%rbp), %r11
	movslq (%r11), %rax
	testq %rax, %rax
	jne .LB14
	jmp .LB20
.LB20:
	leaq -32(%rbp), %r11
	fldt (%r11)
	leaq -160(%rbp), %r11
	fstpt (%r11)
	movw $0, 10(%r11)
	movl $0, 12(%r11)
	leaq -16(%rbp), %r11
	fldt (%r11)
	leaq -176(%rbp), %r11
	fstpt (%r11)
	movw $0, 10(%r11)
	movl $0, 12(%r11)
	fldt -160(%rbp)
	fldt -176(%rbp)
	fucomip %st(1), %st
	fstp %st(0)
	setb %al
	movzbq %al, %rax
	leaq -180(%rbp), %r11
	movl %eax, (%r11)
	leaq -180(%rbp), %r11
	movslq (%r11), %rax
	testq %rax, %rax
	jne .LB14
	jmp .LB19
.LB19:
	leaq -32(%rbp), %r11
	fldt (%r11)
	leaq -208(%rbp), %r11
	fstpt (%r11)
	movw $0, 10(%r11)
	movl $0, 12(%r11)
	leaq -16(%rbp), %r11
	fldt (%r11)
	leaq -224(%rbp), %r11
	fstpt (%r11)
	movw $0, 10(%r11)
	movl $0, 12(%r11)
	fldt -208(%rbp)
	fldt -224(%rbp)
	fucomip %st(1), %st
	fstp %st(0)
	setbe %al
	movzbq %al, %rax
	leaq -228(%rbp), %r11
	movl %eax, (%r11)
	leaq -228(%rbp), %r11
	movslq (%r11), %rax
	testq %rax, %rax
	jne .LB14
	jmp .LB18
.LB18:
	leaq -16(%rbp), %r11
	fldt (%r11)
	leaq -256(%rbp), %r11
	fstpt (%r11)
	movw $0, 10(%r11)
	movl $0, 12(%r11)
	leaq -32(%rbp), %r11
	fldt (%r11)
	leaq -272(%rbp), %r11
	fstpt (%r11)
	movw $0, 10(%r11)
	movl $0, 12(%r11)
	fldt -256(%rbp)
	fldt -272(%rbp)
	fucomip %st(1), %st
	fstp %st(0)
	sete %al
	movzbq %al, %rax
	leaq -276(%rbp), %r11
	movl %eax, (%r11)
	leaq -276(%rbp), %r11
	movslq (%r11), %rax
	testq %rax, %rax
	jne .LB14
	jmp .LB17
.LB17:
	leaq -16(%rbp), %r11
	fldt (%r11)
	leaq -304(%rbp), %r11
	fstpt (%r11)
	movw $0, 10(%r11)
	movl $0, 12(%r11)
	leaq -32(%rbp), %r11
	fldt (%r11)
	leaq -320(%rbp), %r11
	fstpt (%r11)
	movw $0, 10(%r11)
	movl $0, 12(%r11)
	fldt -304(%rbp)
	fldt -320(%rbp)
	fucomip %st(1), %st
	fstp %st(0)
	setne %al
	movzbq %al, %rax
	leaq -324(%rbp), %r11
	movl %eax, (%r11)
	leaq -324(%rbp), %r11
	movslq (%r11), %rax
	testq %rax, %rax
	jne .LB14
	jmp .LB15
.LB14:
	movq $1, %rax
	leaq -36(%rbp), %r11
	movl %eax, (%r11)
	jmp .LB16
.LB15:
	xorq %rax, %rax
	leaq -36(%rbp), %r11
	movl %eax, (%r11)
	jmp .LB16
.LB16:
	leaq -36(%rbp), %r11
	movslq (%r11), %rax
	jmp .Lret5
.Lret5:
	leave
	ret
	.section .rodata
	.balign 16
.LCF2:
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 224
	.byte 1
	.byte 64
	.zero 6
	.text
	.globl convert
	.balign 16
convert:
	pushq %rbp
	movq %rsp, %rbp
	subq $128, %rsp
.LB22:
	leaq -16(%rbp), %r10
	movq 16(%rbp), %rax
	movq %rax, 0(%r10)
	movq 24(%rbp), %rax
	movq %rax, 8(%r10)
.LB23:
	leaq -16(%rbp), %r11
	fldt (%r11)
	leaq -32(%rbp), %r11
	fstpt (%r11)
	movw $0, 10(%r11)
	movl $0, 12(%r11)
	fldt -32(%rbp)
	fnstcw -36(%rbp)
	movw -36(%rbp), %ax
	andw $0xf3ff, %ax
	orw $0x0c00, %ax
	movw %ax, -34(%rbp)
	fldcw -34(%rbp)
	fistpl -40(%rbp)
	fldcw -36(%rbp)
	leaq -40(%rbp), %r11
	movslq (%r11), %rax
	leaq -44(%rbp), %r11
	movl %eax, (%r11)
	movq $7, %rax
	leaq -56(%rbp), %r11
	movq %rax, (%r11)
	fildq -56(%rbp)
	leaq -80(%rbp), %r11
	fstpt (%r11)
	movw $0, 10(%r11)
	movl $0, 12(%r11)
	fldt -80(%rbp)
	fldt .LCF2(%rip)
	fucomip %st(1), %st
	fstp %st(0)
	sete %al
	movzbq %al, %rax
	leaq -84(%rbp), %r11
	movl %eax, (%r11)
	leaq -44(%rbp), %r11
	movslq (%r11), %rax
	leaq -84(%rbp), %r11
	movslq (%r11), %rcx
	addq %rcx, %rax
	leaq -88(%rbp), %r11
	movl %eax, (%r11)
	leaq -16(%rbp), %r11
	fldt (%r11)
	leaq -112(%rbp), %r11
	fstpt (%r11)
	movw $0, 10(%r11)
	movl $0, 12(%r11)
	fldt -112(%rbp)
	fldz
	fucomip %st(1), %st
	fstp %st(0)
	jne .LB24
	jmp .LB25
.LB24:
	movq $1, %rax
	leaq -116(%rbp), %r11
	movl %eax, (%r11)
	jmp .LB26
.LB25:
	movq $0, %rax
	leaq -116(%rbp), %r11
	movl %eax, (%r11)
	jmp .LB26
.LB26:
	leaq -88(%rbp), %r11
	movslq (%r11), %rax
	leaq -116(%rbp), %r11
	movslq (%r11), %rcx
	addq %rcx, %rax
	leaq -120(%rbp), %r11
	movl %eax, (%r11)
	leaq -120(%rbp), %r11
	movslq (%r11), %rax
	jmp .Lret6
.Lret6:
	leave
	ret
	.section .rodata
	.balign 16
.LCF3:
	.byte 8
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 128
	.byte 255
	.byte 63
	.zero 6
	.section .rodata
	.balign 16
.LCF4:
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 128
	.byte 255
	.byte 63
	.zero 6
	.text
	.globl precision
	.balign 16
precision:
	pushq %rbp
	movq %rsp, %rbp
	subq $16, %rsp
.LB27:
.LB28:
	fldt .LCF3(%rip)
	fldt .LCF4(%rip)
	fucomip %st(1), %st
	fstp %st(0)
	setne %al
	movzbq %al, %rax
	leaq -4(%rbp), %r11
	movl %eax, (%r11)
	leaq -4(%rbp), %r11
	movslq (%r11), %rax
	jmp .Lret7
.Lret7:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
