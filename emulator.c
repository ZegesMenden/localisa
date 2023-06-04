#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

// instructions:

// opcode subcode	name	parameters		description
// 0b0000  0b000	ADD 	ra rb 	rc 		-> ra = rb+rc
// 0b0000  0b001	ADC 	ra rb 	rc 		-> ra = rb+rc+c
// 0b0000  0b010	SUB 	ra rb 	rc 		-> ra = rb-rc
// 0b0000  0b011	SUBC 	ra rb 	rc 		-> ra = rb-rc-c
// 0b0000  0b100	NAND 	ra rb 	rc 		-> ra = !(rb&rc)
// 0b0000  0b101	AND 	ra rb 	rc 		-> ra = rb&rc
// 0b0000  0b110	OR 		ra rb 	rc 		-> ra = rb|rc
// 0b0000  0b111	XOR     ra rb   rc      -> ra = rb^rc
// 0b0001   N/A 	ADDI 	ra rb 	imm 	-> ra = rb+imm
// 0b0010	N/A 	BS 		ra rb 	imm 	-> imm>=32 ? ra = rb<<imm : ra = rb>>imm 
// 0b0011	N/A 	SW 		ra rb 	imm 	-> mem[rb+imm] = ra
// 0b0100	N/A 	LW 		ra rb 	imm 	-> ra = mem[rb+imm]
// 0b0101	N/A 	CMP 	ra rb 			-> compare ra and rb and update status register
// 0b0110	N/A 	JALR 	ra rb 			-> set PC to rb and store PC+1 in ra
// 0b0111	N/A 	BEQ 	ra rb   imm 	-> set PC to rb + imm and store PC+1 in ra if EQ flag is set
// 0b1000	N/A 	BGR		ra rb   imm 	-> set PC to rb + imm and store PC+1 in ra if GR flag is set
// 0b1110	N/A 	LUI 	ra imm 			-> ra = imm<<6 this instruction is special because it only uses 3 bits in order to fit a 10 bit immediate. the format is 0b111|reg|imm|

FILE *fptr;

#define N_REGISTERS 8
#define MEMORY_SIZE 64

int flag_beq = 0;
int flag_bgr = 0;
int carry = 0;
uint16_t registers[N_REGISTERS] = {0};
uint16_t memory[MEMORY_SIZE] = {0};
uint16_t PC = 0;
uint16_t instruction = 0;

uint64_t systick = 0;

// ===========================================================================
// typedefs

typedef enum opcode {
	ALU  = 0b0000,
	ADDI = 0b0001,
	BS 	 = 0b0010,
	SW 	 = 0b0011,
	LW 	 = 0b0100,
	CMP  = 0b0101,
	JALR = 0b0110,
	BEQ  = 0b0111,
	BGR	 = 0b1000,
	MOV  = 0b1001,
	LUI  = 0b1110
} opcode;

typedef enum alu_subcode {
	ADD	 = 0b000,
	ADC	 = 0b001,
	SUB	 = 0b010,
	SUBC = 0b011,
	NAND = 0b100,
	AND	 = 0b101,
	OR	 = 0b110,
	XOR	 = 0b111
} alu_subcode;

// ===========================================================================
// debug and error handlers

void __fatal_error_handler(const char * fun_name, char * msg) { 

	// print error, calling function, and error message
	printf("[FATAL ERROR] from function %s\n", fun_name);
	printf("%s\n", msg);
	printf("current PC: 0x%04x\n", PC, PC);
	
	// register dump
	printf("\nregisters:\n");
	for ( int i = 0; i < 8; i++ ) {	printf("r%i: 0x%04x\n", i, registers[i]); }

	// mem dump
	printf("\nmemory:\n");
	for ( int i = 0; i < 8; i++ ) {
		for ( int j = 0; j < 8; j++ ) { printf("%04x, ", memory[i*8+j]); } 
		printf("\n");
	}

	exit(-1);

}

