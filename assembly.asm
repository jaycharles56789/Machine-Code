# reserve memory for x
daddi r1, r0, 5
# load immediate 5 into r1
sb r1, x(r0)
# store r1 into variable x

# reserve memory for y
# reserve memory for z
lb r2, x(r0)
# load x into r2
lb r4, y(r0)
# load y into r4
dsub r1, r2, r4
# r1 = x - y
sb r1, x(r0)
# store result into x

