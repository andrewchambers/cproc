	.text
	.globl main
	.balign 16
main:
	pushq %rbp
	movq %rsp, %rbp
	subq $48, %rsp
.LB1:
.LB2:
	movq $2, %rax
	leaq -4(%rbp), %r11
	movl %eax, (%r11)
	movq $0, %rax
	leaq -8(%rbp), %r11
	movl %eax, (%r11)
.LB3:
	leaq -4(%rbp), %r11
	movslq (%r11), %rax
	leaq -12(%rbp), %r11
	movl %eax, (%r11)
	leaq -12(%rbp), %r11
	movslq (%r11), %rax
	movq $1, %rcx
	cmpq %rcx, %rax
	sete %al
	movzbq %al, %rax
	leaq -16(%rbp), %r11
	movl %eax, (%r11)
	leaq -16(%rbp), %r11
	movslq (%r11), %rax
	testq %rax, %rax
	jne .LB6
	jmp .LB7
.LB6:
	jmp .LB4
.LB7:
	leaq -8(%rbp), %r11
	movslq (%r11), %rax
	leaq -20(%rbp), %r11
	movl %eax, (%r11)
	leaq -20(%rbp), %r11
	movslq (%r11), %rax
	movq $1, %rcx
	addq %rcx, %rax
	leaq -24(%rbp), %r11
	movl %eax, (%r11)
	leaq -24(%rbp), %r11
	movslq (%r11), %rax
	leaq -8(%rbp), %r11
	movl %eax, (%r11)
.LB4:
	leaq -4(%rbp), %r11
	movslq (%r11), %rax
	leaq -28(%rbp), %r11
	movl %eax, (%r11)
	leaq -28(%rbp), %r11
	movslq (%r11), %rax
	movq $1, %rcx
	subq %rcx, %rax
	leaq -32(%rbp), %r11
	movl %eax, (%r11)
	leaq -32(%rbp), %r11
	movslq (%r11), %rax
	leaq -4(%rbp), %r11
	movl %eax, (%r11)
	leaq -28(%rbp), %r11
	movslq (%r11), %rax
	testq %rax, %rax
	jne .LB3
	jmp .LB5
.LB5:
	leaq -8(%rbp), %r11
	movslq (%r11), %rax
	leaq -36(%rbp), %r11
	movl %eax, (%r11)
	leaq -36(%rbp), %r11
	movslq (%r11), %rax
	movq $2, %rcx
	cmpq %rcx, %rax
	setne %al
	movzbq %al, %rax
	leaq -40(%rbp), %r11
	movl %eax, (%r11)
	leaq -40(%rbp), %r11
	movslq (%r11), %rax
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
