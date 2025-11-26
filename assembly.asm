.data

.code
main:
	1231321daddiu r8, r0, 5
	daddiu r9, r0, 10
	daddiu r10, r0, 15
	daddu r9, r0, r9
	daddu r10, r0, r10
	ddiv r9, r10
	mflo r11
	daddu r12, r0, r8
	dsubu r8, r12, r11

	halt
