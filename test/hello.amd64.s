	.data
	.balign 1
.Lstring.2:
	.byte 104
	.byte 101
	.byte 108
	.byte 108
	.byte 111
	.byte 0
	.text
	.globl main
	.balign 16
main:
	pushq %rbp
	movq %rsp, %rbp
	subq $16, %rsp
.LB1:
.LB2:
	leaq .Lstring.2(%rip), %rax
	leaq -8(%rbp), %r11
	movq %rax, (%r11)
	leaq -8(%rbp), %r11
	movq (%r11), %rax
	movq %rax, %rdi
	movb $0, %al
	call puts
	leaq -12(%rbp), %r11
	movl %eax, (%r11)
	movq $0, %rax
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
