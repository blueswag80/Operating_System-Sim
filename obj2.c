
/**
	obj2.c


	Simulator Project for COP 4600

	Objective 2 project that implements loading and executing the boot
	program for the OS simulator.

	Revision List:
		Original Design:  Dr. David Workman, 1990
		Revised by Tim Hughey and Mark Stephens, 1993
		Revised by Wade Spires, Spring 2005
		Minor Revisions by Sean Szumlanski, Spring 2010
*/

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "osdefs.h"
#include "externs.h"

#define NUM_OPS 6
#define FALSE 0
#define TRUE 1

//GLOBAL LAZY VARIABLE
int instrnum = 0;


/**
	Simulate booting the operating system by loading the boot program into
	kernel memory.

	This function is called from simulator.c (the simulator driver)
	before processing the event list. Boot() reads in the file boot.dat to load
	the kernel and initialize Mem_Map for the first program execution. Note that
	boot.dat is already open when Boot() is called and can immediately be read
	using the file pointer Prog_Files[BOOT] (Prog_Files is an array of file
	pointers opened by the simulator).

	The upper half of Mem_Map always holds the address map for the operating
	system, i.e., Mem_Map[Max_Segments ... 2*Max_Segments - 1] (the lower half
	is reserved for user programs). The PROGRAM and SEGMENT statements in the
	boot file are used to initialize the appropriate entries in Mem_Map. The
	actual kernel script is loaded into Mem (array representing physical
	memory) starting at absolute memory location 0. The Free_Mem list
	and Total_Free must be updated as the program is loaded to account for
	the region of Mem that is occupied by the kernel. 

	The format of boot.dat is the same as the other "program.dat"
	files, so it is divided into two sections: segment table information and
	instructions. To load the OS program, Boot() calls the Get_Instr() routine
	to read and process each line in the data file. Once the line is read, it is
	then stored in Mem at the appropriate absolute memory address. Once
	the entire OS program is loaded, Boot() should call Display_pgm() to echo 
	the contents of Mem to the output file. 

	<b> Important Notes:</b>
	\li Boot() is only active during objective 2, so if the current objective is not 2, then simply return from the function.
	\li The boot file, boot.dat, is already open when Boot() is called and can immediately be read using the file pointer Prog_Files[BOOT] (Prog_Files is an array of file pointers opened by the simulator).
	\li Get_Instr() is used by the function Loader() in objective 3.

	<b> Algorithm: </b>
	\verbatim
	Read fields the "PROGRAM" and number of segments from boot file

	Read segment information from boot file:
		For each segment in boot file
			Read "SEGMENT size_of_segment access_bits" from boot file
			Set size of current segment
			Set access bits of current segment
			Go to next segment in Mem_Map

	Read instructions from boot file:
		Current position in main memory--incremented for each instruction read

		For each segment
			Set base pointer of segment to its location in main memory
			For each instruction in the segment
				Read instruction into memory--call Get_Instr()
				Increment current position in main memory
			Go to next segment in Mem_Map

		Adjust size of memory since boot program is loaded into memory:
			Decrement total amount of free memory (Total_Free)
			Decrement size of free memory block (Free_Mem->size)
			Move location of first free memory block (Free_Mem->base)

		Display each segment of memory
			Display segment Mem_Map[i + Max_Segments] since kernel resides in
			Upper half of Mem_Map; pass NULL as PCB since OS has no PCB
	\endverbatim

	@retval None
 */
