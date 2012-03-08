/**
	simulator.c


	Simulator Project for COP 4600

	Revision List:
		Original Design:  Dr. David Workman, 1990
		Revised by Tim Hughey and Mark Stephens, 1993
		Revised by Wade Spires, Spring 2005
		Minor Revisions by Sean Szumlanski, Spring 2010
*/

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> 
#include <string.h>
#include "osdefs.h"
#include "externs.h"

/*
	Definitions of global simulator variables with initial values.
	Note that some values will change after the configuration file is read and
	processed.
 */

/** Current programming objective to run. */
int    Objective = 0;

char  Last_Name[BUFSIZ];  ///<
char  Outfile[BUFSIZ];    ///< Name of output file (Last_Name.dat)

/*
	File pointers and names
 */
FILE*  Out_fp    = NULL;  // output file pointer for simulator
FILE*  Script_fp = NULL;  // script file pointer for script.dat

// File pointers to each program script
FILE*  Prog_Files[ NUM_PROGRAMS - 1 ];

// Strings for input and output matching
char*  Prog_Names[]  = { "EDITOR", "PRINTER", "COMPILER", "LINKER", "USER",
	"BOOT", "LOGOFF" };

/** Name of program files (also referred to as scripts). */
char*  Prog_File_Names[] =
	{ "editor.dat", "printer.dat", "compiler.dat", "linker.dat", "user.dat",
	"boot.dat" };

// Names of events
char* Event_Names[] =
	{ "LOGON", "SIO", "WIO", "EIO", "END", "TIMER", "SEGFAULT", "ADRFAULT" };

/** Table of operator names. Note that NUM_OPCODES = number of opcodes. */
char* Op_Names[] =
	{ "SIO", "WIO", "REQ", "JUMP", "SKIP", "END" };

/** Control switch for the CPU (ON or OFF). */
control_switch_type  CPU_SW;

/** Control switch for the Scheduler (ON or OFF). */
control_switch_type  SCHED_SW;

/**
	Current user running in system.
	User terminal have agent codes in the range 1 to Num_Terminals (inclusive).
	Devices have agent codes in the range (Num_Terminals + 1) to 
	(Num_Terminals + Num_Devices) (inclusive).
	Changed after each call of the function Interrupt() in obj1.c.
 */
int  Agent;

/**
	Current interrupt event being handled.
	Changed after each call of the function Interrupt() in obj1.c.
 */
event_type  Event;

/**
	Current simulation time.
	Changed after each call of the function Interrupt() in obj1.c.
 */
time_type  Clock = { 0, 0 };

// Variables used for CPU statistics
int        Max_Jobs  = 0;       // Total jobs processed
time_type  Tot_Logon = {0, 0};  // Total job  processing time
time_type  Tot_Wait  = {0, 0};  // Total job  wait       time
time_type  Tot_Block = {0, 0};  // Total job  blocked    time
time_type  Tot_Run   = {0, 0};  // Total job  execution  time
time_type  Avg_Run   = {0, 0};  // Average user execution time
time_type  Avg_Wait  = {0, 0};  // Average user wait      time
time_type  Avg_Block = {0, 0};  // Average user blocked   time
time_type  Avg_Logon = {0, 0};  // Average user logon     time

// Variable used with round-robin scheduling
timer_type Timer =
{
	{0, 0},  // time_out
	0,       // instr_slice
	{0, 0}   // time_slice
};

/**
  Interrupt saved state area.
 */
state_type Old_State =
{
	1,      // mode--kernel mode
	{0, 0}  // pc--interrupt handler entry point
};

/**
	New Interrupt state.
 */
state_type New_State =
{
	1,      // mode--kernel mode
	{0, 0}  // pc--interrupt handler entry point
};

/**
	Variable representing CPU.

	<b> Important Note:</b>
	\li The mode field in CPU's state must be initialized to 1 (for kernel mode)
	prior to use.
 */
cpu_type CPU =
{
	NULL,        // active_pcb
	NULL,        // ready_q
	{1, {0, 0}}, // state -- mode must be initialized to 1 for kernel mode
	0,           // rate
	0,           // CPU_burst
	{0, 0},      // total_busy_time
	{0, 0},      // total_q_time
	{0, 0},      // response
	{0, 0},      // total_idle_time
	0,           // served
	0.0          // utilize
};

/*
	Variables for simulator memory structures
 */

/** Maximum size of memory map (must be a power of 2) */
int           Max_Segments = 1;

/** Memory address mapping table (size = 2*Max_Segments). */
segment_type* Mem_Map = NULL;

/** Memory size (length of Mem array). */
int           Mem_Size = 1000;

/** Processor memory array (representes physical memory). */
instr_type*   Mem = NULL;

/** List of free memory segments ordered by memory base address. */
seg_list*     Free_Mem = NULL;

/** Total amount of free memory currently available. */
unsigned int  Total_Free = 1000;

/** Memory address register. */
addr_type     MAR;

/* Operating System tables for device and terminal management */

/** Device table (list of devices). */
device_type*  Dev_Table  = NULL;

/** Terminal table (array of PCB pointers) */
pcb_type**    Term_Table = NULL;

event_list*   Event_List = NULL;  // pointer to head of event list

// Control variables
int  Num_Devices      =  0;   // Number of devices
int  Num_Terminals    =  0;   // Number of terminals
int  Max_Num_Scripts  =  10;  // Maximum number of program scripts

// Variables used for scheduling alogrithms
sched_type     Sched_Alg = FCFS;  ///< Current CPU scheduling algorithm
unsigned long  Time_Unit =  SEC;  ///< Time units for LOGON events
unsigned int   One_Over_Beta;     ///< expected CPU burst (SJN)
double         RHO       =  0.0;  ///< SJN Smoothing factor