void __error_handler(const char * fun_name, char * msg) {

	// print error, calling function, and error message
	printf("[ERROR] from function %s\n", fun_name);
	printf("%s\n", msg);
	printf("current PC: 0x%04x\n", PC, PC);
	
	// register dump
	printf("\nregisters:\n");
	for ( int i = 0; i < 8; i++ ) {	printf("r%i: 0x%04x\n", i, registers[i]); }

	// mem dump
	printf("\nmemory:\n");
	for ( int i = 0; i < 8; i++ ) {
		for ( int j = 0; j < 8; j++ ) { printf("%04x, ", memory[i*8+j]); } 
		printf("\n");
	}

}

#define fatal_error(msg) __fatal_error_handler(__FUNCTION__, msg)
#define error(msg) __error_handler(__FUNCTION__, msg)
#define debugprintf(c, ...) //printf(c, __VA_ARGS__)

void debug_function_diagnostics(const char * fun_name) {
	debugprintf("current operation: %s\n", fun_name);
	debugprintf("current PC: 0x%04x\n", PC, PC);
	
	// register dump
	debugprintf("\nregisters:\n", NULL);
	for ( int i = 0; i < 3; i++ ) {	debugprintf("r%i: 0x%04x\n", i, registers[i]); }
	debugprintf("\n", NULL);
	debugprintf("===========================================================================\n", NULL);
}

// ===========================================================================
// file interface

uint16_t fetch_next_instruction(FILE *fptr) {

	uint16_t ret;

	if ( fseek(fptr, PC*2, SEEK_SET) != 0 ) { fatal_error("invalid PC"); }
	if ( fread(&ret, sizeof(ret), 1, fptr) != 1 ) { fatal_error("invalid PC"); }

	ret = (ret&0xff)<<8 | (ret>>8);

	return ret;

}

// ===========================================================================
// CPU instructions

void update_alu(alu_subcode operation, int ra, int rb, int rc) {

	switch (operation) {
		case(ADD): {
			debug_function_diagnostics(__FUNCTION__);
			registers[ra] = registers[rb]+registers[rc];
			break;
		}
		case(ADC): {
			debug_function_diagnostics(__FUNCTION__);
			registers[ra] = registers[rb]+registers[rc]+carry;
			break;
		}
		case(SUB): {
			debug_function_diagnostics(__FUNCTION__);
			registers[ra] = registers[rb]-registers[rc];
			break;
		}
		case(SUBC): {
			debug_function_diagnostics(__FUNCTION__);
			registers[ra] = registers[rb]-registers[rc]-carry;
			break;
		}
		case(NAND): {
			debug_function_diagnostics(__FUNCTION__);
			registers[ra] = !(registers[rb]&registers[rc]);
			break;
		}
		case(AND): {
			debug_function_diagnostics(__FUNCTION__);
			registers[ra] = registers[rb]&registers[rc];
			break;
		}
		case(OR): {
			debug_function_diagnostics(__FUNCTION__);
			registers[ra] = registers[rb]|registers[rc];
			break;
		}
		case(XOR): {
			debug_function_diagnostics(__FUNCTION__);
			registers[ra] = registers[rb]^registers[rc];
			break;
		}
		default: {
			break;
		}
	}

}

