#include <fstream>
#include <map>
#include <ios>
#include <iostream>
#include <string>
#include <iomanip>
#include <climits>
using namespace std;

#define RAM_SIZE 1024
#define NUM_REGISTERS 32


// Authors: Andrew Weathers and Nicholas Muenchen
// Date: 21 September 2018
// Purpose: Simulate a simplified MIPS-like
//			instruction set

//opcode: 6 bits
//rs: 5 bits
//rt: 5 bits
//rd: 5 bits
//shift: 5 bits
//funct: 6 bits
unsigned int mar,
			 mdr,
 			 ir,
 			 rd,
 			 rs,
 			 rt,
 			 shift,
			 funct,
			 pc = 0,
			 numAlu = 0,
			 numInstFetch = 0,
			 numLoads = 0,
			 numStores = 0,
			 halt = 0,
			 numJumps = 0,
			 numJumpsAndLinks = 0,
			 numTakenBranches = 0,
			 numUnTakenBranches = 0,
			 registerArray[NUM_REGISTERS],
			 ram[RAM_SIZE];

int 	 sign_ext,
			 ram_end = 0;

bool   zeroAttempt=false;

map<unsigned int, string> opcodeMap;

void initiliazeRam()
{
	for(int i = 0; i < RAM_SIZE; i++)
	{
		ram[i] = INT_MAX;
	}
}


void fillMap()
{
 	opcodeMap[0x00] = "r";
    opcodeMap[0x09] = "i";
	opcodeMap[0x04] = "i";
	opcodeMap[0x07] = "i";
	opcodeMap[0x06] = "i";
	opcodeMap[0x05] = "i";
	opcodeMap[0x02] = "i";
	opcodeMap[0x03] = "i";
	opcodeMap[0x0f] = "i";
	opcodeMap[0x23] = "i";
	opcodeMap[0x1c] = "r";
	opcodeMap[0x0a] = "i";
	opcodeMap[0x2b] = "i";
	opcodeMap[0x0e] = "i";
}

void checkRegZero(unsigned int reg){
	if(reg == 0)
		zeroAttempt=true;
}

//Adds the number in rs to the number in rt, then stores in rd
void addu()
{
	checkRegZero(rd);
  registerArray[rd] = registerArray[rs] + registerArray[rt];
  numAlu++;
  cout << setw(3) << setfill('0') << hex << (pc - 1) << ": addu  - register r[";
	cout << rd << "] now contains " << "0x" << hex << setw(8) << setfill('0');
	cout << registerArray[rd] << "\r\n";
}

//Adds the number in rs to the immediately given value, then stores in rt
void addiu()
{
	checkRegZero(rt);
  registerArray[rt] = registerArray[rs] + sign_ext;
  numAlu++;
   cout << setw(3) << setfill('0') << hex << (pc - 1) << ": addiu - register r[";
	 cout << rt << "] now contains " << "0x" << hex << setw(8) << setfill('0');
	 cout << registerArray[rt] << "\r\n";
}

//Performs bitwise AND operation rs*rt, then stores in rd
void _and()
{
	checkRegZero(rd);
	registerArray[rd] = registerArray[rs] & registerArray[rt];
	numAlu++;
	cout << setw(3) << setfill('0') << hex << (pc - 1) << ": and   - register r[";
	cout << rd << "] now contains " << "0x" << hex << setw(8) << setfill('0') << registerArray[rd] << "\r\n";

//Logically shifts register rt right by shift and stores the result in rd, fills with ones or zeroes depending on s
}


//Branch is rs is equal to rt. Branches to immediate value.
void beq()
{
	int print_pc = pc;
	if (registerArray[rs] == registerArray[rt])
	{
		pc += sign_ext;
		pc = pc & 0xffff;
		numTakenBranches++;
		cout << setw(3) << setfill('0') << hex << (print_pc - 1) << ": beq   - branch taken to " << "0x" << hex << setw(8) << setfill('0') << pc << "\r\n";
	}
	else
	{
		numUnTakenBranches++;
		cout << setw(3) << setfill('0') << hex << (pc - 1) << ": beq   - branch untaken" << "\r\n";
	}


}