// Debugging flags.
int  DEBUG_EVT     = 0;  // Flag controlling Event_List debug output
int  DEBUG_MEM     = 0;  // Flag controlling Mem debug output
int  DEBUG_PCB     = 0;  // Flag controlling Term_Table debug output
int  DEBUG_RBLIST  = 0;  // Flag controlling pcb.rb_q debug output
int  DEBUG_DEVQ    = 0;  // Flag controlling device.wait_q debug output
int  DEBUG_CPUQ    = 0;  // Flag controlling CPU.ready_q debug output

/**
	Driver for simulation.

   <b> Algorithm: </b>
   \verbatim
	Read configuration file

	Initialize list of events from logon file

	Open simulation output file (and print initial header information)

	Load boot program from file into memory

	For each event in the event list
		Disable running of CPU and scheduling when interrupt occurs

		Next interrupt occurs--get next event

		Diagnose and service interrupt
			Quit if memory fault occurs

		If scheduling is enabled
			Determine next process to run

		If have a process to run
			If able to run a process
				Execute the program
			Otherwise, the CPU is disabled for handling I/O interrupt
				Save context of currently active process
		Otherwise, no active processes
			Process next interrupt/event (loop)

		If objective 2
			Run boot program 
	Write statistics, close files, etc.
   \endverbatim

 */
int
main( int argc, char** argv )
{
	// read configuration file
	Init();

	// initialize list of events from logon file
	Load_Events();

	// open simulation output file (and print initial header information)
	Open_Output( Outfile );

	printf( "Starting simulation\n" );

	// load boot program from file into memory
	Boot();

	// for each event in the event list
	while( Event_List != NULL )
	{
		// disable running of CPU and scheduling when interrupt occurs
		CPU_SW = SCHED_SW = OFF;

		// next interrupt occurs--get next event
		Interrupt();

		if( Objective == 1 )
		{
			continue;
		}

		// diagnose and service interrupt
		if( Interrupt_Handler() )
		{
			// quit if memory fault occurs (in objective 2)
			break;
		}

		// if scheduling is enabled
		if( Objective >= 4 && SCHED_SW == ON )
		{
			// determine next process to run
			CPU.active_pcb = Scheduler();
		}

		// if have a process to run
		if( Objective >= 4 && CPU.active_pcb != NULL )
		{
			// if able to run a process
			if( CPU_SW == ON )
			{
				// execute the program
				Dispatcher();
			}
			else // CPU disabled for handling I/O interrupt
			{
				CPU.state = CPU.active_pcb->cpu_save;
			}
		}
		else
		{ /* currently no active processes--loop */ }

		// run boot program
		if( Objective == 2 )
		{
			Exec_Program( &Old_State );
		}
	}

	// write statistics, close files, etc.
	Clean_Up();
	printf( "Ending simulation\n" );

	return( 0 );
}

/**
	Handle interrupt by determining 

	Determine type of interrupt and call respective routine to service the
	interrupt.

	Interrupt handler is in control (with all interrupts inhibited).
	Save the state of the interrupted program (if the CPU is busy):
	1. Old_State is copied into CPU.active_pcb->cpu_save
	2. UserMap is copied into *CPU.active_pcb->segtable; (optional)

	@retval status -- Whether to continue the simulation (0) or not (1)
 */
int
Interrupt_Handler( )
{
	int status = 0;

	// CPU is busy, save state of running process
	if( CPU.active_pcb != NULL )
	{
		CPU.active_pcb->cpu_save = Old_State;
	}

	switch( Event )
	{
		case LOGON_EVT:
			Logon_Service();
			break;

		case SIO_EVT:
			Sio_Service();
			break;

		case WIO_EVT:
			Wio_Service();
			break;

		case EIO_EVT:
			Eio_Service();
			break;

		case END_EVT:
			if( Objective == 2 )
			{
				status = 1;
			}
			End_Service();
			break;

		case SEGFAULT_EVT:
		case ADRFAULT_EVT:
			Abend_Service();
			if( Objective == 2 )
			{
				status = 1;
			}
			break;

		case TIMER_EVT:
			Timer_Service();
			break;

		default:
			err_quit( "Interrupt_Handler: invalid interrupt event\n" );
	}

	return( status );
}

/**
	Read configuration file and initialize simulator environment.

	@retval None
 */
void
Init( )
{
	Process_Config_File();

	Open_Files();

	Allocate_Tables();
}

/**
	Read and process configuration file.

	Each line of the 

	@retval None
 */
