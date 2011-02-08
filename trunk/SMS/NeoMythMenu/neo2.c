#include "neo2.h"
#include "sms.h"



void neo2_asic_unlock()
{
    volatile BYTE dummy;

    // FFD200
    Neo2FlashBankLo = 0xFF;
    Frame1 = 0x06;
    dummy = *(volatile BYTE *)0x6400;

    // 001500
    Neo2FlashBankLo = 0x00;
    Frame1 = 0x00;
    dummy = *(volatile BYTE *)0x6A00;

    // 01D200
    Neo2FlashBankLo = 0x01;
    Frame1 = 0x06;
    dummy = *(volatile BYTE *)0x6400;

    // 021500
    Neo2FlashBankLo = 0x02;
    Frame1 = 0x00;
    dummy = *(volatile BYTE *)0x6A00;

    // FE1500
    Neo2FlashBankLo = 0xFE;
    Frame1 = 0x00;
    dummy = *(volatile BYTE *)0x6A00;
}


void neo2_asic_cmd(BYTE cmd, WORD data)
{
    volatile BYTE dummy;

    neo2_asic_unlock();

    Neo2FlashBankLo = cmd;
    Frame1 = (data >> 13) & 7;
    dummy = *(volatile BYTE *)(0x4000 | ((data & 0x1FFF) << 1));
}



