	.section .rodata
	.balign 4
.LCF1:
	.long 1065353216
	.section .rodata
	.balign 4
.LCF2:
	.long 1065353216
	.text
	.globl f
	.balign 16
f:
	pushq %rbp
	movq %rsp, %rbp
	subq $16, %rsp
.LB1:
.LB2:
	movss .LCF1(%rip), %xmm0
	cvtss2sd %xmm0, %xmm0
	leaq -8(%rbp), %r11
	movsd %xmm0, (%r11)
	movq $0, %rax
	movq %rax, %rdi
	movsd -8(%rbp), %xmm0
	movb $1, %al
	call g1
	movss .LCF2(%rip), %xmm0
	movb $1, %al
	call g2
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
