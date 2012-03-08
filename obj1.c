/**
	obj1.c


	Simulator Project for COP 4600

	Objective 1 project that implements loading the event list for
	the OS simulator.

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


// Define constants to be used
#define FALSE 0
#define TRUE  1
//END DEFINITIONS 

int  get_agent_id( char* );
int  get_event_id( char* );
void to_uppercase(char* string);
/**
	Initialize the event list from the file logon.dat. This file normally
	contains only logon events for all terminals. However, for debugging
	purposes, logon.dat can contain events of any type.

	<b> Algorithm: </b>
	\verbatim
	Open logon file given by constant LOGON_FILENAME
	While not end of file
		Read event name, agent name, and time from each line in file
		Convert event name to event ID--call get_event_id() (must be written)
		Convert agent name to agent ID--call get_agent_id() (must be written)
		Convert time to simulation time--call Uint_to_time()
		Add event to event list using event ID, agent ID, and simulation time
	Close file
	\endverbatim

	@retval None
 */
void
Load_Events( )
{
   int eid = 0,aid = 0,test = 0;
   char eventname[EVENT_NAME_LENGTH_MAX];
   char agentname[DEV_NAME_LENGTH];
   unsigned long time;
   struct event_list* current;
   struct event_list* temp;
  struct time_type* timmy = malloc(sizeof(struct time_type));
   FILE* input;
  Event_List = NULL; 
    input = fopen(LOGON_FILENAME,"r");
 
	 if(input == NULL)
	   return;
	   	   
	  
	   while(!feof(input))
	   {
                  fscanf(input,"%s", eventname);
		  fscanf(input,"%s", agentname);
		  fscanf(input,"%u", &time);
		  
		  eid = get_event_id(eventname);
         	  aid = get_agent_id(agentname);
         	  Uint_to_time(time,timmy);
		  Add_Event(eid,aid,timmy);
	   }
  
              fclose(input);
         /* current = Event_List;
           while(current != NULL && test != 3)
            {
              current = current->next;
              test++;
            }
         if(current != NULL)
           {
            temp = current;
            temp->prev->next = temp->next;
            temp->next->prev = temp->prev;
            free(temp);
           }*/
         /*  current = Event_List;
       while(current != NULL)
       {
          printf("Current Agent %d Event %s\n",current->agent,Event_Names[current->event]);
           current = current->next;
       }   */
}

/**
	Add a new event into the list of operating system events.

	This function inserts a future event into the list Event_List at the proper
	time sequence. Event_List points to the event in the list having the
	smallest time (as defined by the function Compare_time).

	<b> Algorithm: </b>
	\verbatim
	Allocate a new event node
	Set the new event node's fields to the event, agent, and time passed in
	If the event list is empty
		Set the event list header node to the new event node
	Else, if the new event node should precede the event list header node
		Place the new node at the start of the list
	Otherwise, the new node goes in the middle or at the end of the list
		Traverse the event list until reaching the node that should precede the new node
		Add the new node after the node reached in the traversal--handle special case of new node being at the end of the list
	\endverbatim

	@param[in] event -- event to simulate
	@param[in] agent -- user agent causing event
	@param[in] time -- time event occurs
	@retval None
 */
