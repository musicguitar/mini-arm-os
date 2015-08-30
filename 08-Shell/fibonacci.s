.syntax unified
.text
.align 2
.thumb
.thumb_func
.global fibonacci

fibonacci:
    push { r4, r5, r6, r7, r8, r9, lr}

    cmp r0,#0              @if n <=0,f(n)=0
    ble r_0

    mov r4,#1              @a=1
    mov r5,#1              @b=1

    clz r6,r0              @count leading zero of r0
    rsbs r6,r6,#30         @using r6 as "i"
                           @(start looking second digit ,skip m=0)

    blt exit               @when r0 = 1 or 2 ,skip

for_loop:
    lsl r7,r5,#1           @"t1"=r7=2*b
    sub r7,r7,r4           @t1=r7=2*b-a
    mul r7,r7,r4           @t1=a*(2*b-a)

    mul r9,r5,r5           @r9=temp=b^2
    mla r5,r4,r4,r9        @b="t2"=r5=temp+a^2=b^2+a^2

    mov r4,r7              @a=t1

check_odd:
    lsr r9,r0,r6           @ temp(r9)=n(r0)>>i(r6)
    tst r9,#1              @((n >> i) & 1) 
    beq finish_once        @<((n >> i) & 1) != 0> is false

    add r7,r4,r5           @t1=a+b
    mov r4,r5              @a=b
    mov r5,r7              @b=t1

finish_once:
    cbz r6,exit            @if i==0,exit
    sub r6,r6,#1           @i!=0,i=i-1
    b for_loop

exit:
    mov r0,r4
	pop { r4, r5, r6, r7, r8, r9, pc}

r_0:
    mov r0,#0
    pop { r4, r5, r6, r7, r8, r9, pc}