void
Boot( )
{
      if(Objective != 2)
         return;
          

/* 	   if(strncmp(segp,holdp,8) != 0)
	   {
	      printf("Expect SEGMENT\n");
		  return;
	   }
 */	   


    char segs[] = "SEGMENT";
	char prgs[] = "PROGRAM";
	char* prgsp = prgs;
	char* segp = segs;
	char hold[80];
	char* holdp = hold;
        int accbits=0;
	int numsegs = 0;
	int numinstrs = 0;
	int base = 0;
	int i,j,memloc = 0;

	
	
	
	   //Read PROGRAM
	   fscanf(Prog_Files[BOOT],"%s",holdp);
	  
	   //Red digit after PROGRAM
	   fscanf(Prog_Files[BOOT],"%d",&numsegs);
	   
	   //Read SEGMENTS
	   for(i = 0; i < numsegs; i++)
	   {
	     //Read segment
	     fscanf(Prog_Files[BOOT],"%s",holdp);
		 //Read num-instructions
         fscanf(Prog_Files[BOOT],"%d",&numinstrs);
 
         //Read in access bits
         fscanf(Prog_Files[BOOT],"%x",&accbits);
         // printf("WHAT THE HELL???\n");
           //     printf("ACC BITSIFJIOEJOIJF %x\n",accbits);
		 //Store the read in results
		 Mem_Map[i + Max_Segments].access = accbits;
                 Mem_Map[i + Max_Segments].size = numinstrs;	  
        
	   }
	   
	   for(j = 0; j < numsegs; j++)
	   {
                Mem_Map[j + Max_Segments].base = memloc;

	     for(i = 0; i < Mem_Map[j + Max_Segments].size; i++)
             {	  
	           Get_Instr(BOOT,&Mem[memloc]);
		 
                   memloc++;
             }
         
	   }	
         
	       	Total_Free = Total_Free - memloc;
                Free_Mem->size = Free_Mem->size - memloc;
                Free_Mem->base = memloc;

        for(i = 0; i < numsegs; i++)
        {
           Display_pgm(Mem_Map,i,NULL);
        }
   //Prog_Files[BOOT]
   
}

/**
	Read the next instruction from the file Prog_Files[prog_id] into
	instruction.  

	The format of the file is a series of statements of the form
	'opcode operand'. opcode may be an actual instruction, such as SIO, or a
	device, such as DISK. The operand will vary depending on the type of opcode
	read. Each instruction starts on a new line, but consecutive instructions
	may be separated by empty lines. 

	<b> Important Notes:</b>
	\li The operand field of an instr_type structure is defined as a C union, so
	it may represent one of many values but only one at a time. For example,
	instruction->operand.burst is set for an SIO instruction, but
	instruction->operand.count is set for a SKIP instruction.
	\li For device instruction, assign unique opcodes to each device by adding
	NUM_OPCODES to its corresponding dev_id so device codes do not conflict with
	regular op_codes
	\li Get_Instr() is used by Boot() in objective 2 and Loader() in objective
	3.

	<b> Algorithm: </b>
	\verbatim
	Read opcode and operand from file--store as strings initially Convert to uppercase so that case does not matter

	Search all op_code IDs to determine which op_code that the opcode_str matches with
		If found the correct op_code
			Save the opcode in the instruction

			Process the operand differently depending on the op_code type:
				If SIO, WIO, or END instruction
					Convert operand into burst count
				Else, if REQ or JUMP instruction
					Convert operand into segment and offset pair
				Else, if SKIP instruction
					Convert operand into skip count

	If no op_codes matched, the op_code may correspond to a device
		Search device table for the particular device
			If found the correct device ID
				Assign a unique opcode for the device (See note above)
				Convert operand into number of bytes to transfer
		If no devices matched, quit program
	\endverbatim 


	@param[in] prog_id -- index into Prog_Files to read instruction from
	@param[out] instruction -- location to store instruction read
 */