void
Add_Event( int event, int agent, struct time_type* time )
{
    struct event_list* current;
    //Declare new node
    struct event_list* temp;
	
	//Get memory for new node
	temp = malloc(sizeof(struct event_list));
	
	//Set temps variables
	  temp->event = event;
	  temp->agent = agent;
	  temp->time = *time;
          temp->prev = NULL;
          temp->next = NULL;
	

   //If we have no list start it
   if(Event_List == NULL)
   {
      Event_List = temp;
	  return;
   }
    
    //If the new node's time is less than the head of the list, replace the head.
	if(Compare_time(time,&Event_List->time) == -1)
     {
	    temp->next = Event_List;
	    Event_List->prev = temp;
	    Event_List = temp;
	    return;
		
     }
     
     if(Compare_time(time,&Event_List->time) == 0)
     {
        temp->next = Event_List->next;
        Event_List->next->prev = temp;
        Event_List->next = temp;
        temp->prev = Event_List;
         return;
     }	 
	 
         //Start a node to loop through list.
	 current = Event_List;
   
  //While we have another following node and the node we are on is not the right place to put new node, get next node.
   while(current->next != NULL && Compare_time(time,&current->time) == 1)
   {
     current = current->next;
   }
   
   //Put our new node at the end of the current list.
   if(current->next == NULL)
   {
     if(Compare_time(time,&current->time)== 1)
       {
         temp->prev = current;
         current->next = temp;
         return;
       }
      if(Compare_time(time,&current->time) == 0)
        {
           temp->prev = current;
           current->next = temp;
           return;
        }

      temp->next = current;
      temp->prev = current->prev;
      current->prev = temp;
      temp->prev->next = temp;
      return; 
   }
   

   if(Compare_time(time,&current->time) == 0)
    {
       temp->next = current->next;
       temp->prev = current;
       current->next->prev = temp;
       current->next = temp;
       return;
    }

   //We have found the place to put the new node between two nodes so put it there.   
   temp->next = current;
   temp->prev = current->prev;
   current->prev = temp;
   temp->prev->next = temp;
   return;
   
}

/**
	Generate clock interrupt.

	<b> Algorithm: </b>
	\verbatim
	Remove an event from Event_List
	Set Clock, Agent and Event to the respective fields in the removed event
	Write the event to the output file--call write_event()
	Deallocate the removed event item
	Save CPU.mode and CPU.pc into Old_State.
	Change New_State to CPU.mode and CPU.pc
	\endverbatim 

	@retval None
 */
void
Interrupt( )
{
   struct event_list* temp;
   temp = Event_List;
   Event_List = temp->next;
   Clock.seconds = temp->time.seconds;
   Clock.nanosec = temp->time.nanosec;
   Agent = temp->agent;
   Event = temp->event;
   Write_Event(Event,Agent,&Clock);
   free(temp);
   
   Old_State.mode = CPU.state.mode;
   Old_State.pc = CPU.state.pc;
   New_State.mode = CPU.state.mode;
   New_State.pc = CPU.state.pc;
   return;
   
}

/**
	Write an event to the output file.
	
	Call print_out() for all output to file. The format of the output is:
	"  EVENT  AGENT  TIME (HR:xxxxxxxx MN:xx SC:xx MS:xxx mS:xxx NS:xxx )"

	<b> Algorithm: </b>
	\verbatim
	Convert the seconds field of time_type to hours, minutes, and seconds
	Convert the nanoseconds field to milliseconds, microseconds and nanoseconds
	Determine type of agent--user terminal or device:
		If agent ID <= Num_Terminals, then agent is user terminal:
			Agent name is of the form 'U0#' where # = agent ID
		Otherwise, agent is a device:
			Agent name is stored in Dev_Table[ agent - Num_Terminals - 1]
	Print formatted message using event name from Event_Names, agent name, and event times--use print_out()
	\endverbatim 

	@param[in] event -- event
	@param[in] agent -- agent
	@param[in] time -- time
	@retval None
 */
void
Write_Event( int event, int agent, struct time_type *time )
{
   unsigned long micro = 0,mili = 0,min = 0,hr = 0;
   min = time->seconds / 60;
   hr = min / 60;
   micro = (time->nanosec / 1000) % 1000;
   mili = (time->nanosec/1000000);
   
   if(agent <= Num_Terminals)
   {
      print_out("%s  U00%d  HR:%d  MN:%d  SC:%d  MS:%d  mS:%d  NS:%d\n\n",Event_Names[event],agent,hr,min,time->seconds,mili,micro,(time->nanosec%1000));
	  return;
   }
   
   print_out("%s %s  HR:%d  MN:%d  SC:%d  MS:%d  mS:%d  NS:%d\n\n",Event_Names[event],Dev_Table[agent - Num_Terminals -1].name,hr,min,time->seconds,mili,micro,time->nanosec%1000);
   
   return;
}

