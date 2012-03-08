#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "osdefs.h"
#include "externs.h"





void Logon_Service(){

	//Allocate new Process Control Block with all fields initialized to 0

	pcb_type* pcb = malloc(sizeof(struct pcb_type));
       
 
	pcb->current_prog = 0;
	pcb->num_segments = 0;
	pcb->served = 0;
	pcb->efficiency = 0;
	pcb->instrleft = 0;
	pcb->sliceleft = 0;
	pcb->sjnave = 0;
	pcb->sjnburst = 0;
        pcb->run_time.seconds = 0;
        pcb->run_time.nanosec = 0; 
	//Set logon time
	pcb->logon_time.seconds = Clock.seconds;
	pcb->logon_time.nanosec = Clock.nanosec;
  
	//Mark status as new
	pcb->status = NEW_PCB;
  
	//Set username
	pcb->username[0] = 'U';
	pcb->username[1] = '0';
	pcb->username[2] = '0';
	pcb->username[3] = (char)(((int)'0')+Agent);
  
	//Save terminal table position
	Term_Table[Agent-1] = pcb;
	pcb->term_pos = Agent-1;
  
	  //Initialize list of programs user will run.
	Get_Script(pcb);

	  //Execute first program of PCB
	Next_pgm(pcb);

	  //If objective is 4 or higher, set scheduling flags:
	if(Objective >= 4){
		  //If no process is active in the cpu currently
		  //turn off scheduling and CPU switches
		  if(CPU.active_pcb==NULL){

		  }
		  else{
			  //turn on switches
		  }
	  }
}


void Get_Script(pcb_type *pcb){

	int i,j=0,found=0,end=0;
	char script[80];
        char* hold = script;
	//Allocate PCB's array of scripts.
	
	pcb->script = malloc(sizeof(struct prog_type*) * Max_Num_Scripts);

	//Mark index of first script 

	pcb->current_prog = 0;

	//Read each script name in file

	while(j < Max_Num_Scripts)
        {
		found = 0;
		fscanf(Script_fp, "%s", hold);
		
		//Capitalize script name so that case does not matter
		hold = strupr(hold);

		//Output name of the script to output file
		//Stop reading script names when LOGOFF script name encountered
		for(i=0;i<7;i++)
                {
			if(strcmp(Prog_Names[i],script) == 0)
                        {
				pcb->script[j] = i;
                               if(i == 6)
                                 end = 1;
			      found = 1;
				break;
			}
		}
            if(found != 1)
              printf("IT WAAS NOT FOUND BUT SHOULD BE \n");
            if(end == 1)
            {
              print_out("            Script for user %s is:\n",pcb->username);
              print_out("	     ");
              for(i=0; i<= j; i++)
              {
                print_out("%s  ",Prog_Names[pcb->script[i]]);
              }
              print_out("\n\n");
              break;
            }
          j++;
	}

	
		
}


int Next_pgm(pcb_type* pcb){

	time_type clok;
       	clok.seconds = Clock.seconds;
	clok.nanosec = Clock.nanosec;

	if(pcb->current_prog != 0 && pcb->rb_q == NULL) 
	{
		Dealloc_pgm(pcb);
	}

	if(pcb->rb_q != NULL)
	{
		return 0; 
	}

	if(pcb->script[pcb->current_prog] == LOGOFF)
	{
           	pcb->status =  TERMINATED_PCB;
		Diff_time(&pcb->logon_time,&clok);
		pcb->logon_time.nanosec = clok.nanosec;
		pcb->logon_time.seconds = clok.seconds;
                return 0;
	}
        
	Get_Memory(pcb);
	//If failed to allocate space, return.
	if(pcb->seg_table == NULL){
		return 0;
	}

	Loader(pcb);

	pcb->cpu_save.mode = 0;
        pcb->cpu_save.pc.segment = 0;
        pcb->cpu_save.pc.offset = 0;
	pcb->status = 1;

	if(Objective >= 4){
		//Insert prcess into CPU's ready queue
	}

	pcb->current_prog += 1;

}

