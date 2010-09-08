	.file	1 "ttt.c"
	.section .mdebug.abiO64
	.previous
	.section .gcc_compiled_long32
	.previous
	.gnu_attribute 4, 1
	.text
	.align	2
	.align	3
	.globl	crc7
	.set	nomips16
	.ent	crc7
	.type	crc7, @function
crc7:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	
	li	$3,-2139095040			# 0xffffffff80800000
	ori	$3,$3,0x8080
	move	$5,$0
	move	$10,$0
	move	$2,$0
	li	$11,40			# 0x28
	.align	3
$L6:
	sll	$2,$2,1
	andi	$2,$2,0x00ff
	andi	$9,$3,0x80
	sll	$8,$2,24
	srl	$7,$3,24
	sll	$6,$3,31
	addiu	$5,$5,1
	sra	$8,$8,24
	beq	$9,$0,$L2
	srl	$3,$3,1

	lbu	$10,0($4)
	addiu	$4,$4,1
$L2:
	bltz	$8,$L9
	and	$7,$10,$7

$L3:
	bnel	$7,$0,$L4
	xori	$2,$2,0x9

$L4:
	bne	$5,$11,$L6
	or	$3,$3,$6

	j	$31
	nop

	.align	3
$L9:
	j	$L3
	xori	$2,$2,0x9

	.set	macro
	.set	reorder
	.end	crc7
	.size	crc7, .-crc7
	.ident	"GCC: (GNU) 4.4.0"
