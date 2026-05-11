	.text
	.globl main
	.balign 16
main:
	pushq %rbp
	movq %rsp, %rbp
	subq $16, %rsp
.LB1:
.LB2:
	movq $0, %rax
	leaq -4(%rbp), %r11
	movl %eax, (%r11)
	movq $1, %rax
	negq %rax
	leaq -8(%rbp), %r11
	movl %eax, (%r11)
	leaq -4(%rbp), %r11
	movl (%r11), %eax
	leaq -8(%rbp), %r11
	movl (%r11), %ecx
	cmpq %rcx, %rax
	seta %al
	movzbq %al, %rax
	leaq -12(%rbp), %r11
	movl %eax, (%r11)
	leaq -12(%rbp), %r11
	movslq (%r11), %rax
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
