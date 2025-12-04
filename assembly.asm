.data
a:	.byte 0
b:	.byte 0

.code
main:
	daddiu r8, r0, 1
	sb r8, a(r0)
	daddiu r8, r0, 2
	sb r8, b(r0)
