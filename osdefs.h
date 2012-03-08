/**
	osdefs.h


	Simulator Project for COP 4600

	Operating system type definitions.

		#defines, enums, and struct definitions are provided in this order.

	Revision List:
		Original Design:  Dr. David Workman, 1990
		Revised by Tim Hughey and Mark Stephens, 1993
		Revised by Wade Spires, Spring 2005
		Minor Revisions by Sean Szumlanski, Spring 2010
*/

#ifndef __OSDEFS_H
#define __OSDEFS_H

//#define NDEBUG  // comment this line to enable assert() debugging calls
// Note: '#define NDEBUG' must precede '#include <assert.h>'
#include <assert.h>

#ifdef  SEC
#undef  SEC // SEC may be already defined in time.h
#endif 

#define   MIN         60            // seconds per minute
#define   SEC         1000000000    // nanosec per second
#define   MSEC        1000000       // nanosec per millisec
#define   mSEC        1000          // nanosec per microsec
#define   NSEC        1
#define   DSEC        1.0e9

/** Number of characters allowed in a user name. */
#define   USER_NAME_LENGTH  4

/* Number of characters allowed in a device name. */
#define   DEV_NAME_LENGTH   4

/** Names of startup files. */
#define CONFIG_FILENAME     "config.dat"
#define LOGON_FILENAME      "logon.dat"
#define SCRIPT_FILENAME     "script.dat"
#define EDITOR_FILENAME     "editor.dat"
#define PRINTER_FILENAME    "printer.dat"
#define COMPILER_FILENAME   "compiler.dat"
#define LINKER_FILENAME     "linker.dat"
#define USER_FILENAME       "user.dat"
#define BOOT_FILENAME       "boot.dat"

/** Maximum length an event name can be. */
#define EVENT_NAME_LENGTH_MAX  8

/** Default size of strings to use when reading from files. */
#ifndef BUFSIZ
#define BUFSIZ 1024
#endif

/**
	Type of programs that can be executed.
	Provides numerical codes for indexing into the arrays Prog_Files,
	Prog_Names, and Prog_File_Names. To facilitate array traversal, the first
	code is guaranteed to be 0, and the last code is guaranteed to be labeled
	NUM_PROGRAMS.
 */
typedef enum { EDITOR = 0, PRINTER, COMPILER, LINKER, USER, BOOT, LOGOFF,
	NUM_PROGRAMS } prog_type;

/**
	Type of OS events that can occur.
	Provides numerical codes for indexing into the array event_tab. Fields
	respectively refer to the following events: User Logon, Start I/O, Wait I/O,
	End I/O, End Program, Timer Interrupt, Segmentation Fault, and Address
	Fault.  To facilitate array traversal, the first code is guaranteed to be 0,
	and the last code is guaranteed to be labeled NUM_EVENTS.
 */
typedef enum { LOGON_EVT = 0, SIO_EVT, WIO_EVT, EIO_EVT, END_EVT, TIMER_EVT,
	SEGFAULT_EVT, ADRFAULT_EVT, NUM_EVENTS } event_type;

/**
	Type of instruction operation codes that can occur in a program.
	Provides numerical codes for indexing into the array opcode_tab.
	To facilitate array traversal, the first code is guaranteed to be 0, and the
	last code is guaranteed to be labeled NUM_OPCODES.

	Note: Device "op codes" should start after these instruction codes; that
	is, the first device begins at NUM_OPCODES.
 */
typedef enum { SIO_OP = 0, WIO_OP, REQ_OP, JUMP_OP, SKIP_OP, END_OP,
	NUM_OPCODES } opcode_type;

/** Type of CPU scheduling algorithm being used. */
typedef enum { FCFS, SJN, HPRN, ROUND_ROBIN } sched_type;

/**
	@brief Type to represent simulation time.
 */
struct time_type
{
	/** Number of seconds in simulation time. */
	unsigned long  seconds;

	/** Number of nanoseconds in simulation time. */
	unsigned long  nanosec;
};
typedef struct time_type time_type;

/**
	@brief Logical program addresses in JUMP and REQ instructions are of
		addr_type.
 */
struct addr_type
{
	/** Segment number. */
	int           segment;

	/** Address offset.  */
	unsigned int  offset;
};
typedef struct addr_type addr_type;

