	.text
	.globl f
	.balign 16
f:
	pushq %rbp
	movq %rsp, %rbp
	subq $48, %rsp
.LB1:
.LB2:
	movq $0, %rax
	leaq -4(%rbp), %r11
	movl %eax, (%r11)
	leaq -4(%rbp), %rax
	leaq -16(%rbp), %r11
	movq %rax, (%r11)
	leaq -16(%rbp), %r11
	movq (%r11), %rax
	leaq -24(%rbp), %r11
	movq %rax, (%r11)
	leaq -24(%rbp), %r11
	movq (%r11), %rax
	movq $4, %rcx
	addq %rcx, %rax
	leaq -32(%rbp), %r11
	movq %rax, (%r11)
	leaq -32(%rbp), %r11
	movq (%r11), %rax
	leaq -16(%rbp), %r11
	movq %rax, (%r11)
	leaq -24(%rbp), %r11
	movq (%r11), %r11
	movslq (%r11), %rax
	leaq -36(%rbp), %r11
	movl %eax, (%r11)
	leaq -36(%rbp), %r11
	movslq (%r11), %rax
	movq $1, %rcx
	addq %rcx, %rax
	leaq -40(%rbp), %r11
	movl %eax, (%r11)
	leaq -40(%rbp), %r11
	movslq (%r11), %rax
	leaq -24(%rbp), %r11
	movq (%r11), %r11
	movl %eax, (%r11)
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
