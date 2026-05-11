	.data
	.globl s
	.balign 8
s:
	.zero 16
	.text
	.globl f
	.balign 16
f:
	pushq %rbp
	movq %rsp, %rbp
	subq $16, %rsp
.LB1:
.LB2:
	xorq %rax, %rax
	leaq -16(%rbp), %r11
	movq %rax, (%r11)
	leaq -16(%rbp), %r11
	leaq 8(%r11), %r11
	movq %rax, (%r11)
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