void neo2_check_card()
{	
	volatile BYTE dummy;

	// CALL     SET_NEO_SW
	/*
SET_NEO_SW:
        LD      A,0FH       ;
        LD      (0BFC1H),A  ; ROM SIZE

        LD      A,001H      ;
        LD      (0BFD0H),A  ; $0~$3FFF READ FF
        RET
	*/
	Neo2FlashBankSize = 0x0f;
	Neo2Frame0We = 0x01;

/*
        LD       A,01H
        LD       HL,0BFC6H   ; NEO CARD CHECK
        LD       (HL),A

*/
	Neo2CardCheck = 0x01;

	//        CALL     CMD5
	neo2_asic_unlock();

	/*
CMD_FWE:

        LD      A,0E2H      ; E2 1500  ;GBA WE ON !
        LD       HL,0BFC0H  ;
        LD      (HL),A      ;

        LD      A,000H      ;
        LD     (WFFFE),A      ; $4000~$7FFF

        LD      HL,06A00H   ;
        LD      A,(HL)      ;
        RET
	*/
	Neo2FlashBankLo = 0xe2;
	Frame1 = 0x00;
	dummy = *(volatile BYTE *)0x6A00;

	neo2_asic_unlock();

	/*
CMD_37M:

        LD      A,037H     ;372003
        LD      HL,0BFC0H  ;
        LD      (HL),A     ;

        LD      A,001H
        LD     (WFFFE),A      ; $4000~$7FFF

        LD      HL,04006H  ;
        LD      A,(HL)     ;
        RET
*/
	Neo2FlashBankLo = 0x37;
	Frame1 = 0x01;
	dummy = *(volatile BYTE *)0x6A00;

	neo2_asic_unlock();
/*
CMD_EE:

        LD      A,0EEH     ;EE0630
        LD       HL,0BFC0H  ;
        LD      (HL),A     ;

        LD      A,000H
        LD     (WFFFE),A      ; $4000~$7FFF

        LD      HL,04C60H  ;
        LD      A,(HL)     ;
        RET
*/
	Neo2FlashBankLo = 0xee;
	Frame1 = 0x00;
	dummy = *(volatile BYTE *)0x4C60;

/*
        LD      A,01H
        LD      HL,0BFC5H    ; GBA CARD WE $4000~$7FFF ON
        LD      (HL),A
*/
	Neo2Frame1We = 0x01;

/*
        LD      HL,0BFC4H    ;
        LD      A,001H       ; SET GBA CARD A24 & A25 OUTPUT
        LD      (HL),A       ;
*/
	*(volatile BYTE*)0xbfc4 = 0x01;

	/*
        LD      A,000H       ;
        LD      (WFFFE),A      ; $4000~$7FFF
	*/
	Frame1 = 0x00;

	/*
        LD       A,000H      ;
        LD       HL,0BFC0H   ;  BANK0
        LD      (HL),A       ;
	*/
	Neo2FlashBankLo = 0x00;

	/*
        LD      A,90H
        LD      (04000H),A
        LD      (04001H),A
	*/
	CMFrm1Ctrl = 0x90;
	*(volatile BYTE*)0x4001 = 0x90;

	/*
        LD      A,(04002H)
        LD      (0C00CH),A
	*/
	dummy = *(volatile BYTE*)0x4002;
	*(volatile BYTE*)0xc00d = dummy;

	/*
        LD      A,(04003H)
        LD      (0C00DH),A
     */ 
	dummy = *(volatile BYTE*)0x4003;
	*(volatile BYTE*)0xc00d = dummy;

	/*
        LD      A,0FFH
        LD      (04000H),A
        LD      (04001H),A	
	*/
	CMFrm1Ctrl = 0xff;
	*(volatile BYTE*)0x4001 = 0xff;

	/*
        LD      A,00H
        LD      (0BFC5H),A   ; GBA CARD WE $4000~$7FFF OFF
	*/
	Neo2Frame1We = 0x00;

   // CALL     CMD5
	neo2_asic_unlock();

	//CALL     CMD_ID
	/*
CMD_ID:

        LD      A,090H      ; 90 35    ;CHIP ID ON !
        LD       HL,0BFC0H  ;
        LD      (HL),A      ;

        LD      A,001H      ;
        LD     (WFFFE),A      ; $4000~$7FFF

        LD      HL,06A00H   ;
        LD      A,(HL)      ;
        RET
	*/
	Neo2FlashBankLo = 0x90;
	Frame1 = 0x01;
	dummy = *(volatile BYTE *)0x6400;

/*
        LD       A,088H      ;
        LD       (0FFFCH),A   ; SET RAM ON
                              
        LD       HL,08000H
        LD       A,(HL)
        CP       034H
        JR       NZ,Z107
*/
	Frm2Ctrl = 0x88;

	if(!(CMFrm2Ctrl - 0x34))
	{
		/*
        INC      HL
        LD       A,(HL)
        CP       016H
        JR       NZ,Z107
        */
		if ( !((*(volatile BYTE *)(0x8001)) - 0x16) )
			goto loc_z107;

		/*
        INC      HL
        LD       A,(HL)
        CP       096H
        JR       NZ,Z107
		*/

		if ( !((*(volatile BYTE *)(0x8002)) - 0x96) )
			goto loc_z107;

		/*
        INC      HL
        LD       A,(HL)
        CP       024H
        JR       NZ,Z107
        LD       A,00H
        JP       Z202
        LD       A,01H
		*/

		if ( !((*(volatile BYTE *)(0x8003)) - 0x24) )
			goto loc_z107;
	}

	loc_z107:

	/*
        LD       HL,0C000H   ;
        LD       (HL),A       ;
     
        LD       A,000H       ;
        LD       (0FFFCH),A    ; SET RAM FFF

        CALL     CMD5
        CALL     CMD_IDX
        CALL     SET_NEO_SWX
	*/

	dummy = MemCtrlShdw;
	Frm2Ctrl = 0;
	neo2_asic_unlock();

	/*
CMD_ID:

        LD      A,090H      ; 90 35    ;CHIP ID ON !
        LD       HL,0BFC0H  ;
        LD      (HL),A      ;

        LD      A,001H      ;
        LD     (WFFFE),A      ; $4000~$7FFF

        LD      HL,06A00H   ;
        LD      A,(HL)      ;
        RET
	*/
	Neo2FlashBankLo = 0x90;
	Frame1 = 0x01;
	dummy = *(volatile BYTE *)0x6400;

	/*
SET_NEO_SWX:

        LD      A,00H       ;
        LD      (0BFC0H),A  ; ROM BANK
        LD      (0BFC1H),A  ; ROM SIZE
        LD      (0BFD0H),A  ; $0~$3FFF READ ON
        RET
	*/
	Neo2FlashBankLo = 0x00;
	Neo2FlashBankSize = 0x00;
	Neo2Frame0We = 0x00;
}