void
Process_Config_File( )
{
	FILE*  config_fp;      // pointer to configuration file
	char   field[BUFSIZ];  // single field (word) read from file
	int    num_items;      // number of items read by fscanf()
	int    name_length;    // length of output file name read from file 
	int    i;              // loop variable

	/*
		flags to determine whether the given value was read from the configuration
		file for error-checking--initially set to false (0) for all fields 
		For example, the objective field must be set to some nonzero integer.
	 */
	int    one_over_beta = 0;  // whether (1 / Beta) was read
	int    quantum       = 0;  // whether quantum unit was read
	int    objective     = 0;  // whether objective number was read
	int    lname         = 0;  // whether last name was read from

	if( ( config_fp = fopen( CONFIG_FILENAME, "r" ) ) == NULL  )
	{
		err_quit( " %s file does not exit! \n", CONFIG_FILENAME );
	}

	// read each line of configuration file
	while( fscanf( config_fp, "%s", field ) != EOF )
	{
		if( !strcmp( field, "TIME=" ) )
		{
			if( (num_items = fscanf( config_fp, "%s", field )) == EOF
				|| num_items != 1 )
			{
				err_quit( " MISSING TIME= PARAMETER \n" );
			}
			if( !strcmp( field, "MIN" ) )
			{
				Time_Unit = MIN;
			}
			else if( !strcmp( field, "SEC" ) )
			{
				Time_Unit = SEC;
			}
			else if( !strcmp( field, "MSEC" ) )
			{
				Time_Unit = MSEC;
			}
			else if( !strcmp( field, "mSEC" ) )
			{
				Time_Unit = mSEC;
			}
			else if( !strcmp( field, "NSEC" ) )
			{
				Time_Unit = NSEC;
			}
			else
			{
				err_quit( " UNRECOGNIZABLE TIME= PARAMETER \n" );
			}
		}
		else if( !strcmp( field, "OBJECTIVE=" ) )
		{
			if( ( num_items = fscanf( config_fp, "%d", &Objective ) ) == EOF
				|| num_items < 1 )
			{
				err_quit( " MISSING OBJECTIVE= PARAMETER \n" );
			}
			objective = 1;
		}
		else if( !strcmp( field, "RRQUANTUM=" ) )
		{
			if( ( num_items = fscanf( config_fp, "%u",
				(unsigned int*) &Timer.instr_slice ) ) == EOF
				|| num_items < 1)
			{
				err_quit( " MISSING RRQUANTUM= PARAMETER \n" );
			}
			quantum = 1;
		}
		else if( !strcmp( field, "1/BETA=" ) )
		{
			if( ( num_items = fscanf( config_fp, "%u", &One_Over_Beta ) ) == EOF
				|| num_items < 1 )
			{
				err_quit( " MISSING 1/BETA= PARAMETER \n" );
			}
			one_over_beta = 1;
		}
		else if( !strcmp( field, "RHO=" ) )
		{
			float temp;
			if( ( num_items = fscanf( config_fp, "%f", &temp ) ) == EOF
				|| num_items < 1 )
			{
				err_quit( " MISSING RHO= PARAMETER \n" );
			}
			RHO = (double) temp;
			if( RHO < 0.0 || RHO > 1.0 )
			{
				err_quit( " INVALID RHO= PARAMETER \n" );
			}
		}
		else if( !strcmp( field, "TERMINALS=" ) )
		{
			if( ( num_items = fscanf( config_fp, "%d", &Num_Terminals ) ) == EOF
				|| num_items < 1 )
			{
				err_quit( " MISSING TERMINALS= PARAMETER \n" );
			}
			if( Num_Terminals <= 0 )
			{
				err_quit( " INVALID TERMINALS= PARAMETER \n" );
			}
		}
		else if( !strcmp( field, "DEVICES=" ) )
		{
			// read number of devices
			if( (num_items = fscanf( config_fp, "%d", &Num_Devices )) == EOF
				|| num_items < 1 )
			{
				err_quit( " MISSING DEVICES= PARAMETER \n" );
			}
			if( Num_Devices <= 0 )
			{
				err_quit( " INVALID DEVICES= PARAMETER \n" );
			}

			// allocate Device Table
			Dev_Table = calloc( Num_Devices, sizeof( device_type ) );
			if( Dev_Table == NULL )
			{
				err_quit( " DEVICE TABLE ALLOCATION FAILURE! \n" );
			}
			{ // local block for initializing devices
				unsigned int bytes_per_sec;  // speed of device (read from file)

				// read description for each device
				for( i = 0; i < Num_Devices; i++)
				{
					if( (num_items = fscanf( config_fp, "\nID= %4c RATE= %u ",
							Dev_Table[i].name, &bytes_per_sec )) == EOF
						|| num_items < 2 )
					{
						err_quit(
							" MISSING DEVICE= SPECIFICATION PARAMETER( s ) \n" );
					}

					// initialize entry in device table
					Dev_Table[i].bytes_per_sec = bytes_per_sec;
					Dev_Table[i].nano_per_byte =
						(double) 1.0e9 / (double) bytes_per_sec;
					Dev_Table[i].total_q_time.seconds = 0;
					Dev_Table[i].total_q_time.nanosec = 0;
					Dev_Table[i].total_busy_time = Dev_Table[i].total_q_time;
					Dev_Table[i].num_served = 0;
				}
			} // end block
		}
		else if( !strcmp( field, "MEMSIZE=" ) )
		{
			if( (num_items = fscanf( config_fp, "%u", &Mem_Size )) == EOF
				|| num_items < 1 )
			{
				err_quit( " MISSING MEMSIZE= PARAMETER \n" );
			}
		}
		else if( !strcmp( field, "CPURATE=" ) )
		{
			if( (num_items = fscanf( config_fp, "%u", (unsigned int*) &CPU.rate ))
				== EOF || num_items < 1 )
			{
				err_quit( " MISSING CPURATE= PARAMETER \n" );
			}
		}
		else if( !strcmp( field, "MAXSCRIPT=" ) )
		{
			if( (num_items = fscanf( config_fp, "%d", &Max_Num_Scripts )) == EOF
				|| num_items < 1 )
			{
				err_quit( " MISSING MAXSCRIPT= PARAMETER \n" );
			}
			if( Max_Num_Scripts <= 0 )
			{
				err_quit( " INVALID MAXSCRIPT= PARAMETER \n" );
			}
		}
		else if( !strcmp( field, "LNAME=" ) )
		{
			if( ( num_items = fscanf( config_fp, "%s", field ) ) == EOF
				|| num_items < 1 )
			{
				err_quit( " MISSING LNAME= PARAMETER \n" );
			}
			lname = 1;
			strcpy( Last_Name, field );
		}
		else if( !strcmp( field, "SCHED=" ) )
		{
			if( (num_items = fscanf( config_fp, "%s", field )) == EOF
				|| num_items < 1 )
			{
				err_quit( " MISSING SCHED= PARAMETER \n" );
			}
			if( !strcmp( field, "FCFS" ) )
			{
				Sched_Alg = FCFS;
			}
			else if( !strcmp( field, "SJN" ) )
			{
				Sched_Alg = SJN;
			}
			else if( !strcmp( field, "HPRN" ) )
			{
				Sched_Alg = HPRN;
			}
			else if( !strcmp( field, "ROUND_ROBIN" ) )
			{
				Sched_Alg = ROUND_ROBIN;
			}
			else
			{
				err_quit( " UNRECOGNIZABLE SCHED= PARAMETER \n" );
			}
		}
		else if( !strcmp( field, "MAXSEGMENTS=" ) )
		{
			if( ( num_items = fscanf( config_fp, "%d", &Max_Segments ) ) == EOF
				|| num_items < 1 )
			{
				err_quit( " MISSING MAXSEGMENTS= PARAMETER \n" );
			}
			if( Max_Segments < 1 )
			{
				err_quit( " INVALID MAXSEGMENTS= PARAMETER \n" );
			}
		}
		else if( !strcmp( field, "DEBUG_MEM=" ) )
		{
			if( ( num_items = fscanf( config_fp, "%s",field ) ) == EOF
				|| num_items < 1 )
			{
				err_quit( " MISSING DEBUG_MEM= PARAMETER \n" );
			}
			if( !strcmp( field, "ON" ) )
			{
				DEBUG_MEM = 1;
			}
			else if( !strcmp( field, "OFF" ) )
			{
				DEBUG_MEM = 0;
			}
			else
			{
				err_quit( " INVALID DEBUG_MEM= PARAMETER \n" );
			}
		}
		else if( !strcmp( field, "DEBUG_EVT=" ) )
		{
			if( ( num_items = fscanf( config_fp, "%s", field ) ) == EOF
				|| num_items < 1 )
			{
				err_quit( " MISSING DEBUG_EVT= PARAMETER \n" );
			}
			if( !strcmp( field, "ON" ) )       DEBUG_EVT = 1;
			else if( !strcmp( field, "OFF" ) ) DEBUG_EVT = 0;
			else
			{
				err_quit( " INVALID DEBUG_EVT= PARAMETER \n" );
			}
		}
		else if( !strcmp( field, "DEBUG_PCB=" ) )
		{
			if( ( num_items = fscanf( config_fp, "%s", field ) ) == EOF
				|| num_items < 1 )
			{
				err_quit( " MISSING DEBUG_PCB= PARAMETER \n" );
			}
			if( !strcmp( field, "ON" ) )       DEBUG_PCB = 1;
			else if( !strcmp( field, "OFF" ) ) DEBUG_PCB = 0;
			else
			{
				err_quit( " INVALID DEBUG_PCB= PARAMETER \n" );
			}
		}
		else if( !strcmp( field, "DEBUG_RBLIST=" ) )
		{
			if( ( num_items = fscanf( config_fp, "%s", field ) ) == EOF
				|| num_items < 1 )
			{
				err_quit( " MISSING DEBUG_RBLIST= PARAMETER \n" );
			}
			if( !strcmp( field, "ON" ) )       DEBUG_RBLIST = 1;
			else if( !strcmp( field, "OFF" ) ) DEBUG_RBLIST = 0;
			else
			{
				err_quit( " INVALID DEBUG_RBLIST= PARAMETER \n" );
			}
		}
		else if( !strcmp( field, "DEBUG_DEVQ=" ) )
		{
			if( ( num_items = fscanf( config_fp, "%s", field ) ) == EOF
				|| num_items < 1 )
			{
				err_quit( " MISSING DEBUG_DEVQ= PARAMETER \n" );
			}
			if( !strcmp( field, "ON" ) )       DEBUG_DEVQ = 1;
			else if( !strcmp( field, "OFF" ) ) DEBUG_DEVQ = 0;
			else
			{
				err_quit( " INVALID DEBUG_DEVQ= PARAMETER \n" );
			}
		}
		else if( !strcmp( field, "DEBUG_CPUQ=" ) )
		{
			if( ( num_items = fscanf( config_fp, "%s", field ) ) == EOF
				|| num_items < 1 )
			{
				err_quit( " MISSING DEBUG_CPUQ= PARAMETER \n" );
			}
			if( !strcmp( field, "ON" ) )       DEBUG_CPUQ = 1;
			else if( !strcmp( field, "OFF" ) ) DEBUG_CPUQ = 0;
			else
			{
				err_quit( " INVALID DEBUG_CPUQ= PARAMETER \n" );
			}
		}
		else // unrecognizable input
		{
			err_warn( "Unrecognizable parameter %s in CONFIG.DAT!\n", field );
			while( fgetc( config_fp ) != '\n')
			{ ; }
		}
	} // while

	// if last name was read from config file
	if( lname )
	{
		// save length of name entered
		name_length = strlen( Last_Name );
		
		// must have space to append .out extension
		if( name_length >= BUFSIZ - 5 )
		{
			err_quit( "Output file name '%s' is too long\n", Last_Name );
		}

		// verify that a bad file name wasn't given in the config file
		for( i = 0; i != name_length; i++ )
		{
			// don't let a user enter something like ../../some_file OR
			// ~some_user/important_stuff
			if( Last_Name[ i ] == '.' || Last_Name[ i ] == '~'
				|| Last_Name[ i ] == '/' || Last_Name[ i ] == '*' )
			{
				err_quit( "The character '%c' is not allowed in the output file"
					" name\n", Last_Name[ i ] );
			}
		}

		// create output file name--use last name read in config file,
		// concatentate with ".out", and convert to lowercase
		strncpy( Outfile, Last_Name, BUFSIZ );
		strncat( Outfile, ".out", BUFSIZ );
		strlwr( Outfile );
	}
	else
	{
		err_quit( " LNAME= PARAMETER NOT DEFINED! \n" );
	}

	// verify that necessary fields were read
	if( !objective )
	{
		err_quit( " OBJECTIVE= PARAMETER NOT DEFINED! \n" );
	}

	if( !one_over_beta && ( Sched_Alg == HPRN || Sched_Alg == SJN ) )
	{
		err_quit( " 1/BETA= PARAMETER NOT DEFINED! \n" );
	}

	if( !quantum && Sched_Alg == ROUND_ROBIN )
	{
		err_quit( " RRQUANTUM= PARAMETER NOT DEFINED! \n" );
	}

	if( Dev_Table == NULL )
	{
		err_quit( " DEVICES= PARAMETER NOT DEFINED! \n" );
	}

	// COMPUTE indirect configuration parameters
	CPU.CPU_burst = SEC / CPU.rate;  // instructions per sec

	fclose( config_fp );
}

