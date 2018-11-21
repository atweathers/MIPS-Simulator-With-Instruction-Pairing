
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
// Date: 20 November 2018
// Purpose: Simulate a paired MIPS like instruction set
//		    capable of performing double issues.

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

int 		 sign_ext,
			 ram_end = 0;

int			 numIssueCycles = 0,
			 numDoubleIssue = 0,
			 numControlStops = 0,
			 numStructuralStops = 0,
			 numDataDependencyStops = 0,
			 numDataAndStructuralStops = 0;


bool zeroAttempt = false;
bool doubleIssue = false;
bool controlStopFound = false;

string issueStatement;

map<unsigned int, string> opcodeMap;

void initiliazeRam(){
	for(int i = 0; i < RAM_SIZE; i++)
		ram[i] = INT_MAX;
}


void fillMap(){
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
void addu(){
  checkRegZero(rd);
  registerArray[rd] = registerArray[rs] + registerArray[rt];
  numAlu++;
  cout << setw(3) << setfill('0') << hex << (pc - 1) << ": addu  ";
}

//Adds the number in rs to the immediately given value, then stores in rt
void addiu(){
	checkRegZero(rt);
	registerArray[rt] = registerArray[rs] + sign_ext;
	numAlu++;
	cout << setw(3) << setfill('0') << hex << (pc - 1) << ": addiu ";
}

//Performs bitwise AND operation rs*rt, then stores in rd
void _and(){
	checkRegZero(rd);
	registerArray[rd] = registerArray[rs] & registerArray[rt];
	numAlu++;
	cout << setw(3) << setfill('0') << hex << (pc - 1) << ": and   - register r[";
	cout << rd << "] now contains " << "0x" << hex << setw(8) << setfill('0'); 
	cout << registerArray[rd] << "\r\n";
}


//Branch is rs is equal to rt. Branches to immediate value.
void beq(){
	int print_pc = pc;
	if (registerArray[rs] == registerArray[rt]){
		pc += sign_ext;
		pc = pc & 0xffff;
		numTakenBranches++;
		cout << setw(3) << setfill('0') << hex << (print_pc - 1) << ": beq   ";
	} else {
		numUnTakenBranches++;
		cout << setw(3) << setfill('0') << hex << (pc - 1) << ": beq   ";
	}
}

//Branch if r[rs] > 0 to the pc + signed immediate.
void bgtz(){
	int print_pc = pc;
	if (int(registerArray[rs]) > 0){
		pc += sign_ext;
		pc = pc & 0xffff;
		numTakenBranches++;
		cout << setw(3) << setfill('0') << hex << (print_pc - 1) << ": bgtz  ";
	} else {
		numUnTakenBranches++;
		cout << setw(3) << setfill('0') << hex << (pc - 1) << ": bgtz  ";
	}
}

//Branch if r[rs] <= 0 to the pc + signed immediate.
void blez(){
	int print_pc = pc;
	if (int(registerArray[rs]) <= 0){
		pc += sign_ext;
		pc = pc & 0xffff;
		numTakenBranches++;
		cout << setw(3) << setfill('0') << hex << (print_pc - 1) << ": blez  ";

	} else {
		numUnTakenBranches++;
		cout << setw(3) << setfill('0') << hex << (pc - 1) << ": blez  ";
	}
}

//Branch if registerArray[rs] is not equal to registerArray[rt], 
//branch to pc + signed immediate.
void bne(){
	int print_pc = pc;
	if (registerArray[rs] != registerArray[rt]){
		pc += sign_ext;
		numTakenBranches++;
		pc = pc & 0xffff;
		cout << setw(3) << setfill('0') << hex << (print_pc - 1) << ": bne   ";

	} else {
		numUnTakenBranches++;
		cout << setw(3) << setfill('0') << hex << (pc - 1) << ": bne   ";
	}
}

//Halts execution
void hlt(){
	halt = 1;
	numInstFetch--;
	cout << setw(3) << setfill('0') << hex << (pc - 1) << ": hlt   ";
}

//Jump to target memory location and store index in pc.
void j(){
	int print_pc = pc;
	pc = sign_ext;
	numJumps++;
	cout << setw(3) << setfill('0') << hex << (print_pc - 1) << ": j     ";
}

//Jump and link jumps, but also stores pc
// in given register.
void jal(){
	registerArray[31] = pc;
	pc = sign_ext;
	numJumpsAndLinks++;
	cout << setw(3) << setfill('0') << hex << registerArray[31] - 1 << ": jal   ";
}

//Store incremented pc in rd and jump to rs.
//Value in rs is now stored in the pc.
void jalr(){
	registerArray[rd] = pc;
	pc = registerArray[rs];
	numJumpsAndLinks++;
	cout << setw(3) << setfill('0') << hex << registerArray[rd] - 1 << ": jalr  ";
}

//A given register is jumped to and
// is loaded in the pc
void jr(){
	int print_pc = pc - 1;
	pc = registerArray[rs];
	numJumps++;
	cout << setw(3) << setfill('0') << hex << print_pc << ": jr    ";
}

//Shifts immediate value to the upper 16 bits with trailing 0's.
//The result is stored in register rt
void lui(){
	checkRegZero(rt);
	registerArray[rt] = sign_ext << 16;
	numAlu++;
	cout << setw(3) << setfill('0') << hex << (pc - 1) << ": lui   ";
}

//Load value in rt from memory + any sign_ext which may apply
void lw(){
	checkRegZero(rt);
	registerArray[rt] = ram[rs+sign_ext];
	numLoads++;
	cout << setw(3) << setfill('0') << hex << (pc - 1) << ": lw    ";
}

//multiplies values in rs and rt and places the result into rd
void mul(){
	checkRegZero(rd);
	registerArray[rd] = registerArray[rs]*registerArray[rt];
	numAlu++;
	cout << setw(3) << setfill('0') << hex << (pc - 1) << ": mul   ";

}

//nor's register rs and rt and places the result into rd
void nor(){
	checkRegZero(rd);
	registerArray[rd] = ~(registerArray[rs] | registerArray[rt]);
	numAlu++;
	cout << setw(3) << setfill('0') << hex << (pc - 1) << ": nor   ";
}

//or's register rs and register rt and places the result into rd
void _or(){
	checkRegZero(rd);
	registerArray[rd] = registerArray[rs] | registerArray[rt];
	numAlu++;
	cout << setw(3) << setfill('0') << hex << (pc - 1) << ": or    ";
}

//Shifts register rt left logically by shift and stores the result in rd
/////////////////////////////////

//Logically shifts register rt right by shift and stores the result in rd, 
//fills with ones or zeroes depending on s
//UNSURE ABOUT THE REGISTERS TO BE USED, ALSO

//Logically shifts register rt right by shift and stores the result in rd, 
//fills with ones or zeroes depending on sUNSURE ABOUT IMPLEMENTATION
/////////////////////////////////
void sll(){
	checkRegZero(rd);
	registerArray[rd] = registerArray[rt] << shift;
	numAlu++;
	cout << setw(3) << setfill('0') << hex << (pc - 1) << ": sll   ";
}

//If register rs < sign_ext, then set register rt to 1 else set to 0
void slti(){
	checkRegZero(rd);

	if (registerArray[rs] < sign_ext) 
		registerArray[rd] = 1;
	else 
		registerArray[rd] = 0;

	numAlu++;
	cout << setw(3) << setfill('0') << hex << (pc - 1) << ": slti  ";
}


//Logically shifts register rt right by shift and stores the result in rd, 
//fills with ones or zeroes depending on sign
/////////////////////////////////
//UNSURE ABOUT THE REGISTERS TO BE USED, ALSO UNSURE ABOUT IMPLEMENTATION
/////////////////////////////////
void sra(){
	checkRegZero(rd);
	registerArray[rd] = registerArray[rt] >> shift;

	if(registerArray[rt] >> 31 & 1)
		registerArray[rd] = registerArray[rd] | (((1 << (shift + 1)) - 1) << (31 - shift));

	numAlu++;
	cout << setw(3) << setfill('0') << hex << (pc - 1) << ": sra   ";
}


//Logically shifts register rt right by shift and stores the result in rd, fills with zeroes
/////////////////////////////////
//UNSURE ABOUT THE REGISTERS TO BE USED
/////////////////////////////////
void srl(){
	checkRegZero(rd);
	registerArray[rd] = (unsigned int )(registerArray[rt]) >> shift;
	numAlu++;

	cout << setw(3) << setfill('0') << hex << (pc - 1) << ": srl   ";
}

//Subtract register rt from register rs and save the result into rd
void subu(){
	checkRegZero(rd);
	registerArray[rd] = registerArray[rs]  - registerArray[rt];
	numAlu++;

	cout << setw(3) << setfill('0') << hex << (pc - 1) << ": subu  ";
}

//Stores the word in r[t] at registerArray[registerArray[rs] + sign_imm
void sw(){
	ram[registerArray[rs] + sign_ext] = registerArray[rt];
	numStores++;

	cout << setw(3) << setfill('0') << hex << (pc - 1) << ": sw    ";
}

//Exclusive or's registerArray[rs] and registerArray[rt] then 
//stores the result in registerArray[rd]
void _xor(){
	checkRegZero(rd);
	registerArray[rd] = registerArray[rs] ^ registerArray[rt];
	numAlu++;

	cout << setw(3) << setfill('0') << hex << (pc - 1) << ": xor   ";
}


//Exclusive or's registerArray[rs] with sign_ext and stores the result in registerArray[rt]
void xori(){
	checkRegZero(rt);
	registerArray[rt] = registerArray[rs] ^ sign_ext;
	numAlu++;

	cout << setw(3) << setfill('0') << hex << (pc - 1) << ": xori  ";
}




//Fetches the next instruction for first issue slot.
void fetch(){
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

void sign_extend(){
	if((sign_ext >> 15) & 1){
		sign_ext = 0xFFFF - sign_ext + 1;
		sign_ext *= -1;
	}
}

void (*imm_func())(){
	unsigned int opcode = (ir >> 26) & 0x3f; // clamp to 6-bit opcode field
	rs = (ir >> 21) & 0x1f; // clamp to the 5 bit rs
	rt = (ir >> 16) & 0x001f; // clamp to the 5 bit rt
	sign_ext = (ir) & 0x0000ffff; // clamp to 16 bit immediate value

	switch(opcode){
		case 0x09:
			sign_extend();
			return addiu;
		case 0x04:
			sign_extend(); 
			return beq;
		case 0x07:
			sign_extend();
			return bgtz;
		case 0x06:
			sign_extend();
			return blez;
		case 0x05:
			sign_extend();
			return bne;
		case 0x23: 
			sign_extend();
			return lw;
		case 0x0a: 
			sign_extend();
			return slti;
		case 0x2b: 
			sign_extend();
			return sw;
		case 0x02: 
			return j;
		case 0x03:
			return jal;
		case 0x0f: 
			return lui;	
		case 0x0e: 
			return xori;
	}
}

void (*other_func())(){
  unsigned int opcode = (ir >> 26) & 0x3f; // clamp to 6-bit opcode field
  rs = (ir >> 21) & 0x1f; // clamps to the 5 bit rs
  rt = (ir >> 16) & 0x1f; // clamps to the 5 bit rt
  rd = (ir >> 11) & 0x1f; // clamps to the 5 bit rd
  shift = (ir >> 6) & 0x1f; // clamps to the 5 bit shift
  funct = ir & 0x2f; // clamps to the 6 bit funct

  if(opcode==0x00)
  	switch(funct){
  		case 0x21: return addu;
  		case 0x24: return _and;
  		case 0x09: return jalr;
  		case 0x08: return jr;
  		case 0x27: return nor;
  		case 0x25: return _or;
  		case 0x00: return sll;
  		case 0x03: return sra;
  		case 0x02: return srl;
  		case 0x23: return subu;
  		case 0x26: return _xor;
  	}
  else if(opcode == 0x1c)
  	return mul;
}

//Right shift by 26 to isolate the opcode

void ( *decode() )(){
	unsigned int opcode = (ir >> 26) & 0x3f; // clamp to 6-bit opcode field

	if(ir == 0) return hlt;

	if(opcodeMap.find(opcode) != opcodeMap.end())
    	//Finds if the function is immediate
    	if(opcodeMap.find(opcode)->second.compare("r"))
      		return imm_func();
    	else
      		return other_func();
    else
    	return hlt;
}

void printMemory(){
	cout << "contents of memory" << "\r\n";
	cout << "addr value" << "\r\n";

	for(int i = 0; i < RAM_SIZE; i++){
		if(ram[i] != INT_MAX){
			cout << setw(3) << setfill('0') << i;
			cout << ": ";
			cout << setw(8) << setfill('0') << hex << noshowbase << ram[i] << "\r\n";
		}
	}
}

void printInstCounts(int numLoadsAndStores, int numJumpsAndBranches, int totalMemAccess){
	int totalInstClassCounts = numAlu + numLoadsAndStores + numJumpsAndBranches;
	cout << "\r\n";

	cout << "instruction class counts (omits hlt instruction)" << "\r\n";

	cout << "  alu ops         ";
	cout << setfill(' ') << setw(5) << numAlu;
	cout << "\r\n";

	cout << "  loads/stores    ";
	cout << setfill(' ') << setw(5) << numLoadsAndStores;
	cout << "\r\n";

	cout << "  jumps/branches  ";
	cout << setfill(' ') << setw(5) << numJumpsAndBranches;
	cout << "\r\n";

	cout << "total             ";
	cout << setfill(' ') << setw(5) << totalInstClassCounts;
	cout << "\r\n";
	cout << "\r\n";
}

void printMemoryCounts(int totalMemAccess){
	cout << "memory access counts (omits hlt instruction)" << "\r\n";

	cout << "  inst. fetches   ";
	cout << setfill(' ') << setw(5) << numInstFetch;
	cout << "\r\n";

	cout << "  loads           ";
	cout << setfill(' ') << setw(5) << numLoads;
	cout << "\r\n";

	cout << "  stores          ";
	cout << setfill(' ') << setw(5) << numStores;
	cout << "\r\n";

	cout << "total             ";
	cout << setfill(' ') << setw(5) << totalMemAccess;
	cout << "\r\n";
	cout << "\r\n";
}

void printControlCounts(int numJumpsAndBranches){
	cout << "transfer of control counts" << "\r\n";
	cout << "  jumps           ";
	cout << setfill(' ') << setw(5) << numJumps;
	cout << "\r\n";

	cout << "  jump-and-links  ";
	cout << setfill(' ') << setw(5) << numJumpsAndLinks;
	cout << "\r\n";

	cout << "  taken branches  ";
	cout << setfill(' ') << setw(5) << numTakenBranches;
	cout << "\r\n";

	cout << "  untaken branches";
	cout << setfill(' ') << setw(5) << numUnTakenBranches;
	cout << "\r\n";

	cout << "total             ";
	cout << setfill(' ') << setw(5) << numJumpsAndBranches;
	cout << "\r\n";
	cout << "\r\n";
}

void printPairingCounts(){
	cout << "instruction pairing counts (includes hlt instruction)\r\n";

	cout << "  issue cycles    ";
	cout << setw(5) << setfill(' ') << numIssueCycles;
	cout << "\r\n";

	double percentDoubleIssue = static_cast<double>(numDoubleIssue);
	percentDoubleIssue /= static_cast<double>(numIssueCycles);
	percentDoubleIssue *= 100;

	cout << "  double issues   ";
	cout << setw(5) << setfill(' ') << numDoubleIssue;
	cout << " ( " << setprecision(3) << showpoint << percentDoubleIssue;
	cout << " percent of issue cycles)";
	cout << "\r\n";

	cout << "  control stops   ";
	cout << setw(5) << setfill(' ') << numControlStops;
	cout << "\r\n";

	cout << "  structural stops";
	cout << setw(5) << setfill(' ') << numStructuralStops;
	cout << " (" << numDataAndStructuralStops;
	cout << " of which would also stop on a data dep.)";
	cout << "\r\n";

	cout << "  data dep. stops ";
	cout << setw(5) << setfill(' ') << numDataDependencyStops;
	cout << "\r\n";
}

void writeOutput(){
	cout << dec;
	int numJumpsAndBranches = numTakenBranches + numUnTakenBranches;
	numJumpsAndBranches += numJumps + numJumpsAndLinks;
	int numLoadsAndStores = numStores + numLoads;
	int totalMemAccess = numLoadsAndStores + numInstFetch;

	printInstCounts(numLoadsAndStores, numJumpsAndBranches, totalMemAccess);
	printMemoryCounts(totalMemAccess);
	printControlCounts(numJumpsAndBranches);
	printPairingCounts();
}


//Store terminal input into ram
void gatherInput(){
	unsigned int input;
	int i=0;
	while(cin >> hex >> input){
		ram[ram_end] = input;
		ram_end++;
	}
	printMemory();
	cout << "\r\n";
	cout << "simple MIPS-like machine with instruction pairing\r\n";
	cout << "  (all values are shown in hexadecimal)\r\n";
	cout << "\r\n";
	cout << "instruction pairing analysis\r\n";
}


//Checks for RAW data dependency
bool checkRAW(int ir2, int storeRegister){
	int rs2, rt2;

	rs2 = (ir2 >> 21) & 0x1f; // clamp to the 5 bit rs
	rt2 = (ir2 >> 16) & 0x001f; // clamp to the 5 bit rt

	//Checks for a data dependency
	if(rs2 == storeRegister || rt2 == storeRegister)

		if(doubleIssue){
			numDataDependencyStops++;
			doubleIssue = false;
			issueStatement = "// data dependency stop";
			return true;
		} else {
			numDataAndStructuralStops++;
			issueStatement += " (also data dep.)";
			return true;
		}

	else 
		return false;
}

//Checks for RAW data dependency
void checkWAW(int ir2, int storeRegister, int opcode2){
	int checkReg;
	//Tries to find opcode1 in the map
	if(opcodeMap.find(opcode2) != opcodeMap.end())
		//Finds if the function is I-type, if it is, sets storeRegister to rt
		if(opcodeMap.find(opcode2)->second.compare("r"))
			checkReg = (ir2 >> 11) & 0x1f; // clamps to the 5 bit rd

		else //Otherwise, storeRegister is set to rd
			checkReg = (ir2 >> 16) & 0x001f; // clamp to the 5 bit rt

	if(checkReg == storeRegister)
		if(doubleIssue){
			numDataDependencyStops++;
			doubleIssue = false;
			issueStatement = "// data dependency stop";
		} else {
			numDataAndStructuralStops++;
			issueStatement += " (also data dep.)";
		}
}

void checkStructural(int ir2, int opcode1, int opcode2){
	//Checks for a structural stop occuring
	//where both slots would be lw/sw instructions
	if((opcode1 == 0x23 || opcode1 == 0x2b) &&
	   (opcode2 == 0x23 || opcode2 == 0x2b)){
			numStructuralStops++;
			doubleIssue = false;
			issueStatement = "// structural stop";
	}

	//Checks for a structural stop occuring
	//where both slots would be mul instructions
	if(opcode1==0x1c && opcode2==0x1c && funct==0x02){
		int func2 = ir2 & 0x2f; // clamps to the 6 bit funct
		if(func2 == 0x02){
			numStructuralStops++;
			doubleIssue = false;
			issueStatement = "// structural stop";
		}
	}
}

void checkControl(int ir2, int opcode1, int opcode2){
	controlStopFound = false;
	//Checks for control stop occuring when issue slot one holds beq, bgtz,
	// blez, or bne
	if(opcode1 == 0x04 || opcode1 == 0x07 || opcode1 == 0x06 || opcode1 == 0x05){
		numControlStops++;
		doubleIssue = false;
		controlStopFound = true;
		issueStatement = "// control stop";
	}

	//Checks for control stop occuring when issue slot one holds j or jal
	else if(opcode1 == 0x02 || opcode1 == 0x03){
		numControlStops++;
		doubleIssue = false;
		controlStopFound = true;
		issueStatement = "// control stop";
	}

	//Checks for control stop occuring when issue slot one holds jr or jalr
	else if(opcode1 == 0x00 && (funct==0x09 || funct==0x08)){
		numControlStops++;
		doubleIssue = false;
		controlStopFound = true;
		issueStatement = "// control stop";
	}
}

void checkDataHazard(int ir2, int opcode1, int opcode2){
	bool found = false;

	//Tries to find opcode1 in the map
	if(opcodeMap.find(opcode1) != opcodeMap.end() && opcode1 != 0x2b)

		//Finds if the function is I-type, if it is, sets storeRegister to rt
		if(opcodeMap.find(opcode1)->second.compare("r"))
			found = checkRAW(ir2, rt);
		else //Otherwise, storeRegister is set to rd
			found = checkRAW(ir2, rd);

	//Tries to find opcode1 in the map
	if(!found && opcodeMap.find(opcode2) != opcodeMap.end() && opcode1 != 0x2b)

		//Finds if the function is I-type, if it is, sets storeRegister to rt
		if(opcodeMap.find(opcode2)->second.compare("r"))
			checkWAW(ir2, rt, opcode2);
		else //Otherwise, storeRegister is set to rd
			checkWAW(ir2, rd, opcode2);
}

void determineSecondSlot(){
	int ir2, rd2, rs2, rt2, shift2,	funct2,	pc2=pc, storeRegister;

	//Checks for a halt in issue slot one, if there is one. Then don't continue.
	if(ir == 0){
		numControlStops++;
		doubleIssue = false;
		controlStopFound = true;
		issueStatement = "// control stop";
		return;
	}

	mar = pc2;
	mdr = ram[mar];
	ir2 = mdr;
	doubleIssue = true;
	issueStatement = "// -- double issue --";
	unsigned int opcode1 = (ir >> 26) & 0x3f; // clamp to 6-bit opcode field
	unsigned int opcode2 = (ir2 >> 26) & 0x3f; // clamp to 6-bit opcode field

	checkStructural(ir2, opcode1, opcode2);
	checkControl(ir2, opcode1, opcode2);

	if(!controlStopFound)
		checkDataHazard(ir2, opcode1, opcode2);
}

int main(){
	initiliazeRam();
	fillMap();
	gatherInput();
	void (* inst)();
	void (* inst2)();

 	while(halt == 0){
		numIssueCycles++;
		fetch();
		inst = decode();
		determineSecondSlot();
		(*inst)();

		//If double issue is possible, use it.
		if(doubleIssue){
			numDoubleIssue++;
			doubleIssue = false;
			fetch();
			inst2 = decode();
			(*inst2)();
			//Fixes the spacing
			cout << "  ";
		} 
		else //Otherwise, correct the print spacing
			cout << "             ";

		cout << issueStatement << "\r\n";
		if(zeroAttempt){
			zeroAttempt = false;
			registerArray[0] = 0;
		}
	}

	writeOutput();
    return 0;
}