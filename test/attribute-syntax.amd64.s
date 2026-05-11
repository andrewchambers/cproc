	.text
	.globl f
	.balign 16
f:
	pushq %rbp
	movq %rsp, %rbp
	subq $16, %rsp
.LB1:
	movq %rdi, %rax
	leaq -4(%rbp), %r11
	movl %eax, (%r11)
	movq %rsi, %rax
	leaq -8(%rbp), %r11
	movl %eax, (%r11)
.LB2:
	movq $0, %rax
	testq %rax, %rax
	jne .LB3
	jmp .LB4
.LB3:
.LB4:
.LB5:
.LB6:
	jmp .Lret1
.Lret1:
	leave
	ret
	.data
	.globl a
	.balign 4
a:
	.zero 4
	.data
	.globl b
	.balign 4
b:
	.zero 4
	.data
	.globl c
	.balign 4
c:
	.zero 4
	.data
	.globl d
	.balign 4
d:
	.zero 4
	.data
	.globl x
	.balign 4
x:
	.zero 4
	.data
	.globl array
	.balign 4
array:
	.zero 32
	.data
	.globl pointer
	.balign 8
pointer:
	.zero 8
	.section .note.GNU-stack,"",@progbits
