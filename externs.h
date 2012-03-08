/**
	externs.h


	Simulator Project for COP 4600

	Declarations for external data and functions used by the simulator.

		All global variables are defined and initialized in simulator.c. Function
		definitions are given in one of the following files: simulator.c, obj1.c,
		obj2.c, obj3.c, obj4.c, obj5.c, obj6.c.

	Revision List:
		Original Design:  Dr. David Workman, 1990
		Revised by Tim Hughey and Mark Stephens, 1993
		Revised by Wade Spires, Spring 2005
		Minor Revisions by Sean Szumlanski, Spring 2010
*/

#ifndef __EXTERNS_H
#define __EXTERNS_H

/*
	Global variables
 */
extern int   Objective;          ///< Current objective number
extern char  Last_Name[BUFSIZ];  ///< Last name of user for making output file
extern char  Outfile[BUFSIZ];    ///< Name of output file

// File information
extern FILE*  Out_fp;        ///< Output file pointer
extern FILE*  Script_fp;     ///< Script file pointer
extern FILE*  Prog_Files[];  ///< Table of program file pointers

// Control information
extern control_switch_type  CPU_SW;    ///< Control switch for the CPU
extern control_switch_type  SCHED_SW;  ///< Control switch for the scheduler

extern int            Agent;              ///< Current 
extern event_type     Event;              ///< Current event
extern char*          Op_Names[];         ///< Table of operator names
extern char*          Event_Names[];      ///< Table of event names
extern char*          Prog_Names[];       ///< Program file pointers
extern char*          Prog_File_Names[];  ///< Name of program files
extern int            Num_Devices;        ///< Number of devices 
extern int            Num_Terminals;      ///< Number of user terminals

/** Process scheduling algorithm used by OS. */
extern sched_type     Sched_Alg;

extern int            Max_Jobs;         ///< Total jobs processed
extern int            Max_Num_Scripts;  ///< Maximum number of programs
extern int            Max_Segments;     ///< Max number program segments
extern          int   Mem_Size;         ///< Memory size
extern unsigned int   One_Over_Beta;    ///< Expected service time
extern unsigned long  Time_Unit;        ///< Integral time unit for events
extern double         Rho;              ///< SJN Smoothing factor
extern time_type      Clock;            ///< System Hardware Clock
extern timer_type     Timer;            ///< System interval timer

extern cpu_type       CPU;         ///< Central Processor
extern state_type     Old_State;   ///< Old State of Interrupt
extern state_type     New_State;   ///< New State of Interrupt
extern instr_type*    Mem;         ///< Memory Storage Array
extern addr_type      MAR;         ///< Memory Address Register
extern segment_type*  Mem_Map;     ///< Memory Address Map Array
extern device_type*   Dev_Table;   ///< Device table
extern pcb_type**     Term_Table;  ///< Terminal table
extern event_list*    Event_List;  ///< Pointer to head of event list
extern seg_list*      Free_Mem;    ///< List of free memory segments
extern unsigned int   Total_Free;  ///< Total free memory available

extern time_type      Tot_Logon;  ///< Total job processing time
extern time_type      Tot_Wait;   ///< Total job wait       time
extern time_type      Tot_Block;  ///< Total job blocked    time
extern time_type      Tot_Run;    ///< Total job execution  time
extern time_type      Avg_Run;    ///< Average user execution time
extern time_type      Avg_Wait;   ///< Average user wait      time
extern time_type      Avg_Block;  ///< Average user blocked   time
extern time_type      Avg_Logon;  ///< Average user logon     time

/* Debugging flags */
extern int  DEBUG_EVT;     ///< Flag controlling Event_List debug output
extern int  DEBUG_MEM;     ///< Flag controlling Mem debug output
extern int  DEBUG_PCB;     ///< Flag controlling Term_Table debug output
extern int  DEBUG_RBLIST;  ///< Flag controlling pcb.rb_q debug output
extern int  DEBUG_DEVQ;    ///< Flag controlling device.wait_q debug output
extern int  DEBUG_CPUQ;    ///< Flag controlling CPU.ready_q debug output

/* Main simulator functions */
extern int     Interrupt_Handler( void );
extern void    Init( void );
extern void    Process_Config_File( void );
extern void    Open_Files( void );
extern void    Allocate_Tables( void );
extern void    Open_Output( char* );
extern void    Print_Event_Header( void );
extern void    Clean_Up( void );
extern void    Write_Stats( void );

/* Auxiliary functions */
extern void    Add_time( struct time_type*, struct time_type* );
extern void    Diff_time( struct time_type*, struct time_type* );
extern double  Divide_time( struct time_type*, struct time_type* );
extern int     Compare_time( struct time_type*, struct time_type* );
extern void    Average_time( struct time_type*, unsigned, struct time_type* );
extern void    Burst_time( unsigned long, struct time_type* );
extern void    Uint_to_time( unsigned long, struct time_type* );
extern double  Time_to_double( struct time_type* );
extern void    err_quit( char* , ... );
extern void    err_warn( char* , ... );
extern void    print_out( char*, ... );
extern char*   strlwr( char* );
extern char*   strupr( char* );
extern char*   trim( char* );


/* Debugging functions--not implemented yet */
extern void  Dump_evt( );
extern void  Dump_mem( segment_type* );
extern void  Dump_pcb( );
extern void  Dump_rblist( );
extern void  Dump_devq( );
extern void  Dump_cpuq( );

/* Objective 1 functions */
extern void  Add_Event( int, int, struct time_type* );
extern void  Load_Events( void );
extern void  Write_Event( int, int, struct time_type* );
extern void  Interrupt( void );

/* Objective 2 functions */
extern void  Cpu( void );
extern int   Memory_Unit( void );
extern void  Exec_Program( struct state_type* );
extern int   Fetch( struct instr_type* );
extern int   Read ( struct instr_type* );
extern int   Write( struct instr_type* );
extern void  Boot( void );
extern void  Get_Instr( int current_script, struct instr_type* );
extern void  Display_pgm( segment_type*, int, pcb_type* );
extern void  Set_MAR( struct addr_type* );

/* Objective 3 functions */
extern void  Logon_Service( void );
extern void  Get_Script( pcb_type* );
extern int   Next_pgm( pcb_type* ); 
extern void  Get_Memory( pcb_type* );   
extern int   Alloc_seg( int );
extern void  Loader( pcb_type* );
extern void  Dealloc_pgm( pcb_type* );
extern void  Dealloc_seg( int, int );
extern void  Merge_seg( seg_list*, seg_list*, seg_list* );
extern void  End_Service( void );
extern void  Abend_Service( void );

/* Objective 4 functions */
extern void       Add_cpuq( pcb_type* );
extern void       Add_devq( int, rb_type* );
extern void       Add_rblist( pcb_type*, rb_type* );
extern pcb_type*  Scheduler( void );
extern void       Sio_Service( void );
extern rb_type*   Alloc_rb( void );
extern void       Start_IO( int );
extern void       Wio_Service( void );
extern rb_type*   Find_rb( pcb_type*, struct addr_type* );
extern void       Delete_rb( rb_type*, pcb_type* );
extern void       Eio_Service( void );
extern void       Purge_rb( pcb_type* );
extern void       Load_Map( segment_type*, int );
extern void       Dispatcher( void );

/* Objective 5 functions */
extern void  Calc_Stats( void );

/* Objective 6 functions */
extern void  Compact_mem( void );
extern void  Timer_Service( void );

#endif // __EXTERNS_H
