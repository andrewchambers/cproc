	.text
	.globl main
	.balign 16
main:
	pushq %rbp
	movq %rsp, %rbp
	subq $32, %rsp
.LB1:
.LB2:
	movq $1, %rax
	leaq -1(%rbp), %r11
	movb %al, (%r11)
	leaq -1(%rbp), %r11
	movzbq (%r11), %rax
	leaq -8(%rbp), %r11
	movl %eax, (%r11)
	movq $2, %rax
	leaq -9(%rbp), %r11
	movb %al, (%r11)
	leaq -9(%rbp), %r11
	movzbq (%r11), %rax
	leaq -16(%rbp), %r11
	movl %eax, (%r11)
	leaq -8(%rbp), %r11
	movslq (%r11), %rax
	leaq -16(%rbp), %r11
	movslq (%r11), %rcx
	subq %rcx, %rax
	leaq -20(%rbp), %r11
	movl %eax, (%r11)
	leaq -20(%rbp), %r11
	movslq (%r11), %rax
	movq $0, %rcx
	cmpq %rcx, %rax
	setg %al
	movzbq %al, %rax
	leaq -24(%rbp), %r11
	movl %eax, (%r11)
	leaq -24(%rbp), %r11
	movslq (%r11), %rax
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
