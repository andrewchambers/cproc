	.text
	.globl main
	.balign 16
main:
	pushq %rbp
	movq %rsp, %rbp
	subq $16, %rsp
.LB1:
.LB2:
	movq $256, %rax
	leaq -1(%rbp), %r11
	movb %al, (%r11)
	leaq -1(%rbp), %r11
	movzbq (%r11), %rax
	testq %rax, %rax
	setne %al
	movzbq %al, %rax
	leaq -2(%rbp), %r11
	movb %al, (%r11)
	leaq -2(%rbp), %r11
	movzbq (%r11), %rax
	leaq -8(%rbp), %r11
	movl %eax, (%r11)
	leaq -8(%rbp), %r11
	movslq (%r11), %rax
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
