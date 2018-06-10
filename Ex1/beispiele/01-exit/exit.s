.section .text
.globl _start
_start:
	mov $60, %eax # Function no 60: exit()
	mov  $0, %edi # Return Value 0
	syscall       # call the kernel
