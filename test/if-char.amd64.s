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
	jne .LB3
	jmp .LB4
.LB3:
	movq $1, %rax
	jmp .Lret1
.LB4:
	movq $0, %rax
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
