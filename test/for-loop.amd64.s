	.text
	.globl f
	.balign 16
f:
	pushq %rbp
	movq %rsp, %rbp
	subq $32, %rsp
.LB1:
.LB2:
	movq $0, %rax
	leaq -4(%rbp), %r11
	movl %eax, (%r11)
.LB3:
	leaq -4(%rbp), %r11
	movslq (%r11), %rax
	leaq -8(%rbp), %r11
	movl %eax, (%r11)
	leaq -8(%rbp), %r11
	movslq (%r11), %rax
	movq $10, %rcx
	cmpq %rcx, %rax
	setl %al
	movzbq %al, %rax
	leaq -12(%rbp), %r11
	movl %eax, (%r11)
	leaq -12(%rbp), %r11
	movslq (%r11), %rax
	testq %rax, %rax
	jne .LB4
	jmp .LB6
.LB4:
	leaq -4(%rbp), %r11
	movslq (%r11), %rax
	leaq -16(%rbp), %r11
	movl %eax, (%r11)
	leaq -16(%rbp), %r11
	movslq (%r11), %rax
	movq %rax, %rdi
	movb $0, %al
	call g
.LB5:
	leaq -4(%rbp), %r11
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
	leaq -4(%rbp), %r11
	movl %eax, (%r11)
	jmp .LB3
.LB6:
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
