	.data
	.globl x
	.balign 1
x:
	.byte 104
	.byte 101
	.byte 108
	.byte 108
	.byte 111
	.byte 0
	.text
	.globl f
	.balign 16
f:
	pushq %rbp
	movq %rsp, %rbp
	subq $16, %rsp
.LB1:
.LB2:
	movq $104, %rax
	leaq -6(%rbp), %r11
	movb %al, (%r11)
	movq $101, %rax
	leaq -6(%rbp), %r11
	leaq 1(%r11), %r11
	movb %al, (%r11)
	movq $108, %rax
	leaq -6(%rbp), %r11
	leaq 2(%r11), %r11
	movb %al, (%r11)
	movq $108, %rax
	leaq -6(%rbp), %r11
	leaq 3(%r11), %r11
	movb %al, (%r11)
	movq $111, %rax
	leaq -6(%rbp), %r11
	leaq 4(%r11), %r11
	movb %al, (%r11)
	movq $0, %rax
	leaq -6(%rbp), %r11
	leaq 5(%r11), %r11
	movb %al, (%r11)
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