/**
	Open files used by simulator for simulating programs.

	@retval None
 */
void
Open_Files( )
{
	if( ( Script_fp = fopen( SCRIPT_FILENAME, "r" ) ) == NULL )
	{
		err_quit( "Unable to open file '%s'\n", SCRIPT_FILENAME );
	}

	if( ( Prog_Files[EDITOR] = fopen( EDITOR_FILENAME, "r" ) ) == NULL )
	{
		err_warn( "EDITOR.DAT does not exist! \n" );
	}

	if( ( Prog_Files[PRINTER] = fopen( PRINTER_FILENAME, "r" ) ) == NULL )
	{
		err_warn( "PRINTER.DAT does not exist! \n" );
	}

	if( ( Prog_Files[COMPILER] = fopen( COMPILER_FILENAME, "r" ) ) == NULL )
	{
		err_warn( "COMPILER.DAT does not exist! \n" );
	}

	if( ( Prog_Files[LINKER] = fopen( LINKER_FILENAME, "r" ) ) == NULL )
	{
		err_warn( "LINKER.DAT does not exist! \n" );
	}

	if( ( Prog_Files[USER] =  fopen( USER_FILENAME, "r" ) ) == NULL )
	{
		err_warn( "USER.DAT does not exist! \n" );
	}

	if( ( Prog_Files[BOOT] =  fopen( BOOT_FILENAME, "r" ) ) == NULL )
	{
		err_warn( "BOOT.DAT does not exist! \n" );
	}
}