//Branch if r[rs] > 0 to the pc + signed immediate.
void bgtz()
{
	int print_pc = pc;
	if (int(registerArray[rs]) > 0)
	{
		pc += sign_ext;
		pc = pc & 0xffff;
		numTakenBranches++;
		cout << setw(3) << setfill('0') << hex << (print_pc - 1) << ": bgtz  - branch taken to " << "0x" << hex << setw(8) << setfill('0') << pc << "\r\n";
	}
	else
	{
		numUnTakenBranches++;
		cout << setw(3) << setfill('0') << hex << (pc - 1) << ": bgtz  - branch untaken" << "\r\n";
	}
}

//Branch if r[rs] <= 0 to the pc + signed immediate.
void blez()
{
	int print_pc = pc;
	if (int(registerArray[rs]) <= 0)
	{
		pc += sign_ext;
		pc = pc & 0xffff;
		numTakenBranches++;
		cout << setw(3) << setfill('0') << hex << (print_pc - 1) << ": blez  - branch taken to " << "0x" << hex << setw(8) << setfill('0') << pc << "\r\n";

	}
	else
	{
		numUnTakenBranches++;
		cout << setw(3) << setfill('0') << hex << (pc - 1) << ": blez  - branch untaken" << "\r\n";
	}
}

//Branch if registerArray[rs] is not equal to registerArray[rt], branch to pc + signed immediate.
void bne()
{
	int print_pc = pc;
	if (registerArray[rs] != registerArray[rt])
	{
		pc += sign_ext;
		numTakenBranches++;
		pc = pc & 0xffff;
		cout << setw(3) << setfill('0') << hex << (print_pc - 1) << ": bne   - branch taken to " << "0x" << hex << setw(8) << setfill('0') << pc << "\r\n";

	}
	else
	{
		numUnTakenBranches++;
		cout << setw(3) << setfill('0') << hex << (pc - 1) << ": bne  - branch untaken" << "\r\n";
	}
}

//Halts execution
void hlt()
{
	halt = 1;
	numInstFetch--;
	cout << setw(3) << setfill('0') << hex << (pc - 1) << ": hlt" << "\r\n";
  return;
}

//Jump to target memory location and store index in pc.
void j()
{
	int print_pc = pc;
	pc = sign_ext;
	numJumps++;
	cout << setw(3) << setfill('0') << hex << (print_pc - 1) << ": j     - jump to " << "0x" << hex << setw(8) << setfill('0') << pc << "\r\n";
}

//Jump and link jumps, but also stores pc
// in given register.
void jal()
{
	registerArray[31] = pc;
	pc = sign_ext;
	numJumpsAndLinks++;
	cout << setw(3) << setfill('0') << hex << registerArray[31] - 1 << ": jal   - jump to " << "0x" << hex << setw(8) << setfill('0') << pc << "\r\n";
}

//Store incremented pc in rd and jump to rs.
//Value in rs is now stored in the pc.
void jalr()
{
	registerArray[rd] = pc;
	pc = registerArray[rs];
	numJumpsAndLinks++;
	cout << setw(3) << setfill('0') << hex << registerArray[rd] - 1 << ": jalr  - jump to " << "0x" << hex << setw(8) << setfill('0') << pc << "\r\n";


}

//A given register is jumped to and
// is loaded in the pc
void jr()
{
	int print_pc = pc - 1;
	pc = registerArray[rs];
	numJumps++;
	cout << setw(3) << setfill('0') << hex << print_pc << ": jr    - jump to " << "0x" << hex << setw(8) << setfill('0') << pc << "\r\n";

}

//Shifts immediate value to the upper 16 bits with trailing 0's.
//The result is stored in register rt
void lui()
{
	checkRegZero(rt);
	registerArray[rt] = sign_ext << 16;
	numAlu++;
	cout << setw(3) << setfill('0') << hex << (pc - 1) << ": lui   - register r[" << rt << "] now contains " << "0x" << hex << setw(8) << setfill('0') << registerArray[rt] << "\r\n";

}

//Load value in rt from memory + any sign_ext which may apply
void lw()
{
	checkRegZero(rt);
	registerArray[rt] = ram[rs+sign_ext];
	numLoads++;
	cout << setw(3) << setfill('0') << hex << (pc - 1) << ": lw    - register r[" << rt << "] now contains " << "0x" << hex << setw(8) << setfill('0') << registerArray[rt] << "\r\n";

}

