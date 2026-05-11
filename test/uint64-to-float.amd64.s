	.text
	.globl f
	.balign 16
f:
	pushq %rbp
	movq %rsp, %rbp
	subq $16, %rsp
.LB1:
.LB2:
	movb $0, %al
	call g
	leaq -8(%rbp), %r11
	movq %rax, (%r11)
	leaq -8(%rbp), %r11
	movq (%r11), %rax
	cvtsi2ssq %rax, %xmm0
	leaq -12(%rbp), %r11
	movss %xmm0, (%r11)
	movss -12(%rbp), %xmm0
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