/**
	Allocate global tables used by simulator.

	@retval None
 */
void
Allocate_Tables( )
{
	int i = 0;

	// allocate memory map hardware
	Mem_Map  =  calloc( 2 * Max_Segments, sizeof( segment_type ) );
	if( Mem_Map == NULL )
	{
		err_quit( "Failed to allocate memory address map\n" );
	}

	// allocate memory
	Mem = calloc( Mem_Size, sizeof( instr_type ) );
	if( Mem == NULL )
	{
		err_quit( "Failed to allocate memory array\n" );
	}

	// initialize list of free memory segments
	Free_Mem = calloc( 1, sizeof( seg_list ) );
	if( Free_Mem == NULL )
	{
		err_quit( "Failed to allocate free memory list\n" );
	}
	Free_Mem->base  = 0;
	Free_Mem->size  = Mem_Size;
	Free_Mem->next  = NULL;
	Total_Free      = Mem_Size;

	// allocate terminal table
	Term_Table = calloc( Num_Terminals, sizeof( pcb_type* ) );
	if( Term_Table == NULL )
	{
		err_quit( "Failed to allocate terminal table\n" );
	}
	for( i = 0; i < Num_Terminals ; i++ )
	{
		Term_Table[ i ] = NULL;
	}
}

/**
	Open the simulator output file and write the values of all configuration
	parameters.

	@retval None
*/
void
Open_Output( char *out_name )
{
	long   clock;
	char*  date;
	char   buff[80];
	int    i;

	if( ( Out_fp = fopen( out_name, "w" ) ) == NULL )
	{
		err_quit( "Unable to open file '%s'\n", out_name );
	}

	time( &clock );  date = ctime( &clock );
	date[24] = date[25];  // trim '\n' character at end of date

	print_out( "\n +-----------------------------------------------------+" );
	print_out( "\n |  COP 4600                          Simulator Report |" );
	print_out( "\n |  DATE:  %-25s%20s", date, "|" );
	print_out( "\n |  NAME:  %-8s                                    |",
		Last_Name );
	print_out( "\n |                                                     |" );
	print_out( "\n |  OBJECTIVE # ...........................%-12d|",
			Objective );
	print_out( "\n |  LOGON TIME UNITS ......................" );
	if( Time_Unit == MIN ) strcpy( buff, "MIN " );
	if( Time_Unit == SEC ) strcpy( buff, "SEC " );
	if( Time_Unit == MSEC ) strcpy( buff, "MSEC" );
	if( Time_Unit == mSEC ) strcpy( buff, "mSEC" );
	if( Time_Unit == NSEC ) strcpy( buff, "NSEC" );
	print_out( "%-12s|", buff );
	print_out( "\n |  NO. OF TERMINALS ......................" );
	print_out( "%-12d|", Num_Terminals );
	print_out( "\n |  MEMORY SIZE ..........................." );
	print_out( "%-12d|", Mem_Size );
	print_out( "\n |  CPU RATE ( NSEC/INSTR ) ..............." );
	print_out( "%-12d|", CPU.rate );
	print_out( "\n |  SCHEDULING ALGORITHM .................." );
	if( Sched_Alg == FCFS )  strcpy( buff, "FCFS" );
	if( Sched_Alg == SJN  )  strcpy( buff, "SJN" );
	if( Sched_Alg == HPRN )  strcpy( buff, "HPRN" );
	if( Sched_Alg == ROUND_ROBIN ) strcpy( buff, "RND-ROBIN" );
	print_out( "%-12s|", buff );
	if( Sched_Alg == SJN || Sched_Alg == HPRN )
	{
		print_out( "\n |  EXPECTED CPU BURST ( 1/BETA ) ..........." );
		print_out( "%-12lu|", One_Over_Beta );
		print_out( "\n |  SMOOTHING FACTOR ( RHO ) ................" );
		print_out( "%-12.3f|", RHO );
	}
	if( Sched_Alg == ROUND_ROBIN )
	{
		print_out( "\n |  ROUND ROBIN QUANTUM ..................." );
		print_out( "%-12lu|", Timer.instr_slice );
	}
	print_out( "\n |  MAX SCRIPT LENGTH ....................." );
	print_out( "%-12d|", Max_Num_Scripts );
	print_out( "\n |                                                     |" );
	print_out( "\n |  DEBUG MEMORY .........................." );
	if( DEBUG_MEM ) strcpy( buff, "ON" );
	else            strcpy( buff, "OFF" );
	print_out( "%-12s|", buff );
	print_out( "\n |  DEBUG EVENT ..........................." );
	if( DEBUG_EVT ) strcpy( buff, "ON" );
	else            strcpy( buff, "OFF" );
	print_out( "%-12s|", buff );
	print_out( "\n |  DEBUG PCB ............................." );
	if( DEBUG_PCB ) strcpy( buff, "ON" );
	else           strcpy( buff, "OFF" );
	print_out( "%-12s|", buff );
	print_out( "\n |  DEBUG RBLIST .........................." );
	if( DEBUG_RBLIST ) strcpy( buff, "ON" );
	else              strcpy( buff, "OFF" );
	print_out( "%-12s|", buff );
	print_out( "\n |  DEBUG DEVQ ............................" );
	if( DEBUG_DEVQ )  strcpy( buff, "ON" );
	else            strcpy( buff, "OFF" );
	print_out( "%-12s|", buff );
	print_out( "\n |  DEBUG CPUQ ............................" );
	if( DEBUG_CPUQ ) strcpy( buff, "ON" );
	else            strcpy( buff, "OFF" );
	print_out( "%-12s|", buff );
	print_out( "\n |                                                     |" );
	print_out( "\n | =================  DEVICE TABLE =================== |" );
	print_out( "\n |                                                     |" );
	print_out( "\n |  ID      RATE (*)                                   |" );
	print_out( "\n |                                                     |" );

	// print device stuff
	for( i = 0; i < Num_Devices; i++ )
	{
		print_out( "\n |  %-4s    %-12lu                               |",
			Dev_Table[i].name,
			(unsigned long)
			( (double) 1.0e9 / (double) Dev_Table[i].nano_per_byte ) );
	}

	print_out( "\n |                                                     |" );
	print_out( "\n | (*) Bytes/Second                                    |" );
	print_out( "\n +-----------------------------------------------------+\n\n" );

	Print_Event_Header();
}

