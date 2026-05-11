	.text
	.globl f
	.balign 16
f:
	pushq %rbp
	movq %rsp, %rbp
	subq $32, %rsp
.LB1:
	leaq -8(%rbp), %r11
	movsd %xmm0, (%r11)
.LB2:
	leaq -8(%rbp), %r11
	movsd (%r11), %xmm0
	leaq -16(%rbp), %r11
	movsd %xmm0, (%r11)
	movsd -16(%rbp), %xmm0
	movq $1, %rax
	shlq $63, %rax
	movq %rax, %xmm1
	xorpd %xmm1, %xmm0
	leaq -24(%rbp), %r11
	movsd %xmm0, (%r11)
	movsd -24(%rbp), %xmm0
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