void
Get_Instr( int prog_id, struct instr_type* instruction )
{
    int i=0;
	int found=FALSE;
	unsigned long opnum = 0;
	unsigned long opnumpt2 = 0;
	char opnums[6];
	char* opnumspt = opnums;
	char opname[5];
    char* opnamept = opname;

     	 //Get the first part of the instruction
         fscanf(Prog_Files[prog_id],"%s",opname);
         //Get the number associated with the opcode
	 fscanf(Prog_Files[prog_id],"%s",opnumspt);
              
         //Make sure event is not to long.	 
	if(opname == NULL || strlen(opname) > 5)
	     printf("Something is wrong with this name\n");
		
          //Find event name
       
	 while( i < NUM_OPS)
	 {
	     if(strncmp(opname,Op_Names[i],NUM_OPS-1) == 0)
                {
		   found = TRUE;
                   break;
		}  
		   i++;
	 }
     
	 if(strtoul(opnumspt,NULL,0))
	    opnum = strtoul(opnumspt,NULL,0);
	else
	  {
             //Get the first number in [1,3]
	     opnum = strtoul(&opnumspt[1],NULL,0);
             //Get second number
	     opnumpt2 = strtoul(&opnumspt[3],NULL,0);
	  }
	 

	 if(i < 2 || i == END_OP)
       {
	     instruction->opcode = i;
		 instruction->operand.burst = opnum;
		 return;
		 
	   }else if(i == REQ_OP || i == JUMP_OP)
	   {
                 instruction->opcode = i;
	         instruction->operand.address.segment = opnum;
		 instruction->operand.address.offset = opnumpt2;
		 return;
		 
	   }else if(i == SKIP_OP)
	   {
              instruction->opcode = i;
	      instruction->operand.count = opnum;
		  return;
	   }
	   
	   i=0;
	   
	   while( i < Num_Devices)
	 {
	     if(strncmp(opname,Dev_Table[i].name,DEV_NAME_LENGTH-1) == 0)
                {
		   found = TRUE;
                   break;
		}  
		   i++;
	 }
	 
	 if(!found)
         err_quit("NO DEVICES FOUND");
	 
	 instruction->opcode = (i + NUM_OPCODES);
	 instruction->operand.bytes = opnum;
	 return;
   
}

/**
	Simulate the Central Processing Unit.

	The CPU, in general, does the following:
	\li Fetches the instruction from memory (Mem)
	\li Interprets the instruction.
	\li Creates a future event based on the type of instruction and
	inserts the event into the event queue.

	Set_MAR() is called to define the next memory location for a Fetch().
	The first fetch from memory is based on CPU.state.pc. Instructions that
	create events are:
	\li WIO   							
	\li SIO							
	\li END							
	The SKIP and JUMP instructions are executed in "real-time" until
	one of the other instructions causes a new event.

	An addressing fault could be generated by a call to Fetch() or Write(), so
	Cpu() should just return in this case (the simulator will later call
	Abend_Service() from the interrupt handler routine to end the simulated
	program).

	<b> Important Notes:</b>
	\li Use a special agent ID, i.e., 0, to identify the boot program. For
	all other objectives, the agent ID should be retrieved from the CPU's
	currently running process--use this process's terminal position and add 1
	so that it does not conflict with boot.
	\li The only instructions processed by Cpu() are SIO, WIO, END, SKIP, and
	JUMP. No other instructions, such as REQ and device instructions, are
	encountered in Cpu(). This is by incrementing the program counter by 2 to
	bypass these unused instructions.
	\li A while(1) loop is needed to correctly evaluate SKIP and JUMP
	instructions since multiple SKIP and JUMP instructions may be encountered
	in a row. The loop is exited when an SIO, WIO, or END instruction is
	encountered.
	\li The function Burst_time() should be used to convert CPU cycles for SIO,
	WIO, and END instructions to simulation time (time_type).

	<b> Algorithm: </b>
	\verbatim
	Identify the agent ID for the currently running program:
		If CPU has no active PCB, then the program is boot
		Otherwise, the program is the active process running in the CPU, so use its ID

	Loop forever doing the following:
		Set MAR to CPU's program counter
		Fetch instruction at this address
		If fetch returns a negative value, a fault has occurred, so
			Return
	
		Determine type of instruction to execute
			If SIO, WIO, or END instruction
				If the objective is 3 or higher
					Increment total number of burst cycles for PCB
				Calculate when I/O event will occur using current time + burst time
				Add event to event list
				Increment PC by 2 to skip the next instruction--device instruction
				Return from Cpu() (exit from loop)

			If SKIP instruction
				If count > 0,
					Decrement count by 1
					Write this change to memory by calling Write()
					If write returns a negative value, a fault has occurred, so
						Return
					Increment the CPU's PC by 2 since the next instruction is to be skipped
				Otherwise, instruction count equals 0, so
					Increment the CPU's PC by 1 since the next instruction, JUMP, is to be executed
				Continue looping

			If JUMP instruction
				Set the PC for the CPU so that the program jumps to the address determined by the operand of instruction
				Continue looping
	\endverbatim 

	\retval None
 */
