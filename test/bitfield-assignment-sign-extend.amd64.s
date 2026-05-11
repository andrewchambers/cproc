	.text
	.globl main
	.balign 16
main:
	pushq %rbp
	movq %rsp, %rbp
	subq $32, %rsp
.LB1:
.LB2:
	leaq s(%rip), %rax
	leaq -8(%rbp), %r11
	movq %rax, (%r11)
	leaq -8(%rbp), %r11
	movq (%r11), %rax
	movq $0, %rcx
	addq %rcx, %rax
	leaq -16(%rbp), %r11
	movq %rax, (%r11)
	movq $15, %rcx
	movq $15, %rdx
	andq %rdx, %rcx
	leaq -16(%rbp), %r11
	movq (%r11), %r11
	movslq (%r11), %rax
	movq $-16, %rdx
	andq %rdx, %rax
	orq %rcx, %rax
	leaq -16(%rbp), %r11
	movq (%r11), %r11
	movl %eax, (%r11)
	leaq -16(%rbp), %r11
	movq (%r11), %r11
	movslq (%r11), %rax
	shlq $60, %rax
	sarq $60, %rax
	leaq -20(%rbp), %r11
	movl %eax, (%r11)
	movq $1, %rax
	negq %rax
	leaq -24(%rbp), %r11
	movl %eax, (%r11)
	leaq -20(%rbp), %r11
	movslq (%r11), %rax
	leaq -24(%rbp), %r11
	movslq (%r11), %rcx
	cmpq %rcx, %rax
	setne %al
	movzbq %al, %rax
	leaq -28(%rbp), %r11
	movl %eax, (%r11)
	leaq -28(%rbp), %r11
	movslq (%r11), %rax
	jmp .Lret1
.Lret1:
	leave
	ret
	.data
	.globl s
	.balign 4
s:
	.zero 4
	.section .note.GNU-stack,"",@progbits
