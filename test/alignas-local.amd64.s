	.text
	.globl main
	.balign 16
main:
	pushq %rbp
	movq %rsp, %rbp
	subq $48, %rsp
.LB1:
.LB2:
	leaq -16(%rbp), %rax
	leaq -24(%rbp), %r11
	movq %rax, (%r11)
	movq $16, %rax
	leaq -32(%rbp), %r11
	movq %rax, (%r11)
	leaq -24(%rbp), %r11
	movq (%r11), %rax
	leaq -32(%rbp), %r11
	movq (%r11), %rcx
	xorq %rdx, %rdx
	divq %rcx
	movq %rdx, %rax
	leaq -40(%rbp), %r11
	movq %rax, (%r11)
	leaq -40(%rbp), %r11
	movq (%r11), %rax
	leaq -44(%rbp), %r11
	movl %eax, (%r11)
	leaq -44(%rbp), %r11
	movslq (%r11), %rax
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