void
Cpu( )
{
  instr_type* instr = (instr_type*)malloc(sizeof(instr_type));
  time_type* time = (time_type*)malloc(sizeof(time_type));
  
  while(TRUE)
  {
    
    Set_MAR(&CPU.state.pc);
    
     if(Fetch(instr) < 0)
       {
         free(time);
         free(instr);
         return;
       }
 
       if(instr->opcode == SIO_OP || instr->opcode == WIO_OP || instr->opcode == END_OP)
       {
             // if(Objective >= 3)
            // {
                //Increment total number of burst cycles for PCB
               //CPU.active_pcb->sjnburst++;
              //}

          Burst_time(instr->operand.burst,time);
          Add_time(&Clock,time);

          if(instr->opcode == SIO_OP)
          {
            Add_Event(SIO_EVT,0,time);
          }else if(instr->opcode == WIO_OP)
          {
            Add_Event(WIO_EVT,0,time);
          }else if(instr->opcode == END_OP)
          {
            Add_Event(END_EVT,0,time);
          }
          CPU.state.pc.offset +=2;
          return;

       }else if(instr->opcode == SKIP_OP)
	   {
	      if(instr->operand.count > 0)
		  {
		     instr->operand.count -= 1;
			 
			 if(write(instr) == -1)
			 {
			    return;
			 }
			 CPU.state.pc.offset += 2;
		  }else
		  {
		     CPU.state.pc.offset += 1;
			 continue;
		  }
	   }else if(instr->opcode == JUMP_OP)
	   {
	     CPU.state.pc.segment = instr->operand.address.segment;
             CPU.state.pc.offset = instr->operand.address.offset;
		 continue;
	   }
  }
}

/**
	Simulate a context switch that places a user program in execution.

	<b> Algorithm: </b>
	\verbatim
	Copy the given state into the CPU's state
	Resume execution of the program at the new PC--just call Cpu()
	\endverbatim

	@param[in] state -- program state to load into CPU
 */
void
Exec_Program( struct state_type* state )
{
   CPU.state = *state;
   Cpu();
   return;
}

/**
	Simulate the Memory Management Unit (MMU) that translates the contents of
	the Memory Address Register (MAR) to a real physical address.

	Use the contents of MAR = [segment,offset] as the logical address to be
	translated to a physical address.

	<b> Algorithm: </b>
	\verbatim
	Set segment to the segment saved in the MAR:
		If in kernel mode (CPU's mode equals 1)
			Set segment to be in upper half of Mem_Map (MAR.segment+Max_Segments)
		Otherwise, in user mode (CPU's mode equals 0)
			Just use lower half of Mem_Map (MAR.segment)

	If illegal memory access (access == 0)
		Create seg. fault event at the current time using the CPU's active process's terminal position (+ 1) as the agent ID
		Return -1

	If address is out of the bounds
		Create address fault event at the current time using the CPU's active process's terminal position (+ 1) as the agent ID
		Return -1

	Compute physical memory address:
		Address = base address from segment in Mem_Map + offset saved in MAR

	Return address
	\endverbatim

	@retval address -- physical address into Mem obtained from logical address
		stored in MAR
 */