/**
	Print event list header to output file.

	@retval None
 */
void
Print_Event_Header( )
{
	print_out(
		" EVENT  AGENT  HR:xxxxxxxx MN:xx SC:xx MS:xxx mS:xxx NS:xxx\n" );
	print_out(
		" -----  -----  ----------- ----- ----- ------ ------ ------\n" );
}

/**
	Perform processing at the end of simulation.
	It computes all statistics for the CPU and each device and terminal.
	It then calls a function to write the statistics to DATA*.OUT.
	Finally, it closes all files.

	@retval None
*/
void
Clean_Up( )
{
	int data_file;

	// Compute and print statistics to Out_fp
	if( Objective != 1 )
	{
		Calc_Stats();
	}
	Write_Stats();

	print_out( "\n\nEnd of Benchmark Report.\n\n" );

	// close all files
	fclose( Out_fp );
	fclose( Script_fp );
	for( data_file = EDITOR; data_file < LOGOFF; data_file++ )
	{
		if ( Prog_Files[ data_file ] != NULL )
		{
			fclose( Prog_Files[ data_file ]);
		}
	}
}

/**
	Write statistics summary to output file.

	@retval None
 */
void
Write_Stats( )
{
	int i;
	int flag;

	/*
		write terminal statistics
	 */
	print_out( "\n\n\n                              TERMINAL  SUMMARY                             " );
	print_out( "\n ============================================================================" );
	print_out( "\n TERMNL|      EXEC      |      WAIT    |    BLOCKED   |    ELAPSED   |  %%EFF" );
	print_out( "\n ======|================|==============|==============|==============|=======" );

	// print statistics for each terminal
	for( i = 0, flag = 0; i < Num_Terminals ; i++ )
	{
		if( Term_Table[ i ] == NULL )
		{
			continue;
		}

		if( flag )
		{
			print_out(  "\n ------|----------------|--------------|--------------|--------------|-------" );
		}

		print_out( "\n %s  |  %10lu SC |%10lu SC |%10lu SC |%10lu SC | %.2f",
			Term_Table[i]->username,
			Term_Table[i]->total_run_time.seconds,
			Term_Table[i]->total_ready_time.seconds,
			Term_Table[i]->total_block_time.seconds,
			Term_Table[i]->total_logon_time.seconds,
			Term_Table[i]->efficiency );

		print_out( "\n       |  %10lu NS |%10lu NS |%10lu NS |%10lu NS |",
			Term_Table[i]->total_run_time.nanosec,
			Term_Table[i]->total_ready_time.nanosec,
			Term_Table[i]->total_block_time.nanosec,
			Term_Table[i]->total_logon_time.nanosec );

		flag = 1;
	}

	print_out(  "\n ============================================================================" );

	print_out(  "\n TOTALS|  %10lu SC |%10lu SC |%10lu SC |%10lu SC |",
		Tot_Run.seconds,
		Tot_Wait.seconds,
		Tot_Block.seconds,
		Tot_Logon.seconds );

	print_out( "\n       |  %10lu NS |%10lu NS |%10lu NS |%10lu NS |",
		Tot_Run.nanosec,
		Tot_Wait.nanosec,
		Tot_Block.nanosec,
		Tot_Logon.nanosec );

	print_out( "\n ------|----------------|--------------|--------------|--------------|" );

	print_out(  "\n AVERGE|  %10lu SC |%10lu SC |%10lu SC |%10lu SC |",
		Avg_Run.seconds,
		Avg_Wait.seconds,
		Avg_Block.seconds,
		Avg_Logon.seconds );

	print_out( "\n       |  %10lu NS |%10lu NS |%10lu NS |%10lu NS |",
		Avg_Run.nanosec,
		Avg_Wait.nanosec,
		Avg_Block.nanosec,
		Avg_Logon.nanosec );

	print_out(  "\n =====================================================================" );

	/*
		write device statistics
	 */
	print_out( "\n\n\n                                DEVICE  SUMMARY                            " );
	print_out(   "\n ============================================================================" );
	print_out(   "\n DEVICE|        BUSY    |      WAIT    |      IDLE "
			"   |   RESPONSE   | %%UTIL" );
	print_out(   "\n ======|================|==============|==============|==============|=======" );

	// print statistics for CPU
	print_out(   "\n CPU   |  %10lu SC |%10lu SC |%10lu SC |%10lu SC | %.2f",
		CPU.total_busy_time.seconds,
		CPU.total_q_time.seconds,
		CPU.total_idle_time.seconds,
		CPU.response.seconds,
		CPU.utilize );
	print_out( "\n       |  %10lu NS |%10lu NS |%10lu NS |%10lu NS |",
		CPU.total_busy_time.nanosec,
		CPU.total_q_time.nanosec,
		CPU.total_idle_time.nanosec,
		CPU.response.nanosec );

	// print statistics for each device
	for( i = 0; i < Num_Devices; i++ )
	{
		if( Dev_Table[i].num_served == 0 )
		{
			continue;
		}

		print_out( "\n ------|----------------|--------------|--------------|--------------|-------" );

		print_out( "\n %s  |  %10lu SC |%10lu SC |%10lu SC |%10lu SC | %.2f",
			Dev_Table[i].name,
			Dev_Table[i].total_busy_time.seconds,
			Dev_Table[i].total_q_time.seconds,
			Dev_Table[i].total_idle_time.seconds,
			Dev_Table[i].response.seconds,
			Dev_Table[i].utilize );

		print_out( "\n       |  %10lu NS |%10lu NS |%10lu NS |%10lu NS |",
			Dev_Table[i].total_busy_time.nanosec,
			Dev_Table[i].total_q_time.nanosec,
			Dev_Table[i].total_idle_time.nanosec,
			Dev_Table[i].response.nanosec );
	}
	print_out(  "\n ============================================================================" );
}

