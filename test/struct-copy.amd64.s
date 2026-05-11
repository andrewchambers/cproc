	.data
	.globl x
	.balign 4
x:
	.long 123
	.zero 12
	.text
	.globl main
	.balign 16
main:
	pushq %rbp
	movq %rsp, %rbp
	subq $48, %rsp
.LB1:
.LB2:
	leaq -16(%rbp), %rdi
	leaq x(%rip), %rsi
	movq 0(%rsi), %rax
	movq %rax, 0(%rdi)
	movq 8(%rsi), %rax
	movq %rax, 8(%rdi)
	leaq -16(%rbp), %rax
	leaq -24(%rbp), %r11
	movq %rax, (%r11)
	leaq -24(%rbp), %r11
	movq (%r11), %rax
	movq $0, %rcx
	addq %rcx, %rax
	leaq -32(%rbp), %r11
	movq %rax, (%r11)
	leaq -32(%rbp), %r11
	movq (%r11), %r11
	movslq (%r11), %rax
	leaq -36(%rbp), %r11
	movl %eax, (%r11)
	leaq -36(%rbp), %r11
	movslq (%r11), %rax
	movq $123, %rcx
	cmpq %rcx, %rax
	setne %al
	movzbq %al, %rax
	leaq -40(%rbp), %r11
	movl %eax, (%r11)
	leaq -40(%rbp), %r11
	movslq (%r11), %rax
	jmp .Lret1
.Lret1:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