//multiplies values in rs and rt and places the result into rd
void mul()
{
	checkRegZero(rd);
	registerArray[rd] = registerArray[rs]*registerArray[rt];
	numAlu++;
	cout << setw(3) << setfill('0') << hex << (pc - 1) << ": mul   - register r[" << rd << "] now contains " << "0x" << hex << setw(8) << setfill('0') << registerArray[rd] << "\r\n";

}

//nor's register rs and rt and places the result into rd
void nor()
{
	checkRegZero(rd);
	registerArray[rd] = ~(registerArray[rs] | registerArray[rt]);
	numAlu++;
	cout << setw(3) << setfill('0') << hex << (pc - 1) << ": nor   - register r[" << rd << "] now contains " << "0x" << hex << setw(8) << setfill('0') << registerArray[rd] << "\r\n";

}

//or's register rs and register rt and places the result into rd
void _or()
{
	checkRegZero(rd);
	registerArray[rd] = registerArray[rs] | registerArray[rt];
	numAlu++;
	cout << setw(3) << setfill('0') << hex << (pc - 1) << ": or    - register r[" << rd << "] now contains " << "0x" << hex << setw(8) << setfill('0') << registerArray[rd] << "\r\n";

}

//Shifts register rt left logically by shift and stores the result in rd
/////////////////////////////////

//Logically shifts register rt right by shift and stores the result in rd, fills with ones or zeroes depending on s
//UNSURE ABOUT THE REGISTERS TO BE USED, ALSO

//Logically shifts register rt right by shift and stores the result in rd, fills with ones or zeroes depending on sUNSURE ABOUT IMPLEMENTATION
/////////////////////////////////
void sll()
{
	checkRegZero(rd);
	registerArray[rd] = registerArray[rt] << shift;
	numAlu++;
	cout << setw(3) << setfill('0') << hex << (pc - 1) << ": sll   - register r[" << rd << "] now contains " << "0x" << hex << setw(8) << setfill('0') << registerArray[rd] << "\r\n";

}

//If register rs < sign_ext, then set register rt to 1 else set to 0
void slti()
{
	checkRegZero(rd);
	if (registerArray[rs] < sign_ext)
	{
		registerArray[rd] = 1;
	}
	else
	{
		registerArray[rd] = 0;
	}
	numAlu++;
	cout << setw(3) << setfill('0') << hex << (pc - 1) << ": slti  - register r[" << rd << "] now contains " << "0x" << hex << setw(8) << setfill('0') << registerArray[rd] << "\r\n";

}


//Logically shifts register rt right by shift and stores the result in rd, fills with ones or zeroes depending on sign
/////////////////////////////////
//UNSURE ABOUT THE REGISTERS TO BE USED, ALSO UNSURE ABOUT IMPLEMENTATION
/////////////////////////////////
void sra()
{
	checkRegZero(rd);
	registerArray[rd] = registerArray[rt] >> shift;
	if(registerArray[rt] >> 31 & 1)
	{
		registerArray[rd] = registerArray[rd] | (((1 << (shift + 1)) - 1) << (31 - shift));
	}
	numAlu++;
	cout << setw(3) << setfill('0') << hex << (pc - 1) << ": sra   - register r[" << rd << "] now contains " << "0x" << hex << setw(8) << setfill('0') << registerArray[rd] << "\r\n";

}


//Logically shifts register rt right by shift and stores the result in rd, fills with zeroes
/////////////////////////////////
//UNSURE ABOUT THE REGISTERS TO BE USED
/////////////////////////////////
void srl()
{
	checkRegZero(rd);
	registerArray[rd] = (unsigned int )(registerArray[rt]) >> shift;
	numAlu++;

	cout << setw(3) << setfill('0') << hex << (pc - 1) << ": srl   - register r[" << rd << "]";
	cout << " now contains " << "0x" << hex << setw(8) << setfill('0') << registerArray[rd] << "\r\n";
}

//Subtract register rt from register rs and save the result into rd
void subu()
{
	checkRegZero(rd);
	registerArray[rd] = registerArray[rs]  - registerArray[rt];
	numAlu++;

	cout << setw(3) << setfill('0') << hex << (pc - 1) << ": subu  - register r[" << rd << "]";
	cout << " now contains " << "0x" << hex << setw(8) << setfill('0') << registerArray[rd] << "\r\n";
}

