	.data
	.balign 1
.Lstring.2:
	.byte 0
	.byte 49
	.byte 0
	.text
	.globl main
	.balign 16
main:
	pushq %rbp
	movq %rsp, %rbp
	subq $32, %rsp
.LB1:
.LB2:
	movq $0, %rax
	leaq -8(%rbp), %r11
	movq %rax, (%r11)
	leaq -8(%rbp), %r11
	movq (%r11), %rax
	movq $1, %rcx
	imulq %rcx, %rax
	leaq -16(%rbp), %r11
	movq %rax, (%r11)
	leaq .Lstring.2(%rip), %rax
	leaq -16(%rbp), %r11
	movq (%r11), %rcx
	addq %rcx, %rax
	leaq -24(%rbp), %r11
	movq %rax, (%r11)
	leaq -24(%rbp), %r11
	movq (%r11), %r11
	movsbq (%r11), %rax
	leaq -25(%rbp), %r11
	movb %al, (%r11)
	leaq -25(%rbp), %r11
	movsbq (%r11), %rax
	leaq -32(%rbp), %r11
	movl %eax, (%r11)
	leaq -32(%rbp), %r11
	movslq (%r11), %rax
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
