	.text
	.globl main
	.balign 16
main:
	pushq %rbp
	movq %rsp, %rbp
	subq $16, %rsp
.LB1:
.LB2:
	leaq x(%rip), %r11
	movslq (%r11), %rax
	leaq -4(%rbp), %r11
	movl %eax, (%r11)
	leaq -4(%rbp), %r11
	movslq (%r11), %rax
	jmp .Lret1
.Lret1:
	leave
	ret
	.data
	.globl x
	.balign 4
x:
	.zero 4
	.section .note.GNU-stack,"",@progbits
