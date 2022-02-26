.extern main

.global exit
exit:
 	movq $60, %rax
  	syscall

.global write
write:
	movq     $1, %rax    # sys_write call number 
	movq     $1, %rdi    # write to stdout (fd=1)
	movq     %rsp, %rsi  # use char on stack
	movq     $1, %rdx    # write 1 char
	syscall   
	ret

.global _start
_start:
	call main
	movq %rax, %rdi
	movq $60, %rax
	syscall
	