/*
	Auxiliary functions for time_type manipulation, error-handling, and string
	parsing
 */

/**
	Add two simulation times: time_2 + time_1 (save the result in time_2).

	@param[in] time_1 -- time to use in addition
	@param[in,out] time_2 -- time to use in addition
	@retval None
 */
void
Add_time( struct time_type* time_1, struct time_type* time_2 )
{
	if( time_2->nanosec < (SEC - time_1->nanosec) )
	{
		time_2->nanosec += time_1->nanosec;
	}
	else
	{
		time_2->seconds++;
		time_2->nanosec -= (SEC - time_1->nanosec);
	}

	time_2->seconds += time_1->seconds;
}

/**
	Subract two simulation times: time_2 - time_1 (save the result in time_2).

	<b> Important Notes:</b>
	\li It is assumed that time_2 >= time_1.

	@param[in] time_1 -- time to use in difference
	@param[in,out] time_2 -- time to use in difference
	@retval None
 */
void
Diff_time( struct time_type* time_1, struct time_type* time_2 )
{
	if( time_2->seconds == time_1->seconds )
	{
		time_2->seconds  = 0;
		time_2->nanosec -= time_1->nanosec;
	}
	else
	{
		time_2->seconds -= time_1->seconds + 1;
		if( time_2->nanosec >= time_1->nanosec )
		{
			++time_2->seconds;
			time_2->nanosec -= time_1->nanosec;
		}
		else
		{
			time_2->nanosec += SEC - time_1->nanosec;
		}
	}
}

/**
	Divide two simulation times: time_1 / time_2).

	@param[in] time_1 -- time to use in division
	@param[in] time_2 -- time to use in division
	@retval fraction -- ratio of two times
 */
double
Divide_time( struct time_type* time_1, struct time_type* time_2 )
{
	double sec_1, sec_2;

	// both times are first converted to seconds before dividing.
	sec_1 = (double) time_1->seconds + (double) time_1->nanosec / DSEC;
	sec_2 = (double) time_2->seconds + (double) time_2->nanosec / DSEC;

	return( sec_1 / sec_2 );
}

/**
	Compare two simulation times: time_1 < time_2.

	@param[in] time_1 -- time to use in comparision
	@param[in] time_2 -- time to use in comparision
	@retval Either -1, 0, or +1 depending on if time_1 is respectively less
	than, equal to, or greater than time_2.
 */