//Stores the word in r[t] at registerArray[registerArray[rs] + sign_imm
void sw()
{
	ram[registerArray[rs] + sign_ext] = registerArray[rt];
	numStores++;

	cout << setw(3) << setfill('0') << hex << (pc - 1) << ": sw    - register r[" << rt << "]";
	cout << " value stored in memory" << "\r\n";
}

//Exclusive or's registerArray[rs] and registerArray[rt] then stores the result in registerArray[rd]
void _xor()
{
	checkRegZero(rd);
	registerArray[rd] = registerArray[rs] ^ registerArray[rt];
	numAlu++;

	cout << setw(3) << setfill('0') << hex << (pc - 1) << ": xor   - register r[" << rd << "]";
	cout << " now contains " << "0x" << hex << setw(8) << setfill('0') << registerArray[rd] << "\r\n";
}


//Exclusive or's registerArray[rs] with sign_ext and stores the result in registerArray[rt]
void xori()
{
	checkRegZero(rt);
	registerArray[rt] = registerArray[rs] ^ sign_ext;
	numAlu++;

	cout << setw(3) << setfill('0') << hex << (pc - 1) << ": xori  - register r[" << rt << "]";
	cout << " now contains " << "0x" << hex << setw(8) << setfill('0') << registerArray[rt] << "\r\n";
}




//Fetches the next instruction.
void fetch()
{
  mar = pc;
  mdr = ram[mar];
  ir = mdr;
  pc++;
  numInstFetch++;
}

/*
rs = (ir >> 21) & 0x1f; // clamps to the 5 bit rs
rt = (ir >> 16) & 0x1f; // clamps to the 5 bit rt
rd = (ir >> 11) & 0x1f; // clamps to the 5 bit rd
shift = (ir >> 6) & 0x1f; // clamps to the 5 bit shift
rs = ir & 0x2f; // clamps to the 6 bit funct
*/

void sign_extend()
{
	if((sign_ext >> 15) & 1)
	{
		sign_ext = 0xFFFF - sign_ext + 1;
		sign_ext *= -1;
	}
}

void (*imm_func())()
{
  unsigned int opcode = (ir >> 26) & 0x3f; // clamp to 6-bit opcode field
  rs = (ir >> 21) & 0x1f; // clamp to the 5 bit rs
  rt = (ir >> 16) & 0x001f; // clamp to the 5 bit rt
  sign_ext = (ir) & 0x0000ffff; // clamp to 16 bit immediate value


  if(opcode == 0x09)
  {
		sign_extend();
    return addiu;
  }
	if(opcode == 0x04)
	{
		sign_extend();
		return beq;
	}
	if(opcode == 0x07)
	{
		sign_extend();
		return bgtz;
	}
	if(opcode == 0x06)
	{
		sign_extend();
		return blez;
	}
	if(opcode == 0x05)
	{
		sign_extend();
		return bne;
	}
	if(opcode == 0x02)
	{
		return j;
	}
	if(opcode == 0x03)
	{
		return jal;
	}
	if(opcode == 0x0f)
	{
		return lui;
	}
	if(opcode == 0x23)
	{
		sign_extend();
		return lw;
	}
	if(opcode == 0x0a)
	{
		sign_extend();
		return slti;
	}
	if(opcode == 0x2b)
	{
		sign_extend();
		return sw;
	}
	if(opcode == 0x0e)
	{
		return xori;
	}
}

void (*other_func())()
{
  unsigned int opcode = (ir >> 26) & 0x3f; // clamp to 6-bit opcode field
  rs = (ir >> 21) & 0x1f; // clamps to the 5 bit rs
  rt = (ir >> 16) & 0x1f; // clamps to the 5 bit rt
  rd = (ir >> 11) & 0x1f; // clamps to the 5 bit rd
  shift = (ir >> 6) & 0x1f; // clamps to the 5 bit shift
  funct = ir & 0x2f; // clamps to the 6 bit funct

  if(opcode == 0x00)
  {
    if(funct == 0x21)
    {
      return addu;
    }
		if(funct == 0x24)
		{
			return _and;
		}
		if(funct == 0x09)
		{
			return jalr;
		}
		if(funct == 0x08)
		{
			return jr;
		}
		if(funct == 0x27)
		{
			return nor;
		}
		if(funct == 0x25)
		{
			return _or;
		}
		if(funct == 0x00)
		{
			return sll;
		}
		if(funct == 0x03)
		{
			return sra;
		}
		if(funct == 0x02)
		{
			return srl;
		}
		if(funct == 0x23)
		{
			return subu;
		}
		if(funct == 0x26)
		{
			return _xor;
		}
	}
	if(opcode == 0x1c)
	{
	return mul;
	}
}




