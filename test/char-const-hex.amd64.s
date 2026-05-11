	.text
	.globl main
	.balign 16
main:
	pushq %rbp
	movq %rsp, %rbp
	subq $16, %rsp
.LB1:
.LB2:
	movq $1, %rax
	negq %rax
	leaq -4(%rbp), %r11
	movl %eax, (%r11)
	leaq -4(%rbp), %r11
	movslq (%r11), %rax
	leaq -5(%rbp), %r11
	movb %al, (%r11)
	leaq -5(%rbp), %r11
	movsbq (%r11), %rax
	leaq -12(%rbp), %r11
	movl %eax, (%r11)
	movq $-1, %rax
	leaq -12(%rbp), %r11
	movslq (%r11), %rcx
	cmpq %rcx, %rax
	setne %al
	movzbq %al, %rax
	leaq -16(%rbp), %r11
	movl %eax, (%r11)
	leaq -16(%rbp), %r11
	movslq (%r11), %rax
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