int
Compare_time( struct time_type *time_1, struct time_type *time_2 )
{
	int result;

	// compare seconds
	result = ( time_1->seconds < time_2->seconds ) ? -1 : 1;

	// if seconds are equal, then compare nano-seconds
	if( time_1->seconds == time_2->seconds )
	{
		result = ( time_1->nanosec < time_2->nanosec ) ? -1 : 1;

		// if nanoseconds are also equal, then time_1 and time_2 are equal
		if( time_1->nanosec == time_2->nanosec )
		{
			result = 0;
		}
	}

	return( result );
}

/**
	Compute average simulation time: time_1 / n (save result in time_2).

	@param[in] time_1 -- time to use in average
	@param[in] n -- divisor to use in average
	@param[out] time_2 -- average time
	@retval None
 */
void
Average_time( struct time_type* time_1, unsigned n, struct time_type* time_2 )
{
	double x, dn;

	dn = (double) n;
	x = ((double) time_1->seconds / dn)
		+ ((double) time_1->nanosec / dn) / DSEC;

	time_2->seconds = x;
	time_2->nanosec = (x - (double) time_2->seconds) * DSEC;
}

/**
	Convert CPU burst count into a simulation time.

	The time depends on the CPU rate and length of the burst.

	@param[in] burst_count -- CPU burst count
	@param[out] sim_time -- average time
	@retval None
*/
void
Burst_time( unsigned long burst_count, struct time_type* sim_time )
{
	sim_time->seconds = burst_count / CPU.CPU_burst;
	sim_time->nanosec =
		(burst_count - CPU.CPU_burst * sim_time->seconds) * CPU.rate;
}

/**
	Convert external time units (defined by Time_Unit) to internal simulation
	time: seconds and nanosec.

	@param[in] ul_time -- time represented as an integral unit determined by
	Time_Unit
	@param[out] sim_time -- simulation time converted from ul_time
	@retval None
 */
void
Uint_to_time( unsigned long ul_time, struct time_type* sim_time )
{
	switch( Time_Unit )
	{
		case MIN:
			sim_time->seconds = ul_time * 60;
			sim_time->nanosec = 0;
			break;
		case SEC:
			sim_time->seconds = ul_time;
			sim_time->nanosec = 0;
			break;
		case MSEC:
			sim_time->seconds = ul_time / 1000;
			sim_time->nanosec = (ul_time - 1000 * sim_time->seconds) * MSEC;
			break;
		case mSEC:
			sim_time->seconds = ul_time / 1000000;
			sim_time->nanosec = (ul_time - 1000000 * sim_time->seconds) * mSEC;
			break;
		case NSEC:
			sim_time->seconds = ul_time / 1000000000;
			sim_time->nanosec = ul_time - 1000000000 * sim_time->seconds;
			break;
		default:
			break;
	}
}

/**
	Convert simulation time to double.

	@param[in] sim_time -- simulation time
	@retval convert_time -- converted time
 */
double
Time_to_double( struct time_type* sim_time )
{
	double  nano, convert_time;
	nano = sim_time->nanosec / (double) SEC;
	convert_time = sim_time->seconds + nano;

	return( convert_time );
}

/**
   Print an error message and exit program.

   Error message can have conversion specifications (i.e., %d, %s, etc.) since
   err_quit() takes a variable number of arguments.

	@param[in] message -- error message to display
 */
void err_quit( char* message, ... )
{
   va_list v_args;

   va_start( v_args, message );
	fprintf( stderr, "Error: " );
   vfprintf( stderr, message, v_args );
   va_end( v_args );

   exit( EXIT_FAILURE );
}

/**
   Print a warning message and continue running program.

   Error message can have conversion specifications (i.e., %d, %s, etc.) since
   err_quit() takes a variable number of arguments.

	@param[in] message -- error message to display
 */
void err_warn( char* message, ... )
{
   va_list v_args;

   va_start( v_args, message );
	fprintf( stderr, "Warning: " );
   vfprintf( stderr, message, v_args );
   va_end( v_args );
}

/**
   Print an output message to the simulation output file.

   An output message can have conversion specifications (i.e., %d, %s, etc.)
	since print_out() takes a variable number of arguments.

	@param[in] message -- message to display to output file
 */
void print_out( char* message, ... )
{
   va_list v_args;

	assert( Out_fp != NULL );

   va_start( v_args, message );
   vfprintf( Out_fp, message, v_args );
   va_end( v_args );
}

/**
	Convert string to lower case.

	@param[out] string -- string to convert to lower case
 */
char* strlwr( char* string )
{
	char* copy = string;
	while( *copy != '\0' )
	{
		*copy = tolower( *copy );
		copy++;
	}
	return( string );
}

/**
	Convert string to upper case.

	@param[out] string -- string to convert to lower case
 */
char* strupr( char* string )
{
	char* copy = string;
	while( *copy != '\0' )
	{
		*copy = toupper( *copy );
		copy++;
	}
	return( string );
}

/**
	Remove whitespace from beginning and end of string.

	<b> Important Notes:</b>
	\li The given string must be non-NULL and must not point to a literal, e.g.,
	it cannot be declared as char word[ 100 ] = " word " or as 
	char* word = " word ".

	@param[out] string -- string to remove white-space from
 */
char* trim( char* string )
{
	char*  search = NULL;  // string to find non-whitespace and copy from
	char*  copy   = NULL;  // string to copy to

	// find first non-whitespace character
	for( search = string;
		*search != '\0' && isspace( (int) *search );
		search++ )
	{ ; }

	// copy all non-whitespace characters into copy string
	for( copy = string;
		*search != '\0' && !isspace( (int) *search );
		search++, copy++ )
	{
		*copy = *search;
	}
	*copy = '\0';  // null-terminate string

	return( string );
}
