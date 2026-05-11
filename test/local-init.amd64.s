	.text
	.globl f
	.balign 16
f:
	pushq %rbp
	movq %rsp, %rbp
	subq $48, %rsp
.LB1:
.LB2:
	movq $97, %rax
	leaq -33(%rbp), %r11
	movb %al, (%r11)
	leaq -33(%rbp), %r11
	movsbq (%r11), %rax
	leaq -32(%rbp), %r11
	movb %al, (%r11)
	xorq %rax, %rax
	leaq -32(%rbp), %r11
	leaq 1(%r11), %r11
	movq %rax, (%r11)
	leaq -32(%rbp), %r11
	leaq 9(%r11), %r11
	movq %rax, (%r11)
	leaq -32(%rbp), %r11
	leaq 17(%r11), %r11
	movq %rax, (%r11)
	leaq -32(%rbp), %r11
	leaq 25(%r11), %r11
	movl %eax, (%r11)
	leaq -32(%rbp), %r11
	leaq 29(%r11), %r11
	movw %ax, (%r11)
	leaq -32(%rbp), %r11
	leaq 31(%r11), %r11
	movb %al, (%r11)
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