/**
	@brief Operand of an instruction.

	Note that the type is union, so an operand_type variable can hold one, and
	only one, value despite the multiple fields listed; otherwise, indeterminant
	behavior could result.
 */
union operand_type
{
	/** Address for REQ and JUMP instructions. */
	struct addr_type  address;

	/** CPU cycles for SIO, WIO, and END instructions. */
	unsigned long     burst;

	/** Byte transfer count for devices. */
	unsigned long     bytes;

	/** Skip count for SKIP instruction. */
	unsigned int      count;
};
typedef union operand_type operand_type;

/**
	@brief Program instruction.

	Note: Memory is an array of instr_type.
 */
struct instr_type
{
	/** Type of instruction (operation code). */
	opcode_type          opcode;

	/** Operand of instruction. */
	union operand_type  operand;
};
typedef struct instr_type instr_type;

/**
	@brief Memory segment information.
 */
struct segment_type
{
	/** Segment access bits. */
	unsigned char  access;

	/** Size of segment. */
	unsigned int   size;

	/** Position in memory that segment begins at (-1 marks invalid). */
	int   base;
};
typedef struct segment_type segment_type;

/**
	@brief Current state of CPU or pcb.
 */
struct state_type
{
	/** CPU running mode: 0 for user, 1 for kernel (privileged) */
	unsigned char     mode;

	/** Program counter (segment and offset pair) */
	struct addr_type  pc;
};
typedef struct state_type state_type;

/**
	@brief Process control block.
 */
struct pcb_type
{
	/** Status of PCB. */
	enum pcb_status_type { NEW_PCB, READY_PCB, ACTIVE_PCB, BLOCKED_PCB,
		DONE_PCB, TERMINATED_PCB } status;

	/**
		Various flags: bit 0 = set timer flag, bit 1 = new instr flag,
		bit 2 = new burst flag, bits 3-7 = not used.
	*/
	char               flags;

	/** Name of user (e.g., "U001"). */
	char               username[ USER_NAME_LENGTH + 1 ];

	/** Index in Term_Table. */
	unsigned int       term_pos;

	/** Array of program user will execute. */
	prog_type*         script;

	/** Index into array script. */
	int                current_prog;

	/** Segment table for current process. */
	segment_type*      seg_table;

	/** Number of segments in segment table. */
	unsigned int       num_segments;

	/**
		Area for saving the process's CPU information (i.e., segment and offset
		of next instruction) during an interrupt.
	 */
	struct state_type  cpu_save;

	/** Logon time. */
	struct time_type   logon_time;

	/** Last time process was blocked. */
	struct time_type   block_time;

	/** Last time process was ready. */
	struct time_type   ready_time;

	/** Last time process was active. */
	struct time_type   run_time;

	/** Total time process was blocked. */
	struct time_type   total_block_time;

	/** Total time process was ready. */
	struct time_type   total_ready_time;

	/** Total time process was active. */
	struct time_type   total_run_time;

	/** Total time process was logged on. */
	struct time_type   total_logon_time;

	/** Number of times process was allocated to CPU. */
	int		          served;

	/** (Total time blocked + total time active) / total time logged on. */
	double             efficiency;

	/** I/O request block that the process is waiting on. */
	struct rb_type*    wait_rb;

	/** Head of queue containing I/O request blocks (circularly linked). */
	struct rb_list*    rb_q;

	/** Total time user has been serviced by all devices. */
	struct time_type   total_busy_time;

	/** Length of last CPU burst. */
	unsigned long      sjnburst;

	/** Average of last n CPU bursts. */
	double             sjnave;

	/** Instructions remaining (or round-robin scheduling). */
	unsigned long      instrleft;

	/** Time slice remaining (or round-robin scheduling). */
	unsigned long      sliceleft;
};
typedef struct pcb_type pcb_type;

/**
	@brief Type representing a device, such as a printer or disk drive.

	The global variable Dev_Table is an array of this type.
 */
struct device_type
{
	/** Name of device. */
	char               name[ DEV_NAME_LENGTH + 1 ];

	/** Currently active IORB. Device status is indicated by this field. */
	struct rb_type*    current_rb;

	/** Head of wait queue containing I/O request blocks (circularly linked). */
	struct rb_list*    wait_q;

	/** Device rate bytes per second. */
	unsigned long int  bytes_per_sec;