/**
	Convert event name into event ID number.

	<b> Algorithm: </b>
	\verbatim
	Capitalize event name so that case does not matter
	Verify that name's length is shorter than constant the EVENT_NAME_LENGTH_MAX
	For event ID = 0 to NUM_EVENTS
		Compare event name to Event_Names[ event ID ]
		If they are equal
			Return event ID
		Otherwise,
			Continue in loop
	Report error if no event matched
	\endverbatim

	@param[in] event_name -- name of event
	@retval event -- event ID corresponding to event_name
 */
int
get_event_id( char* event_name )
{
     int i=0;
     int found=FALSE;


     to_uppercase(event_name);
            
         //Make sure event is not to long.	 
	 if(event_name == NULL || strlen(event_name) > EVENT_NAME_LENGTH_MAX)
	     return(-1);
		
          //Find event name

	 while( i <= NUM_EVENTS)
	 {
	     if(strncmp(event_name,Event_Names[i],EVENT_NAME_LENGTH_MAX-1) == 0)
                {
                   found = TRUE;
                   break;
		}  
		   i++;
	 }
	   if(found == FALSE)
	      return (-1);
	   else
	   return(i);
}

/**
	Convert agent name into agent ID number.

	<b> Algorithm: </b>
	\verbatim
	Capitalize agent name so that case does not matter
	Verify that the name's length is shorter than the constant DEV_NAME_LENGTH
	If name starts with 'U', then agent is a user terminal:
		Convert the name (except the initial 'U') into an integer--the agent ID
		Return the agent ID
	Otherwise, agent is a device:
		For agent ID = 0 to Num_Devices
			Compare agent name to the name at Dev_Table[ agent ID ]
			If they are equal
				Return agent ID + Num_Terminals + 1 since device agent IDs follow user agent IDs
			Otherwise,
				Continue in loop
		Report error if no agent name matched
	\endverbatim

	@param[in] agent_name -- name of agent
	@retval agent_id -- agent ID corresponding to agent_name
 */
int
get_agent_id( char* agent_name )
{
	int i=0;
        int found=FALSE;
        agent_name[4] = '\0'; 
        
        to_uppercase(agent_name);
	 

	 if(agent_name == NULL || strlen(agent_name) > DEV_NAME_LENGTH)
            {
              printf("First error"); 
              if(agent_name == NULL)
                printf("It's null\n");
	     return (-1);
            } 
		 
        //We have found a user agent take action
	if(agent_name[0] == 'U')
	{
	  //Increment the agent_name pointer such that atoi only gets the numbers in the string.
      	   found  = atoi((agent_name+1));
	  return(found);
	}
		
 
	 while( i <= Num_Devices)
	 {
	     if(strncmp(agent_name,Dev_Table[i].name,DEV_NAME_LENGTH-1) == 0)
                {
		   found = TRUE;
                   break;
		}  
		   i++;
	 }
	 
	   if(found == FALSE)
            {
              printf("Second error");
	      return (-1);
            }
	   else
	   return(i+Num_Terminals+1);
}

/**
	Print debugging message about Event_List.

	<b> Important Notes:</b>
	\li The debugging message is only printed if the variable DEBUG_EVT is
	turned on (set to 1). This is done by adding the following line to
	config.dat: <br>
	DEBUG_EVT= ON
	\li The function is not necessary to implement as it serves only to help
	the programmer in debugging the implementation of common data structures.

	@retval None
 */
void
Dump_evt( )
{
	// if debug flag is turned on
	if( DEBUG_EVT )
	{
             printf("We are here");
	}
	else // do not print any message
	{ ; }
}

void to_uppercase(char* string)
   {
        int i = 0;
		
		if(string == NULL)
		  return;
		  
		  while(i <= strlen(string))
		  {
                     string[i] = toupper(string[i]);	  
		     i++;
		  }
		  
   }
