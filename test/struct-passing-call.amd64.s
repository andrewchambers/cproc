	.text
	.globl g
	.balign 16
g:
	pushq %rbp
	movq %rsp, %rbp
.LB1:
.LB2:
	leaq s(%rip), %rax
	movq %rax, %rdi
	movb $0, %al
	call f
	jmp .Lret1
.Lret1:
	leave
	ret
	.data
	.globl s
	.balign 4
s:
	.zero 4
	.section .note.GNU-stack,"",@progbits