//Right shift by 26 to isolate the opcode

void ( *decode() )()
{
  unsigned int opcode = (ir >> 26) & 0x3f; // clamp to 6-bit opcode field
	if(ir == 0)
	{
		return hlt;
	}
  if(opcodeMap.find(opcode) != opcodeMap.end())
  {
    //Finds if the function is immediate
    if(opcodeMap.find(opcode)->second.compare("r"))
    {
      return imm_func();
    }
    else
    {
      return other_func();
    }
  }
  else
  {
    return hlt;
  }
}

void printMemory()
{
	cout << "contents of memory" << "\r\n";
	cout << "addr value" << "\r\n";
	for(int i = 0; i < RAM_SIZE; i++)
	{
		if(ram[i] != INT_MAX)
		{
			cout << setw(3) << setfill('0') << i;
			cout << ": " << setw(8) << setfill('0') << hex << noshowbase << ram[i] << "\r\n";
		}
	}
}

void writeOutput()
{
	cout << "\r\n";
	printMemory();
	cout << dec;
	int numJumpsAndBranches = numTakenBranches + numUnTakenBranches + numJumps + numJumpsAndLinks;
	int numLoadsAndStores = numStores + numLoads;
	int totalInstClassCounts = numAlu + numLoadsAndStores + numJumpsAndBranches;
	int totalMemAccess = numLoadsAndStores + numInstFetch;
	cout << "\r\n";
	cout << "instruction class counts (omits hlt instruction)" << "\r\n";
	cout << "  alu ops            " << setfill(' ') << setw(2) << numAlu << "\r\n";
	cout << "  loads/stores       " << setfill(' ') << setw(2) << numLoadsAndStores << "\r\n";
	cout << "  jumps/branches     " << setfill(' ') << setw(2) << numJumpsAndBranches << "\r\n";
	cout << "total                " << setfill(' ') << setw(2) << totalInstClassCounts << "\r\n" << "\r\n";

	cout << "memory access counts (omits hlt instruction)" << "\r\n";
	cout << "  inst. fetches      " << setfill(' ') << setw(2) << numInstFetch << "\r\n";
	cout << "  loads              " << setfill(' ') << setw(2) << numLoads << "\r\n";
	cout << "  stores             " << setfill(' ') << setw(2) << numStores << "\r\n";
	cout << "total                " << setfill(' ') << setw(2) << totalMemAccess << "\r\n" << "\r\n";

	cout << "transfer of control counts" << "\r\n";
	cout << "  jumps              " << setfill(' ') << setw(2) << numJumps << "\r\n";
	cout << "  jump-and-links     " << setfill(' ') << setw(2) << numJumpsAndLinks << "\r\n";
	cout << "  taken branches     " << setfill(' ') << setw(2) << numTakenBranches << "\r\n";
	cout << "  untaken branches   " << setfill(' ') << setw(2) << numUnTakenBranches << "\r\n";
	cout << "total                " << setfill(' ') << setw(2) << numJumpsAndBranches << "\r\n";



}


//Store terminal input into ram
void gatherInput()
{
	unsigned int input;
	int i=0;
	while(cin >> hex >> input)
	{
		ram[ram_end] = input;
		ram_end++;
	}
	printMemory();
	cout << "\r\n";
	cout << "behavioral simulation of simple MIPS-like machine\r\n";
	cout << "  (all values are shown in hexadecimal)\r\n";
	cout << "\r\n";
	cout << "pc   result of instruction at that location\r\n";
}

int main()
{
	initiliazeRam();
	fillMap();
	gatherInput();
	void (* inst)();

  while(halt == 0)
	{
		fetch();
		inst = decode();
		(*inst)();
		if(zeroAttempt)
		{
			zeroAttempt = false;
			cout << "***** - register r[0] not allowed to change; reset to 0\r\n";
			registerArray[0] = 0;
		}
	}
	writeOutput();

  return 0;
}

//000000 00001 00010 00011 00000 100011
//000000 00001 00010 00011 00000 10011
