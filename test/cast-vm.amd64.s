	.text
	.globl main
	.balign 16
main:
	pushq %rbp
	movq %rsp, %rbp
	subq $80, %rsp
.LB1:
.LB2:
	movq $0, %rax
	leaq -4(%rbp), %r11
	movl %eax, (%r11)
	leaq -4(%rbp), %r11
	movslq (%r11), %rax
	leaq -8(%rbp), %r11
	movl %eax, (%r11)
	leaq -8(%rbp), %r11
	movslq (%r11), %rax
	movq $1, %rcx
	addq %rcx, %rax
	leaq -12(%rbp), %r11
	movl %eax, (%r11)
	leaq -12(%rbp), %r11
	movslq (%r11), %rax
	leaq -4(%rbp), %r11
	movl %eax, (%r11)
	leaq -12(%rbp), %r11
	movslq (%r11), %rax
	leaq -24(%rbp), %r11
	movq %rax, (%r11)
	leaq -24(%rbp), %r11
	movq (%r11), %rax
	movq $4, %rcx
	imulq %rcx, %rax
	leaq -32(%rbp), %r11
	movq %rax, (%r11)
	movq $0, %rax
	leaq -40(%rbp), %r11
	movq %rax, (%r11)
	leaq -4(%rbp), %r11
	movslq (%r11), %rax
	leaq -44(%rbp), %r11
	movl %eax, (%r11)
	leaq -44(%rbp), %r11
	movslq (%r11), %rax
	movq $1, %rcx
	addq %rcx, %rax
	leaq -48(%rbp), %r11
	movl %eax, (%r11)
	leaq -48(%rbp), %r11
	movslq (%r11), %rax
	leaq -4(%rbp), %r11
	movl %eax, (%r11)
	leaq -48(%rbp), %r11
	movslq (%r11), %rax
	leaq -56(%rbp), %r11
	movq %rax, (%r11)
	leaq -56(%rbp), %r11
	movq (%r11), %rax
	movq $4, %rcx
	imulq %rcx, %rax
	leaq -64(%rbp), %r11
	movq %rax, (%r11)
	movq $0, %rax
	leaq -72(%rbp), %r11
	movq %rax, (%r11)
	leaq -4(%rbp), %r11
	movslq (%r11), %rax
	leaq -76(%rbp), %r11
	movl %eax, (%r11)
	leaq -76(%rbp), %r11
	movslq (%r11), %rax
	movq $2, %rcx
	cmpq %rcx, %rax
	setne %al
	movzbq %al, %rax
	leaq -80(%rbp), %r11
	movl %eax, (%r11)
	leaq -80(%rbp), %r11
	movslq (%r11), %rax
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