void Get_Memory(pcb_type* pcb){

	int position = pcb->script[pcb->current_prog];
	int numsegs = 0;
	int numinstrs = 0;
	int accbits = 0;
	int i,j=0;
	int memory_needed = 0;
	char holder[80];
	char* holdp = holder;


      		   //Read PROGRAM
	fscanf(Prog_Files[position],"%s", holdp);
	  
	   //Red number of segments
	fscanf(Prog_Files[position],"%d",&numsegs);
	  
         pcb->num_segments = numsegs; 
		   //Allocate Number of segments
	pcb->seg_table = calloc(numsegs,sizeof(struct segment_type));

	   //Read SEGMENTS
	for(i=0;i<numsegs;i++)
          {
			//Read segment
		fscanf(Prog_Files[position],"%s",holdp);
			//Read num-instructions
		fscanf(Prog_Files[position],"%d",&numinstrs);
 
			 //Read in access bits
		fscanf(Prog_Files[position],"%x",&accbits);
	  
		pcb->seg_table[i].access = accbits;
		pcb->seg_table[i].size = numinstrs;
				  
		memory_needed += numinstrs;
	   }//END READING SEGMENTS
	   
		//If we don't have enough memory quit
		if(memory_needed > Total_Free){
			free(pcb->seg_table);
			return;
		  }

		  while(j<numsegs){
			 pcb->seg_table[j].base = Alloc_seg(pcb->seg_table[j].size);
			 
			 if(pcb->seg_table[j].base == -1){
				Compact_mem();
				pcb->seg_table[j].base = Alloc_seg(pcb->seg_table[j].size);
				j++;
				continue;
			 }
			 j++;
		  }
}

int Alloc_seg(int size)
{
		int num = 0;
		seg_list* head = Free_Mem;
		seg_list* temp;

		//If it is the head of the list and exact size
		if(head->size == size)
		{
		  Free_Mem = head->next;
		  num = head->size;
		  free(head);
		  Total_Free -= head->size;
		  return(num);
 
		//If head and larger than needed
		}else if(size < head->size)
		{
		   num = head->base;
		   head->base += size;
		   head->size -= size;
		   Total_Free -= size;
		   return(num);
		}

		while(head->next != NULL)
		{
		   
		  
		   if(head->next->size == size)
		   {
			  temp = head->next;
			  num = head->next->base;
			  head = head->next->next;
			  free(temp);
			  Total_Free -= size;
			  return(num);

		   }else if(head->size > size)
		   {
			  temp = head->next;
			  num = head->next->base;
			  head->next->base += size;
			  head->next->size -= size;
			  Total_Free -= size;
			  return(num);
		   }
		   head = head->next;
		}
	return(-1);
}

void Loader(pcb_type* pcb)
{
   int i,j;
   
   for(i=0; i < pcb->num_segments; i++)
   {
      for(j=0; j < pcb->seg_table[i].size; j++)
	  Get_Instr(pcb->script[pcb->current_prog],&Mem[pcb->seg_table[i].base+j]);
       
      Display_pgm(pcb->seg_table,i,pcb);     
 
   }
   print_out("Program # %d, %s, has been loaded for user %s.\n",pcb->current_prog+1,
   Prog_Names[pcb->current_prog], pcb->username);
}

void Dealloc_pgm( pcb_type* pcb )
{
	int i;
     print_out("Program number %d, %s, has ended for user %s\n",pcb->current_prog+1,
     Prog_Names[pcb->current_prog],pcb->username);
	for(i=0;i<pcb->num_segments;i++)
   {
	 Dealloc_seg(pcb->seg_table[i].base,pcb->seg_table[i].size);
   }
	 free(pcb->seg_table);
}

