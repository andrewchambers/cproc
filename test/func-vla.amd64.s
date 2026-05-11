	.text
	.globl f
	.balign 16
f:
	pushq %rbp
	movq %rsp, %rbp
	subq $48, %rsp
.LB1:
	movq %rdi, %rax
	leaq -4(%rbp), %r11
	movl %eax, (%r11)
	leaq -4(%rbp), %r11
	movslq (%r11), %rax
	leaq -8(%rbp), %r11
	movl %eax, (%r11)
	leaq -8(%rbp), %r11
	movslq (%r11), %rax
	leaq -16(%rbp), %r11
	movq %rax, (%r11)
	leaq -16(%rbp), %r11
	movq (%r11), %rax
	movq $8, %rcx
	imulq %rcx, %rax
	leaq -24(%rbp), %r11
	movq %rax, (%r11)
	movq %rsi, %rax
	leaq -32(%rbp), %r11
	movq %rax, (%r11)
.LB2:
	leaq -32(%rbp), %r11
	movq (%r11), %rax
	leaq -40(%rbp), %r11
	movq %rax, (%r11)
	leaq -24(%rbp), %r11
	movq (%r11), %rax
	leaq -44(%rbp), %r11
	movl %eax, (%r11)
	leaq -44(%rbp), %r11
	movslq (%r11), %rax
	jmp .Lret1
.Lret1:
	leave
	ret
	.text
	.globl main
	.balign 16
main:
	pushq %rbp
	movq %rsp, %rbp
	subq $64, %rsp
.LB3:
.LB4:
	movq $5, %rax
	movq %rax, %rdi
	leaq -40(%rbp), %rax
	movq %rax, %rsi
	movb $0, %al
	call f
	leaq -44(%rbp), %r11
	movl %eax, (%r11)
	leaq -44(%rbp), %r11
	movslq (%r11), %rax
	leaq -56(%rbp), %r11
	movq %rax, (%r11)
	leaq -56(%rbp), %r11
	movq (%r11), %rax
	movq $40, %rcx
	cmpq %rcx, %rax
	setne %al
	movzbq %al, %rax
	leaq -60(%rbp), %r11
	movl %eax, (%r11)
	leaq -60(%rbp), %r11
	movslq (%r11), %rax
	jmp .Lret2
.Lret2:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
