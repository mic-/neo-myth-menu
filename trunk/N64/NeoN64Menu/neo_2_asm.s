
/*Interface : neo_2_asm.h*/

.section .text

.align 2 /*.align 3*/
.set push
.set noreorder
.set noat

.global neo2_recv_sd_multi /*ChillyWilly's MASTERPIECE*/
.ent    neo2_recv_sd_multi
		neo2_recv_sd_multi:

        la $15,0xB30E6060         /* $15 = 0xB30E6060*/
        ori $14,$4,0              /* $14 = buf*/
        ori $12,$5,0              /* $12 = count*/
		
		/*
			t8 = 0xF000 , t9 = 0x0F00 (not saved)
			s0 = 0x00F0 , s1 = 0x000F (saved)
		*/
		addiu $sp,$sp,-8 /*this block adds lots of latency to the prologue but in the end we get a few ms!*/
		sw $16,0($sp)
		sw $17,4($sp)
		lui $24,0xF000
		lui $25,0x0F00
		lui $16,0x00F0
		lui $17,0x000F
 
        oloop:
        lui $11,0x0001            /* $11 = timeout = 64 * 1024*/

        tloop:
        lw $2,($15)               /* rdMmcDatBit4*/
        andi $2,$2,0x0100         /* eqv of (data>>8)&0x01*/
        beq $2,$0,getsect         /* start bit detected*/
        nop
        addiu $11,$11,-1
        bne $11,$0,tloop          /* not timed out*/
        nop
        beq $11,$0,___exit        /* timeout*/
        ori $2,$0,0               /* res = FALSE*/

        getsect:
        ori $13,$0,128            /* $13 = long count*/

        gsloop:
        lw $2,($15)               /* rdMmcDatBit4 => -a-- -a--*/
        /*lui $10,0xF000*/            /* $10 = mask = 0xF0000000*/
        sll $2,$2,4               /* a--- a---*/

        lw $3,($15)               /* rdMmcDatBit4 => -b-- -b--*/
        and $2,$2,$24             /* a000 0000*/
        /*lui $10,0x0F00*/            /* $10 = mask = 0x0F000000*/
        and $3,$3,$25             /* 0b00 0000*/

        lw $4,($15)               /* rdMmcDatBit4 => -c-- -c--*/
        /*lui $10,0x00F0*/            /* $10 = mask = 0x00F00000*/
        or $11,$3,$2              /* $11 = ab00 0000*/
        srl $4,$4,4               /* --c- --c-*/

        lw $5,($15)               /* rdMmcDatBit4 => -d-- -d--*/
        and $4,$4,$16             /* 00c0 0000*/
        /*lui $10,0x000F*/            /* $10 = mask = 0x000F0000*/
        srl $5,$5,8               /* ---d ---d*/
        or $11,$11,$4             /* $11 = abc0 0000*/

        lw $6,($15)               /* rdMmcDatBit4 => -e-- -e--*/
        and $5,$5,$17             /* 000d 0000*/
        /*ori $10,$0,0xF000*/         /* $10 = mask = 0x0000F000*/
        sll $6,$6,4               /* e--- e---*/
        or $11,$11,$5             /* $11 = abcd 0000*/

        lw $7,($15)               /* rdMmcDatBit4 => -f-- -f--*/
        andi $6,$6,0xF000             /* 0000 e000*/
        /*ori $10,$0,0x0F00*/         /* $10 = mask = 0x00000F00*/
        or $11,$11,$6             /* $11 = abcd e000*/
        and $7,$7,0x0F00             /* 0000 0f00*/

        lw $8,($15)               /* rdMmcDatBit4 => -g-- -g--*/
        /*ori $10,$0,0x00F0*/         /* $10 = mask = 0x000000F0*/
        or $11,$11,$7             /* $11 = abcd ef00*/
        srl $8,$8,4               /* --g- --g-*/

        lw $9,($15)               /* rdMmcDatBit4 => -h-- -h--*/
        andi $8,$8,0x00F0            /* 0000 00g0*/
        /*ori $10,$0,0x000F*/         /* $10 = mask = 0x000000F*/
        or $11,$11,$8             /* $11 = abcd efg0*/

        srl $9,$9,8               /* ---h ---h*/
        andi $9,$9,0x000F             /* 0000 000h*/
        or $11,$11,$9             /* $11 = abcd efgh*/

        sw $11,0($14)             /* save sector data*/
        addiu $13,$13,-1
        bne $13,$0,gsloop
        addiu $14,$14,4           /* inc buffer pointer */

        lw $2,($15)               /* rdMmcDatBit4 - just toss checksum bytes */
        lw $2,($15)               /* rdMmcDatBit4*/
        lw $2,($15)               /* rdMmcDatBit4*/
        lw $2,($15)               /* rdMmcDatBit4*/
        lw $2,($15)               /* rdMmcDatBit4*/
        lw $2,($15)               /* rdMmcDatBit4*/
        lw $2,($15)               /* rdMmcDatBit4*/
        lw $2,($15)               /* rdMmcDatBit4*/
        lw $2,($15)               /* rdMmcDatBit4*/
        lw $2,($15)               /* rdMmcDatBit4*/
        lw $2,($15)               /* rdMmcDatBit4*/
        lw $2,($15)               /* rdMmcDatBit4*/
        lw $2,($15)               /* rdMmcDatBit4*/
        lw $2,($15)               /* rdMmcDatBit4*/
        lw $2,($15)               /* rdMmcDatBit4*/
        lw $2,($15)               /* rdMmcDatBit4*/

        lw $2,($15)               /* rdMmcDatBit4 - clock out end bit*/

        addiu $12,$12,-1          /* count--*/
        bne $12,$0,oloop          /* next sector*/
        nop

        ori $2,$0,1                 /* res = TRUE*/

		___exit:
		lw $16,0($sp)
		lw $17,4($sp)
		addiu $sp,$sp,8

		jr $ra
		nop

.end neo2_recv_sd_multi

.global neo_xferto_psram
.ent    neo_xferto_psram
		neo_xferto_psram:

	la $10,0xB0000000
	ori $8,$4,0
	addu $8,$8,$6
	addu $10,$10,$5
	
	0:
		lw $12,0($4)
		lbu $11,0($10)
		sw $12,0($10)
		
		addiu $10,$10,4
		addiu $4,$4,4

		bne $4,$8,0b
		nop	

	jr $ra
	nop

.end neo_xferto_psram

.global neo_memcpy64
.ent    neo_memcpy64
		neo_memcpy64:

	ori $8,$5,0
	addu $8,$8,$6

	0:
		ld $9,($5)
		sd $9,($4)
		addiu $4,$4,8
		addiu $5,$5,8

	bne $5,$8,0b
	nop

	jr $ra
	nop
.end neo_memcpy64

.global neo_memcpy32
.ent    neo_memcpy32
		neo_memcpy32:

	ori $8,$5,0
	addu $8,$8,$6

	0:
		lw $9,($5)
		sw $9,($4)
		addiu $4,$4,4
		addiu $5,$5,4

	bne $5,$8,0b
	nop

	jr $ra
	nop
.end neo_memcpy32

.set pop
.set reorder
.set at


