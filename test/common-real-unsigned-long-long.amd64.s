	.text
	.globl main
	.balign 16
main:
	pushq %rbp
	movq %rsp, %rbp
	subq $48, %rsp
.LB1:
.LB2:
	movq $0, %rax
	leaq -8(%rbp), %r11
	movq %rax, (%r11)
	movq $-1, %rax
	leaq -8(%rbp), %r11
	movq (%r11), %rcx
	orq %rcx, %rax
	leaq -16(%rbp), %r11
	movq %rax, (%r11)
	leaq -16(%rbp), %r11
	movq (%r11), %rax
	leaq -24(%rbp), %r11
	movq %rax, (%r11)
	movq $7433582362234655811, %rax
	leaq -32(%rbp), %r11
	movq %rax, (%r11)
	leaq -24(%rbp), %r11
	movq (%r11), %rax
	leaq -32(%rbp), %r11
	movq (%r11), %rcx
	cmpq %rcx, %rax
	setbe %al
	movzbq %al, %rax
	leaq -36(%rbp), %r11
	movl %eax, (%r11)
	leaq -36(%rbp), %r11
	movslq (%r11), %rax
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
