.global test_and_set
test_and_set:
	pop %ecx
	pop %eax
	pop %edx
	lock cmpxchg %ecx, (%edx)
	jz test_and_set_succ
	mov $0, %eax
	ret
test_and_set_succ:
	mov $1, %eax
	ret
	