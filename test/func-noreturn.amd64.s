	.data
	.balign 4
.Lc.2:
	.zero 4
	.data
	.balign 1
.Lstring.3:
	.byte 108
	.byte 111
	.byte 111
	.byte 112
	.byte 0
	.text
	.globl f
	.balign 16
f:
	pushq %rbp
	movq %rsp, %rbp
	subq $32, %rsp
.LB1:
.LB2:
.LB3:
	leaq .Lc.2(%rip), %r11
	movslq (%r11), %rax
	leaq -4(%rbp), %r11
	movl %eax, (%r11)
	leaq -4(%rbp), %r11
	movslq (%r11), %rax
	testq %rax, %rax
	jne .LB4
	jmp .LB5
.LB4:
	leaq .Lstring.3(%rip), %rax
	leaq -16(%rbp), %r11
	movq %rax, (%r11)
	leaq -16(%rbp), %r11
	movq (%r11), %rax
	movq %rax, %rdi
	movb $0, %al
	call puts
	leaq -20(%rbp), %r11
	movl %eax, (%r11)
	jmp .LB3
.LB5:
	ud2
.Lret1:
	leave
	ret
	.text
	.globl main
	.balign 16
main:
	pushq %rbp
	movq %rsp, %rbp
.LB6:
.LB7:
	movq $0, %rax
	movq %rax, %rdi
	movb $0, %al
	call exit
	ud2
.LB8:
	movb $0, %al
	call f
	ud2
.LB9:
	movb $0, %al
	call f
	ud2
.Lret2:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