int
Memory_Unit( )
{
     int seg = 0;
    

	if(CPU.state.mode == 1)
	{
          
          seg = (MAR.segment + Max_Segments);

        }else
        {
          seg = MAR.segment;
        }
	
	if(Mem_Map[seg].access == 0)
          { 
            
              Add_Event(SEGFAULT_EVT,CPU.active_pcb->term_pos + 1, &Clock);
              return -1;
	  }

	if(seg > (Max_Segments*2-1))
          {
            Add_Event(ADRFAULT_EVT,(CPU.active_pcb->term_pos) + 1, &Clock);
	    return -1;
          }	

	return(Mem_Map[seg].base + MAR.offset);
}

/**
	Set Memory Address Register (MAR) to the value of the given address.

	This function must be called to define the logical memory address of the
	next Fetch(), Read(), or Write() operation in memory.

	<b> Algorithm: </b>
	\verbatim
	Set MAR to given address
	\endverbatim

	@param[in] addr -- address to set MAR to
 */
void
Set_MAR( struct addr_type* addr )
{
     MAR.offset = CPU.state.pc.offset;
     MAR.segment = CPU.state.pc.segment;
     return;
}

/**
	Fetch next instruction from memory. 

	<b> Important Notes:</b>
	\li Fetch() is identical to Read()

	<b> Algorithm: </b>
	\verbatim
	Get current physical memory address--call Memory_Unit()
	If returned address < 0, then a fault occurred
		Return -1

	Set given instruction to instruction in memory at the current memory address
	Return 1 to signify that no errors occurred
	\endverbatim

	@param[out] instruction -- location to store instruction fetched from memory
	@retval status -- whether an error occurred or not (-1 or 1)
 */
int
Fetch( struct instr_type* instruction )
{
	int addr = 0;
	
	addr = Memory_Unit();
	
	if(addr < 0)
	  return(-1);
	  
         *instruction = Mem[addr];
	  
	return(1);
}

/**
	Read next instruction from memory. 

	<b> Important Notes:</b>
	\li Read() is identical to Fetch()

	<b> Algorithm: </b>
	\verbatim
	Get current physical memory address--call Memory_Unit()
	If returned address < 0, then a fault occurred
		Return -1

	Set given instruction to instruction in memory at the current memory address
	Return 1 to signify that no errors occurred
	\endverbatim

	@param[out] instruction -- location to store instruction read from memory
	@retval status -- whether an error occurred or not (-1 or 1)
 */
int
Read( struct instr_type* instruction )
{
     int addr = 0;
	 
	 addr = Memory_Unit();
	 
	 if(addr < 0)
	 return(-1);
	
	*instruction = Mem[addr];
	
	return(1);
}

/**
	Write instruction to memory. 

	<b> Algorithm: </b>
	\verbatim
	Get current physical memory address--call Memory_Unit()
	If returned address < 0, then a fault occurred
		Return -1

	Set instruction in memory at the current memory address to given instruction
	Return 1 to signify that no errors occurred
	\endverbatim

	@param[in] instruction -- instruction to write to memory
	@retval status -- whether an error occurred or not (-1 or 1)
 */
int
Write( struct instr_type* instruction )
{
	int addr = 0;
	
	addr = Memory_Unit();
	
	if(addr < 0)
	   return(-1);
	
        Mem[addr] = *instruction;
	 	
	return(1);
}

