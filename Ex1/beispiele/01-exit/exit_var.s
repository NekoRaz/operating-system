.section .data
returnValue: .byte 23

.section .text
.globl _start
_start:
	mov $60, %rax
	mov (returnValue), %rdi
	syscall