	/** Device rate in nanoseconds per byte (10^9 / bytes_per_sec). */
	unsigned long int  nano_per_byte;

	/** Total busy time. */
	struct time_type   total_busy_time;

	/** Total queue wait time. */
	struct time_type   total_q_time;

	/** Average response time. */
	struct time_type   response;

	/** Total idle time. */
	struct time_type   total_idle_time;

	/** Number of IORBs serviced. */
	int                num_served;

	/** Utilization. */
	double             utilize;

	/** Time that I/O started on device for current IORB. */
	struct time_type   start;
};
typedef struct device_type device_type;

/**
	@brief Type representing Central Processing Unit (CPU).
 */
struct cpu_type
{
	/** Currently active process. */
	pcb_type*           active_pcb;

	/** Head of PCB ready queue (circularly linked). */
	struct pcb_list*    ready_q;

	/** Current state of CPU: access mode and program counter. */
	struct state_type   state;

	/** CPU rate in nanoseconds per instruction. */
	unsigned long       rate;

	/** CPU rate in instructions per second. */
	unsigned long       CPU_burst;

	/** Total busy time. */
	struct time_type    total_busy_time;

	/** Total queue wait time. */
	struct time_type    total_q_time;

	/** Average response time. */
	struct time_type    response;

	/** Total idle time. */
	struct time_type    total_idle_time;

	/** Number of (possibly non-unique) processes served. */
	int                 num_served;

	/** Utilization. */
	double              utilize;
};
typedef struct cpu_type cpu_type;

/**
	@brief Type representing an I/O request block (IORB).
 */
struct rb_type
{
	/** Status of I/O request block. */
	enum rb_status_type { ACTIVE_RB, PENDING_RB, DONE_RB }  status;

	/** Process requesting I/O. */
	pcb_type*         pcb;

	/** Requested device (index into Dev_Table). */
	int               dev_id;

	/** Time that the IORB enters the device queue. */
	struct time_type  q_time;

	/** Number of bytes to transfer to/from device. */
	unsigned long     bytes;

	/** Logical address of program instruction that generated the request. */
	struct addr_type  req_id;
};
typedef struct rb_type rb_type;

/**
	@brief Timer device for round-robin scheduling.
 */
struct timer_type
{
	/** Time of next interrupt. */
	struct time_type  time_out;

	/** Round-robin quantum in number of instructions. */
	unsigned long     instr_slice;

	/** Round-robin quantum in number of seconds and nanoseconds (time_type) */
	struct time_type  time_slice;
};
typedef struct timer_type timer_type;

/**
	@brief Node in the doubly-linked list, event_list, that represents OS
		events.  

	We have a typedef at the end of the definition as opposed to the following:
		typedef struct {
			node* next;
		} node;
 	The problem is that on a single-pass compiler, such as gcc, the name
	'struct node' is not defined for the declaration 'node* next'.
 */
struct event_list
{
	/** Type of event. */
	event_type          event;

	/** User or device causing the event. */
	int                 agent;

	/** Time event occurs. */
	struct time_type    time;

	/** Next event node in list. */
	struct event_list*  prev;

	/** Previous event node in list. */
	struct event_list*  next;
};
typedef struct event_list event_list;

/**
 */
struct rb_list
{
	/** I/O request block stored in node. */
	rb_type*  rb;

	/** Next node in list. */
	struct rb_list*  next;

	/** Previous node in list. */
	struct rb_list*  prev;
};
typedef struct rb_list rb_list;

/**
	@brief Node in list of process control blocks.

	Used in lists such as the CPU ready queue.
 */
struct pcb_list
{
	/** PCB stored in node. */
	pcb_type*         pcb;

	/** Next node in list. */
	struct pcb_list*  next;

	/** Previous node in list. */
	struct pcb_list*  prev;
};
typedef struct pcb_list pcb_list;

/**
	@brief Node in list of free memory segments.
 */
struct seg_list
{
	/** Position segment starts at in memory (Mem). */
	unsigned int  base;

	/** Size of free block. */
	unsigned int  size;

	/** Next free segment node in list. */
	struct seg_list*  next;  // next segment node in list
};
typedef struct seg_list seg_list;

/** Control switch for various aspects of OS, such as scheduling. */
typedef enum { ON = 0, OFF = 1 } control_switch_type;

#endif // __OSDEFS_H