/**
	Display contents of program to output file.

	Refer to intro.doc for the proper output format.
	Only a single segment (seg_num) of the segment table is displayed per
	call of Display_pgm(). Multiple calls of Display_pgm() must be made to
	display the entire table.

	<b> Important Notes:</b>
	\li Call print_out() to print to the output file.
	\li For objective 2, the given pcb will be NULL, so print the name as
	"BOOT".
	\li For objectives other than 2, use
	Prog_Names[ pcb->script[ pcb->current_prog ] ] to get the program name.
	Prog_Names is a global array holding the names of all programs the
	simulator can run. pcb->script is an array of all the programs that the
	process has (or will) run, and pcb->current_prog is the index into
	pcb->script of the program that the user is currently running, that is, the
	one that needs to be displayed.
	\li If instruction is not a regular instruction, e.g., not SIO, WIO, etc.,
	then it is a device. The location of the device in Dev_Table (to get its
	name) is obtained by taking the opcode of the current memory position -
	NUM_OPCODES since NUM_OPCODES was added to device op_codes in Get_Instr().

	<b> Algorithm: </b>
	\verbatim
	Print segment header:
		If boot process
			Print "BOOT"
		Else, if user process
			Print name of user's program
	Print additional header information

	Display current segment of memory:
		Get base memory position of the first instruction in the segment

		For each instruction in the segment
			If opcode is an instruction, look for the appropriate one in Op_Names
				Display memory position and instruction code

				Determine instruction type to display operand differently:
					If SIO, WIO, or END instruction
						Display burst count

					Else, if REQ or JUMP instruction
						Display segment/offset pair

					Else, if SKIP instruction
						Display skip count
			Else, opcode is a device
				Look for it in devtable
				Display memory position, device name, and byte count

			Update base memory position to go to next instruction in segment
	\endverbatim

	nt)@param[in] seg_table -- segment table to display
	@param[in] seg_num -- segment within table to display (seg_table[ seg_num ])
	@param[in] pcb -- process control block that the segment table belongs to
	(NULL if table belongs to boot process)
 */
void
Display_pgm( segment_type* seg_table, int seg_num, pcb_type* pcb )
{
   int base_mem = seg_table[seg_num].base;
   int i,dev_place,operand;
 
 // printf("%d\n",seg_num); 
   
   print_out("SEGMENT #%d OF PROGRAM %s OF PROCESS %s \n",seg_num,Prog_Names[pcb->current_prog],
   pcb->username );
   print_out("ACCBITS: 0x%x LENGTH: %u\n",seg_table[seg_num].access,seg_table[seg_num].size);
   print_out("MEM ADDR  OPCODE  OPERAND\n");
   print_out("--------  ------  -------\n");
   //  printf("%d\n",seg_table[seg_num].size);  
   for(i = 0; i < seg_table[seg_num].size ; i++)
   {
      operand = Mem[base_mem+i].opcode;
      if(operand == SIO_OP || operand == WIO_OP ||operand == END_OP)
	  {
        print_out("    %d    %s    %d\n",instrnum,Op_Names[operand],Mem[base_mem].operand.burst);  
		
	  }else if(operand == REQ_OP || operand == JUMP_OP)
	  {
	    print_out("    %d    %s    [%d,%d]\n",instrnum,Op_Names[operand],Mem[base_mem].operand.address.segment,Mem[base_mem].operand.address.offset);
          }
          else if(operand == SKIP_OP)
          {
            print_out("    %d    %s     %d\n",instrnum,Op_Names[operand],Mem[base_mem].operand.count);

          } else
	  {
	     dev_place = (operand - NUM_OPCODES);
		 
		 print_out("    %d    %s    %d\n",instrnum,Dev_Table[dev_place].name,Mem[base_mem].operand.bytes);
		 
	  }
	  instrnum++;
      }	  
   print_out("\n");
 }	  

/**
	Print debugging message about Mem.

	<b> Important Notes:</b>
	\li The debugging message is only printed if the variable DEBUG_MEM is
	turned on (set to 1). This is done by adding the following line to
	config.dat: <br>
	DEBUG_MEM= ON
	\li The function is not necessary to implement as it serves only to help
	the programmer in debugging the implementation of common data structures.

	@retval None
 */
void
Dump_mem( segment_type* seg_tab )
{
	// if debug flag is turned on
	if( DEBUG_MEM )
	{
		/* print debugging message */
	}
	else // do not print any message
	{ ; }
}
