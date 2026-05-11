	.data
	.globl x
	.balign 4
x:
	.long 0
	.text
	.globl main
	.balign 16
main:
	pushq %rbp
	movq %rsp, %rbp
	subq $16, %rsp
.LB1:
.LB2:
	movq $1, %rax
	negq %rax
	leaq -4(%rbp), %r11
	movl %eax, (%r11)
	leaq -4(%rbp), %r11
	movslq (%r11), %rax
	movq $0, %rcx
	cmpq %rcx, %rax
	setg %al
	movzbq %al, %rax
	leaq -8(%rbp), %r11
	movl %eax, (%r11)
	leaq -8(%rbp), %r11
	movslq (%r11), %rax
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
