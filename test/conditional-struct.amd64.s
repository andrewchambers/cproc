	.text
	.globl main
	.balign 16
main:
	pushq %rbp
	movq %rsp, %rbp
	subq $48, %rsp
.LB1:
.LB2:
	movq $1, %rax
	leaq -4(%rbp), %r11
	movl %eax, (%r11)
	movq $2, %rax
	leaq -8(%rbp), %r11
	movl %eax, (%r11)
	leaq -4(%rbp), %rax
	movq $0, %rcx
	addq %rcx, %rax
	leaq -16(%rbp), %r11
	movq %rax, (%r11)
	leaq -16(%rbp), %r11
	movq (%r11), %r11
	movslq (%r11), %rax
	leaq -20(%rbp), %r11
	movl %eax, (%r11)
	leaq -20(%rbp), %r11
	movslq (%r11), %rax
	testq %rax, %rax
	jne .LB3
	jmp .LB4
.LB3:
	leaq -24(%rbp), %rdi
	leaq -8(%rbp), %rsi
	movl 0(%rsi), %eax
	movl %eax, 0(%rdi)
	jmp .LB5
.LB4:
	leaq -24(%rbp), %rdi
	leaq -4(%rbp), %rsi
	movl 0(%rsi), %eax
	movl %eax, 0(%rdi)
	jmp .LB5
.LB5:
	leaq -24(%rbp), %r11
	movq (%r11), %rax
	leaq -32(%rbp), %r11
	movq %rax, (%r11)
	leaq -32(%rbp), %r11
	movq (%r11), %rax
	movq $0, %rcx
	addq %rcx, %rax
	leaq -40(%rbp), %r11
	movq %rax, (%r11)
	leaq -40(%rbp), %r11
	movq (%r11), %r11
	movslq (%r11), %rax
	leaq -44(%rbp), %r11
	movl %eax, (%r11)
	leaq -44(%rbp), %r11
	movslq (%r11), %rax
	movq $2, %rcx
	cmpq %rcx, %rax
	setne %al
	movzbq %al, %rax
	leaq -48(%rbp), %r11
	movl %eax, (%r11)
	leaq -48(%rbp), %r11
	movslq (%r11), %rax
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