void add_immediate(int ra, int rb, int imm) {
	debug_function_diagnostics(__FUNCTION__);
	registers[ra] = registers[rb]+imm;
}
void bit_shift(int ra, int rb, int imm) {
	debug_function_diagnostics(__FUNCTION__);

	// if bit 4 is set (shift left)
	if ( (imm>>4)&1 == 1 ) {

		// if bit 5 is set (shift from register)
		if ( (imm>>5)&1 == 1 ) { registers[ra] = registers[rb]<<registers[(imm&7)]; }
		else { registers[ra] = registers[rb]<<(imm&0xf); }

	} else {

		// if bit 5 is set (shift from register)
		if ( (imm>>5)&1 == 1 ) { registers[ra] = registers[rb]>>registers[(imm&7)]; }
		else { registers[ra] = registers[rb]>>(imm&0xf); }

	}
}
void load_word(int ra, int rb, int imm) {
	debug_function_diagnostics(__FUNCTION__);
	uint16_t memory_position = registers[rb]+imm;
	if ( memory_position >= MEMORY_SIZE ) { fatal_error("invalid memory access"); }
	registers[ra] = memory[memory_position];
};
void store_word(int ra, int rb, int imm) {
	debug_function_diagnostics(__FUNCTION__);
	uint16_t memory_position = registers[rb]+imm;
	if ( memory_position >= MEMORY_SIZE ) { fatal_error("invalid memory access"); }
	memory[memory_position] = registers[ra];
};
void compare(int ra, int rb) {
	debug_function_diagnostics(__FUNCTION__);
	flag_beq = registers[ra] == registers[rb];
	flag_bgr = registers[ra] > registers[rb];
}
void jump_and_link_register(int ra, int rb) {
	debug_function_diagnostics(__FUNCTION__);
	registers[ra] = PC;
	PC = registers[rb];
}
void branch_if_equal(int ra, int rb, int imm) {
	debug_function_diagnostics(__FUNCTION__);
	if ( flag_beq ) {
		registers[ra] = PC;
		PC = registers[rb]+imm;
	}
}
void branch_if_greater(int ra, int rb, int imm) {
	debug_function_diagnostics(__FUNCTION__);
	if ( flag_bgr ) {
		registers[ra] = PC;
		PC = registers[rb]+imm;
	}
}
void move(int ra, int rb) {
	debug_function_diagnostics(__FUNCTION__);
	registers[ra] = registers[rb];
}
void load_upper_immediate(int ra, int imm) {
	debug_function_diagnostics(__FUNCTION__);
	registers[ra] = imm<<6;
}
// ===========================================================================

// update function

int cpu_tick(void) {
	instruction = fetch_next_instruction(fptr);
	PC++;
		
	opcode code = (instruction&0xf000)>>12;

	debugprintf("instruction: %x\n", instruction);
	debugprintf("opcode: %i\n", code);

	// special processing for LUI due to a shorter instruction
	if ( code == 0b1110 || code == 0b1111 ) {
		
		int ra = (instruction&0x1c00)>>10;
		int imm = (instruction&0x03ff);
		load_upper_immediate(ra, imm);

		return 1;
		
	} 

	// extract registers and immediate
	int ra = (instruction&0x0e00)>>9;
	int rb = (instruction&0x01c0)>>6;
	int rc = instruction&0x7;
	int imm = (instruction&0x003f);

	switch(code) {
		case(ALU): {
			alu_subcode operation = (instruction&0x0038)>>3;
			update_alu(operation, ra, rb, rc);
			break;
		}  
		case(ADDI): {
			add_immediate(ra, rb, imm);
			break;
		} 
		case(BS): {
			bit_shift(ra, rb, imm);
			break;
		} 	 
		case(SW): {
			store_word(ra, rb, imm);
			break;
		} 	 
		case(LW): {
			load_word(ra, rb, imm);
			break;
		} 	 
		case(CMP): {
			compare(ra, rb);
			break;
		}  
		case(JALR): {
			jump_and_link_register(ra, rb);
			break;
		} 
		case(BEQ): {
			branch_if_equal(ra, rb, imm);
			break;
		}  
		case(BGR): {
			branch_if_greater(ra, rb, imm);
			break;
		}	
		case(MOV): {
			move(ra, rb);
			break;
		}	 
		default: {
			if ( code != LUI ) { error("unrecognized cpu instruction"); }
		} 
	}

	// return 1 to continue execution
	return 1;
}

int main(int argc, char *argv[]) {

	if ( argc < 2 ) { 
		printf("must enter file name\n"); 
		return 0; 
	}

	fptr = fopen(argv[1], "rb");

	if ( fptr == NULL ) { 
		printf("could not open file\n"); 
		return 0; 
	}

	printf("starting emulation..\n");

	while(cpu_tick()) { 
		systick++;
		// if ( systick > 10 ) { break; }
	}

	printf("ending emulation\n");
	fclose(fptr);
	return 0;
}