	.text
	.globl f
	.balign 16
f:
	pushq %rbp
	movq %rsp, %rbp
	subq $64, %rsp
.LB1:
	movq %rdi, %rax
	leaq -8(%rbp), %r11
	movq %rax, (%r11)
	movq %rsi, %rax
	leaq -16(%rbp), %r11
	movq %rax, (%r11)
.LB2:
	leaq -8(%rbp), %r11
	movq (%r11), %rax
	leaq -24(%rbp), %r11
	movq %rax, (%r11)
	leaq -24(%rbp), %r11
	movq (%r11), %rax
	leaq -32(%rbp), %r11
	movq %rax, (%r11)
	leaq -16(%rbp), %r11
	movq (%r11), %rax
	leaq -40(%rbp), %r11
	movq %rax, (%r11)
	leaq -40(%rbp), %r11
	movq (%r11), %rax
	leaq -48(%rbp), %r11
	movq %rax, (%r11)
	leaq -32(%rbp), %r11
	movq (%r11), %rax
	leaq -48(%rbp), %r11
	movq (%r11), %rcx
	subq %rcx, %rax
	leaq -56(%rbp), %r11
	movq %rax, (%r11)
	leaq -56(%rbp), %r11
	movq (%r11), %rax
	movq $4, %rcx
	cqto
	idivq %rcx
	leaq -64(%rbp), %r11
	movq %rax, (%r11)
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
