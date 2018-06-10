.section .data
hello: .ascii "Hello World!\n"

.section .text

.globl _start
_start:
	
	mov $1, %rax     # Function write()
	mov $1, %rdi     # File Descriptor
	mov $hello, %rsi # Memory Adress
	mov $13, %rdx    # Text Length
	syscall          # call the kernel
	
	mov $60, %rax    # Function exit()
	mov  $0, %rdi    # Exitcode 0
	syscall          # call the kernel
