
;	opcode subcode	name	parameters		description
;	0b0000  0b000	ADD 	ra rb 	rc 		-> ra = rb+rc
;	0b0000  0b001	ADC 	ra rb 	rc 		-> ra = rb+rc+c
;	0b0000  0b010	SUB 	ra rb 	rc 		-> ra = rb-rc
;	0b0000  0b011	SUBC 	ra rb 	rc 		-> ra = rb-rc-c
;	0b0000  0b100	NAND 	ra rb 	rc 		-> ra = !(rb&rc)
;	0b0000  0b101	AND 	ra rb 	rc 		-> ra = rb&rc
;	0b0000  0b110	OR 		ra rb 	rc 		-> ra = rb|rc
;	0b0000  0b111	XOR     ra rb   rc      -> ra = rb^rc
;	0b0001   N/A 	ADDI 	ra rb 	imm 	-> ra = rb+imm
;	0b0010	 N/A 	BS 		ra rb 	imm 	-> imm>=32 ? ra = rb<<imm : ra = rb>>imm 
;	0b0011	 N/A 	SW 		ra rb 	imm 	-> mem[rb+imm] = ra
;	0b0100	 N/A 	LW 		ra rb 	imm 	-> ra = mem[rb+imm]
;	0b0101	 N/A 	CMP 	ra rb 			-> compare ra and rb and update status register
;	0b0110	 N/A 	JALR 	ra rb 			-> set PC to rb and store PC+1 in ra
;	0b0111	 N/A 	BEQ 	ra rb   imm 	-> set PC to rb + imm and store PC+1 in ra if EQ flag is set
;	0b1000	 N/A 	BGR		ra rb   imm 	-> set PC to rb + imm and store PC+1 in ra if GR flag is set
;	0b1110	 N/A 	LUI 	ra imm 			-> ra = imm<<6 this instruction is special because it only uses 3 bits in order to fit a 10 bit immediate. the format is 0b111|reg|imm|

; base instructions
#ruledef {
	add  r{reg_a:u3} r{reg_b:u3} r{reg_c:u3}		=> 0x0`4 @ reg_a @ reg_b @ 0b000 @ reg_c
	adc  r{reg_a:u3} r{reg_b:u3} r{reg_c:u3}		=> 0x0`4 @ reg_a @ reg_b @ 0b001 @ reg_c
	sub  r{reg_a:u3} r{reg_b:u3} r{reg_c:u3}		=> 0x0`4 @ reg_a @ reg_b @ 0b010 @ reg_c
	subc r{reg_a:u3} r{reg_b:u3} r{reg_c:u3}		=> 0x0`4 @ reg_a @ reg_b @ 0b011 @ reg_c
	nand r{reg_a:u3} r{reg_b:u3} r{reg_c:u3} 		=> 0x0`4 @ reg_a @ reg_b @ 0b100 @ reg_c
	and	 r{reg_a:u3} r{reg_b:u3} r{reg_c:u3} 		=> 0x0`4 @ reg_a @ reg_b @ 0b101 @ reg_c
	or	 r{reg_a:u3} r{reg_b:u3} r{reg_c:u3} 		=> 0x0`4 @ reg_a @ reg_b @ 0b110 @ reg_c
	xor	 r{reg_a:u3} r{reg_b:u3} r{reg_c:u3} 		=> 0x0`4 @ reg_a @ reg_b @ 0b111 @ reg_c
	addi r{reg_a:u3} r{reg_b:u3} i{imm:i6}  		=> 0x1`4 @ reg_a @ reg_b @ imm
	bs	 r{reg_a:u3} r{reg_b:u3} i{imm:u6}			=> 0x2`4 @ reg_a @ reg_b @ imm
	sw	 r{reg_a:u3} r{reg_b:u3} i{imm:s6} 			=> 0x3`4 @ reg_a @ reg_b @ imm
	lw	 r{reg_a:u3} r{reg_b:u3} i{imm:s6}			=> 0x4`4 @ reg_a @ reg_b @ imm
	cmp	 r{reg_a:u3} r{reg_b:u3}					=> 0x5`4 @ reg_a @ reg_b @ 0b000000
	jalr r{reg_a:u3} r{reg_b:u3}					=> 0x6`4 @ reg_a @ reg_b @ 0b000000
	beq	 r{reg_a:u3} r{reg_b:u3} i{imm:s6}			=> 0x7`4 @ reg_a @ reg_b @ imm
	bgr	 r{reg_a:u3} r{reg_b:u3} i{imm:s6}			=> 0x8`4 @ reg_a @ reg_b @ imm
	lui  r{reg_a:u3} i{imm: i10}					=> 0b111`3 @ reg_a @ imm
}

; useful macros
#ruledef {

	; negate a number
	not r{reg_a:u3} r{reg_b:u3} => asm { nand r reg_a r reg_b r reg_b }

	; shift with 4-bit immediate
	bsl r{reg_a:u3} r{reg_b:u3} i{imm:u4} => 0x2 @ reg_a @ reg_b @ 0b01 @imm
	bsr r{reg_a:u3} r{reg_b:u3} i{imm:u4} => 0x2 @ reg_a @ reg_b @ 0b00 @imm

	; shift from register
	bsl r{reg_a:u3} r{reg_b:u3} r{reg_c:u3} => 0x2 @ reg_a @ reg_b @ 0b110 @reg_c
	bsr r{reg_a:u3} r{reg_b:u3} r{reg_c:u3} => 0x2 @ reg_a @ reg_b @ 0b100 @reg_c

	; load immediate to register
	ldi r{reg_a:u3} i{imm:i16} => asm {
		lui r reg_a i ((imm & 0xffc0)>>6)`10
		addi r reg_a r reg_a i imm`6
	}

}