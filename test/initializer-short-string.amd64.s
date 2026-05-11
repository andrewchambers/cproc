	.text
	.globl f
	.balign 16
f:
	pushq %rbp
	movq %rsp, %rbp
	subq $16, %rsp
.LB1:
.LB2:
	movq $97, %rax
	leaq -10(%rbp), %r11
	movb %al, (%r11)
	movq $98, %rax
	leaq -10(%rbp), %r11
	leaq 1(%r11), %r11
	movb %al, (%r11)
	movq $99, %rax
	leaq -10(%rbp), %r11
	leaq 2(%r11), %r11
	movb %al, (%r11)
	movq $0, %rax
	leaq -10(%rbp), %r11
	leaq 3(%r11), %r11
	movb %al, (%r11)
	xorq %rax, %rax
	leaq -10(%rbp), %r11
	leaq 4(%r11), %r11
	movl %eax, (%r11)
	leaq -10(%rbp), %r11
	leaq 8(%r11), %r11
	movw %ax, (%r11)
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