void
Dealloc_seg( int base, int size )
{
   seg_list* temp = malloc(sizeof(struct seg_list));
   seg_list* head = Free_Mem;
	//Initialize new segment info
	temp->base = base;
	temp->size = size;

	//Increment Free Memory
	Total_Free++;

	if(head == NULL)
        {
	  head = temp;
          return;
        }
	//Temp belongs at the head of Free_Mem
	if(head->base > temp->base)
	 {
	   //Keep rest of list
	   temp->next = head;
	   head = temp;
           return;
	 }

	while(head->next != NULL)
	{
	   //Temp belongs between head and head next 
	  if(head->next->base > temp->base)
		{
		   temp->next = head->next;
		   head->next = temp;
		   Merge_seg(head,temp,head->next);
		  return;
		}
	   //Go to next node.
	   head = head->next;
	}

	//Temp belongs at the end of the list
	head->next = temp;
   return;

}


void Merge_seg(seg_list* prev_seg,seg_list* new_seg, seg_list* next_seg){

   seg_list* temp = malloc(sizeof(struct seg_list));
   
   if(prev_seg != NULL)
   {
	 if((prev_seg->base + prev_seg->size + 1) == new_seg->base){
		prev_seg->size += new_seg->size;
		temp = new_seg;
		prev_seg->next = new_seg->next;
		
		free(temp);
		
		new_seg = prev_seg;
	 }
   }
   
   if(next_seg != NULL)
   {
	  if((new_seg->base + new_seg->size + 1) == next_seg->base)
	  {
		 //Add the next segments size to our new segment.
		 new_seg->size += next_seg->size;
		 
		 new_seg->next = next_seg->next;
		 
		 free(next_seg);
	  }
   }
   

}		
		
void End_Service( )
{
    pcb_type* pcb;
    time_type* active_time;

	active_time = malloc(sizeof(struct time_type));
     //Get the pcb term_pos
    pcb = Term_Table[Agent-1];
    CPU.active_pcb = NULL;
   
    active_time->nanosec = pcb->run_time.nanosec;
    active_time->seconds = pcb->run_time.seconds;
    Diff_time( &Clock, active_time );                                               
    Add_time( active_time, &CPU.total_busy_time ); 
    
    //PCB is done.
    pcb->status = DONE_PCB; 

    //Set block time for CPU
    pcb->block_time.seconds = Clock.seconds;
    pcb->block_time.nanosec = Clock.nanosec;

    if(Objective >= 4)
    {
      //TODO Stuff.
    }

    if(pcb->rb_q == NULL)
    {
      Next_pgm(pcb);
    }

    if(Objective >= 4)
    {
      //TODO Stuff.
    }

}


/**
	Service segmentation fault and address fault events.

	<b> Important Notes:</b>
	\li This function services causes an abnormal end to the simulated program.
	\li Events of type SEGFAULT_EVT and ADRFAULT_EVT will result in this call
	via Interrupt_Handler() in simulator.c after interrupt() in obj1.c sets the
	global Event variable.
	\li Use print_out() function for output.

	<b> Algorithm: </b>
	\verbatim
	Print output message indicating that process was terminated due to some abnormal condition
	Call End_Service() to handle terminating process.
	\endverbatim

	@retval None
 */
void
Abend_Service( )
{
   print_out("Process was terminated due to abnormal condition\n");
   End_Service();
   return;
}

/**
	Print contents of free list.

	<b> Important Notes:</b>
	\li This function should be useful for debugging and verifying memory
	allocation and deallocation functions, especially Merge_seg().

	<b> Algorithm: </b>
	\verbatim
	For each free block
		Print base position and last position in block
	\endverbatim
 */
void print_free_list( )
{
  seg_list* temp = Free_Mem;

  while(temp != NULL)
  {
	 printf("The base is %u the end is %u \n",temp->base,temp->size);
	 temp = temp->next;
  }
}

/**
	Print debugging message about Term_Table.

	<b> Important Notes:</b>
	\li The debugging message is only printed if the variable DEBUG_PCB is
	turned on (set to 1). This is done by adding the following line to
	config.dat: <br>
	DEBUG_PCB= ON
	\li The function is not necessary to implement as it serves only to help
	the programmer in debugging the implementation of common data structures.

	@retval None
 */
void
Dump_pcb( )
{
	// if debug flag is turned on
	if( DEBUG_PCB )
	{
		/* print debugging message */
	}
	else // do not print any message
	{ ; }
}

