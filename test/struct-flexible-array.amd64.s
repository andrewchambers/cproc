	.data
	.globl x
	.balign 4
x:
	.long 4
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
.LB2:
	leaq -8(%rbp), %r11
	movq (%r11), %rax
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
	movq $2, %rax
	leaq -40(%rbp), %r11
	movq %rax, (%r11)
	leaq -40(%rbp), %r11
	movq (%r11), %rax
	movq $2, %rcx
	imulq %rcx, %rax
	leaq -48(%rbp), %r11
	movq %rax, (%r11)
	leaq -32(%rbp), %r11
	movq (%r11), %rax
	leaq -48(%rbp), %r11
	movq (%r11), %rcx
	addq %rcx, %rax
	leaq -56(%rbp), %r11
	movq %rax, (%r11)
	leaq -56(%rbp), %r11
	movq (%r11), %r11
	movswq (%r11), %rax
	leaq -58(%rbp), %r11
	movw %ax, (%r11)
	leaq -58(%rbp), %r11
	movswq (%r11), %rax
	leaq -64(%rbp), %r11
	movl %eax, (%r11)
	leaq -64(%rbp), %r11
	movslq (%r11), %rax
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
