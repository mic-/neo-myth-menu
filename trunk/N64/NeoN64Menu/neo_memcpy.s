.section .text
.align 2
.set push
.set noreorder
.set noat

.global neo_memcpy_auto
.ent neo_memcpy_auto
	neo_memcpy_auto:

	ori $15,$4,0
	addu $15,$15,$6

	ori $14,$0,0xB000
	andi $12,$4,0xF0000000
	beq $14,$12,neo_memcpy_auto_psram	/*Don't ever write quads to psram!*/
	nop

	ori $13,$0,64
	blte $6,$13,neo_memcpy_auto8	/*no benefit at all*/
	nop

	ori $14,$15,0 				/*find best addr*/
	addiu $14,$14,-8

	bgtz $14,neo_memcpy_auto64
	nop

	addiu $14,$14,4

	bgtz $14,neo_memcpy_auto32
	nop

	addiu $14,$14,2

	bgtz $14,neo_memcpy_auto16
	nop
										/*..*/
	j neo_memcpy_auto8
	nop

	neo_memcpy_auto64:
	0:
		ld $8,0($5)
		sd $8,0($4)
		addiu $4,$4,8
		addiu $5,$5,8
	bne $6,$14,0b
	nop

	j neo_memcpy_auto8
	nop

	neo_memcpy_auto32:
	0:
		lw $8,0($5)
		sw $8,0($4)
		addiu $4,$4,4
		addiu $5,$5,4
	bne $6,$14,0b
	nop

	j neo_memcpy_auto8
	nop

	neo_memcpy_auto16:
	0:
		lh $8,0($5)
		sh $8,0($4)
		addiu $4,$4,2
		addiu $5,$5,2
	bne $6,$14,0b
	nop

	j neo_memcpy_auto8
	nop

	neo_memcpy_auto_psram:
	andi $8,$6,3
	bgtz $8,neo_memcpy_auto8_psram
	nop

	0:
		lw $8,0($5)
		lbu $9,(0xB0000000)
		sw $8,0($4)
		addiu $4,$4,1
		addiu $5,$5,1
	bne $4,$15,0b
	nop

	jr $ra
	nop

	neo_memcpy_auto8_psram: /*Anything that's left out or not aligned*/
	0:
		lbu $8,0($5)
		lbu $9,(0xB0000000)
		sb $8,0($4)
		addiu $4,$4,1
		addiu $5,$5,1
	bne $4,$15,0b
	nop

	jr $ra
	nop

	neo_memcpy_auto8: /*Anything that's left out or not aligned*/
	0:
		lbu $8,0($5)
		sb $8,0($4)
		addiu $4,$4,1
		addiu $5,$5,1
	bne $4,$15,0b
	nop

	jr $ra
	nop

.end neo_memcpy_auto

.set pop
.set reorder
.set at


