	.data
	.balign 8
.La.2:
	.zero 24
	.data
	.balign 8
.Lb.3:
	.zero 24
	.text
	.globl f1
	.balign 16
f1:
	pushq %rbp
	movq %rsp, %rbp
.LB1:
.LB2:
	leaq .La.2(%rip), %rdi
	leaq .Lb.3(%rip), %rsi
	movq 0(%rsi), %rax
	movq %rax, 0(%rdi)
	movq 8(%rsi), %rax
	movq %rax, 8(%rdi)
	movq 16(%rsi), %rax
	movq %rax, 16(%rdi)
	jmp .Lret1
.Lret1:
	leave
	ret
	.data
	.balign 8
.La.5:
	.zero 24
	.text
	.globl f2
	.balign 16
f2:
	pushq %rbp
	movq %rsp, %rbp
	subq $16, %rsp
.LB3:
	movq %rdi, %rax
	leaq -8(%rbp), %r11
	movq %rax, (%r11)
.LB4:
	leaq -8(%rbp), %r11
	movq (%r11), %rax
	leaq -16(%rbp), %r11
	movq %rax, (%r11)
	leaq .La.5(%rip), %rdi
	leaq -16(%rbp), %r11
	movq (%r11), %rsi
	movq 0(%rsi), %rax
	movq %rax, 0(%rdi)
	movq 8(%rsi), %rax
	movq %rax, 8(%rdi)
	movq 16(%rsi), %rax
	movq %rax, 16(%rdi)
	jmp .Lret2
.Lret2:
	leave
	ret
	.text
	.globl f3
	.balign 16
f3:
	pushq %rbp
	movq %rsp, %rbp
	subq $32, %rsp
.LB5:
	movq %rdi, %rax
	leaq -8(%rbp), %r11
	movq %rax, (%r11)
	movq %rsi, %rax
	leaq -16(%rbp), %r11
	movq %rax, (%r11)
.LB6:
	leaq -16(%rbp), %r11
	movq (%r11), %rax
	leaq -24(%rbp), %r11
	movq %rax, (%r11)
	leaq -8(%rbp), %r11
	movq (%r11), %rax
	leaq -32(%rbp), %r11
	movq %rax, (%r11)
	leaq -32(%rbp), %r11
	movq (%r11), %rdi
	leaq -24(%rbp), %r11
	movq (%r11), %rsi
	movq 0(%rsi), %rax
	movq %rax, 0(%rdi)
	movq 8(%rsi), %rax
	movq %rax, 8(%rdi)
	movq 16(%rsi), %rax
	movq %rax, 16(%rdi)
	jmp .Lret3
.Lret3:
	leave
	ret
	.section .note.GNU-stack,"",@progbits
