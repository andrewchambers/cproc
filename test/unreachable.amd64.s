	.text
	.globl f
	.balign 16
f:
	pushq %rbp
	movq %rsp, %rbp
	subq $16, %rsp
.LB1:
	movq %rdi, %rax
	leaq -4(%rbp), %r11
	movl %eax, (%r11)
.LB2:
	jmp .Lret1
.LB5:
	leaq -4(%rbp), %r11
	movslq (%r11), %rax
	leaq -8(%rbp), %r11
	movl %eax, (%r11)
	jmp .LB3
.LB6:
	jmp .LB4
.LB3:
	leaq -8(%rbp), %r11
	movslq (%r11), %rax
	movq $0, %rcx
	cmpq %rcx, %rax
	sete %al
	movzbq %al, %rax
	leaq -12(%rbp), %r11
	movl %eax, (%r11)
	leaq -12(%rbp), %r11
	movslq (%r11), %rax
	testq %rax, %rax
	jne .LB6
	jmp .LB7
.LB7:
	leaq -8(%rbp), %r11
	movl (%r11), %eax
	movq $0, %rcx
	cmpq %rcx, %rax
	setb %al
	movzbq %al, %rax
	leaq -16(%rbp), %r11
	movl %eax, (%r11)
	leaq -16(%rbp), %r11
	movslq (%r11), %rax
	testq %rax, %rax
	jne .LB8
	jmp .LB9
.LB8:
	jmp .LB4
.LB9:
	jmp .LB4
.LB4:
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
