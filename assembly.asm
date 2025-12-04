.data
	a:	.byte 0
	b:	.byte 0
	c:	.byte 0
.code
main:
	daddiu r1, r0, 1
	sb r1, a(r0)
	daddiu r1, r0, 2
	sb r1, b(r0)
	lb r1, a(r0)
	lb r2, b(r0)
	daddu r1, r1, r2
	sb r1, c(r0)
