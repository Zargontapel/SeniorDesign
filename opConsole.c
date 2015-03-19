#include <stdio.h>
#include <ncurses.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "UDPclient.h"

#define WIDTH 30
#define HEIGHT 10 

int startx;
int starty;
int c;
int comm_index;
int setupActive = 1;
int systemRunning = 1;
int debugChannel = 0;  //which channel to watch in debug mode, not used in normal mode
	
char comm[30]; //holds initial command upon entry (unparsed, includes all dashes and options)
char setup_comm[6]; //contains initial command
char *parsedComm[5]; //holds parts of the command. Each part is a pointer to a string. [0] is command itself, [1]-[4] contain the options

int n_channels = 16; //these are the channels coming in from the ADC
time_t now; //use system time to delay table refresh to ~3Hz
FILE *fp;
bool debugview = FALSE;
bool datalog = false;

void print_table(WINDOW *table_win);
void print_console(WINDOW *console_win);
void setup(WINDOW *setup_win);
void parse_comm(WINDOW *destination, char *command);
void comm_handle(WINDOW *comm_win, int numOptions);

int main()
{	WINDOW *table_win;
	WINDOW *console_win;
	WINDOW *setup_win;
	comm_index = 0;

	initscr();
	clear();
	noecho();
	
	nocbreak();  // Line buffering disabled. pass on everything
	startx = 0;
	starty = 0;
		
	table_win = newwin(n_channels, COLS, 1, startx);
	console_win = newwin(3, COLS, n_channels + 1, startx);
	setup_win = newwin(3, COLS, 3, startx);
	print_console(setup_win);
	mvwprintw(setup_win, 0, 0, "Please enter the setup command to begin.");
	mvprintw(0, 0, "Welcome to the NRAO Feed Arm Servo Testing Interface -v0.1");
	refresh();
	while(setupActive)
	{
		print_console(setup_win);
	}
	setupActive = 1;
	int i = 0;
	for(i; i < 30; i++)
	{
		comm[i] = ' ';
	}
	comm_index = 0;
	while(systemRunning)
	{	
		print_table(table_win);
		print_console(console_win);
	}	
	clrtoeol();
	refresh();
	endwin();
	return 0;
}


void print_table(WINDOW *table_win)
{
	int x, y, i;
	x = 0;
	y = 0;	
	if(now != time(0))
	{
		now = time(0);
		if(!debugview) //only show all values if we are in normal mode
		{
			for(i = 0; i < n_channels; ++i,++y)
			{	
				char* result;
				result = sendRequest(" ");
				mvwprintw(table_win, y, x, "Channel %i: %s", i, result);
				if(datalog && fp != NULL)
				{
					fprintf(fp, "%s\t", result);
				}
			}
			if(datalog && fp != NULL)
			{
				fprintf(fp, "\n");
			}
		}
		else
		{
			for(i = 0; i < n_channels; ++i, ++y)
			{
				mvwprintw(table_win, y, x, "                ");
				if(i == debugChannel)
				{
					mvwprintw(table_win, y, x, "Channel %i: %s", debugChannel, sendRequest(" "));
				}
			}
		}
		wrefresh(table_win);
	}
}

void print_console(WINDOW *console_win)
{
	nodelay(console_win, true); //makes wgetch a non-blocking call
	cbreak();
	mvwprintw(console_win, 1, 0, "~> ");
	if(c = wgetch(console_win))
	{
		if(c != ERR)
		{
			switch(c)
			{
				case 127: //backspace
					if(comm_index >= 0)
					{
						comm_index--;
						comm[comm_index] = ' ';
						mvwprintw(console_win, 1, 0, "~> %s", comm);
						wrefresh(console_win);
					}
					break;
				case '\n': //Enter
					mvwprintw(console_win, 1, 0, "~>                                 ");
					mvwprintw(console_win, 2, 0, "                                   ");
					comm_index = 0;
					parse_comm(console_win, comm);
					int i = 2;
					for(i; i < 30; i++)
					{
						comm[i] = ' ';
					}
					break;
				default:
					comm[comm_index] = c;
					comm_index++;
					mvwprintw(console_win, 0, 0, "                                              ");
					mvwprintw(console_win, 0, 0, "Character pressed is %c", c);
					mvwprintw(console_win, 1, 0, "~> %s", comm);
					wrefresh(console_win);
					break;
			}
			
		}
	}
}

void setup(WINDOW *setup_win)
{
	nodelay(setup_win, true); //makes wgetch a non-blocking call
	cbreak();
	if(c = wgetch(setup_win))
	{
		if(c != ERR)
		{
			switch(c)
			{
				case 127: //backspace
					comm_index--;
					setup_comm[comm_index] = ' ';
					int currentX;
					int currentY;
					getyx(setup_win, currentY, currentX);
					wmove(setup_win, currentY, currentX-1);
					mvwprintw(setup_win, 0, 0, "%s", setup_comm);
					wrefresh(setup_win);
					break;
				case '\n': //Enter
					setup_comm[comm_index] = 'q';
					comm_index++;
					mvwprintw(setup_win, 0, 0, "%s", setup_comm);
					wrefresh(setup_win);
					break;
				default:
					setup_comm[comm_index] = c;
					comm_index++;
					mvwprintw(setup_win, 0, 0, "%s", setup_comm);
					wrefresh(setup_win);
					break;
			}
		}
	}
}

void parse_comm(WINDOW *destination, char *command) //store resulting parts in parsedComm array
{
	char *token;
	char s[3] = " -";
	token = strtok(command, s);

	int commNumber = 0;  //keeps track of which part of the command we are at
   
   /* walk through other tokens */
   while( token != NULL ) 
   {
    	parsedComm[commNumber] = token;
        token = strtok(NULL, s);
	commNumber++;
   }
	comm_handle(destination, commNumber - 1);
}

void comm_handle(WINDOW *comm_win, int numOptions) //get parts from parsedComm and complete the necessary operation
{
	int i = 0;
	/* THIS SECTION FOR DEBUGGING ONLY
	char *command;
	for(i; i <= numOptions; i++)
	{
		mvwprintw(comm_win, 2, i*10, "%s", parsedComm[i]);
	}*/
	if(!strcmp(parsedComm[0], "display"))
	{
		i = 1;
		for(i; i <= numOptions; i++)
		{
			char *option;
			option = strtok(parsedComm[i], "=");
			if(!strcmp(option, "n") || !strcmp(option, "normal"))
			{
				debugview = false;
				mvwprintw(comm_win, 2, 0, "Reset to normal mode");
			}
			else if(!strcmp(option, "d") || !strcmp(option, "debug"))
			{
				debugview = true;
				mvwprintw(comm_win, 2, 0, "Set to debug mode");
			}
			else if(!strcmp(option, "v") || !strcmp(option, "view"))
			{
				debugChannel = atoi(strtok(NULL, "="));
				mvwprintw(comm_win, 2, 0, "Now watching channel %i", debugChannel);
			}
		}
	}
	else if(!strcmp(parsedComm[0], "help"))
	{
		mvwprintw(comm_win, 2, 0, "Available commands are: setup display help send exit");
	}
	else if(!strcmp(parsedComm[0], "send"))
	{
		char *value = "Packet!";
		i = 1;
		for(i; i <= numOptions; i++)
		{
			char *option;
			char *channel;
			option = strtok(parsedComm[i], "=");
			if(!strcmp(option, "c") || !strcmp(option, "channel"))
			{
				channel = strtok(NULL, "=");
				mvwprintw(comm_win, 2, 0, "Channel selected: %s", channel);
			}
			else if(!strcmp(option, "v") || !strcmp(option, "value"))
			{
				value = strtok(NULL, "=");
				mvwprintw(comm_win, 2, 0, "Value to be sent has been chosen: %s", value);
			}
		}
		mvwprintw(comm_win, 2, 0, "Response is: %s", sendRequest(value));
	}
	else if(!strcmp(parsedComm[0], "setup"))
	{
		char *server_addr = "127.0.0.1";
		i = 1;
		for(i; i <= numOptions; i++)
		{
			char *option;
			option = strtok(parsedComm[i], "=");
			if(!strcmp(option, "a") || !strcmp(option, "address"))
			{
				char *address = strtok(NULL, "=");
				server_addr = address;
			}
			else if(!strcmp(option, "l") || !strcmp(option, "log"))
			{
				datalog = true;
				mvwprintw(comm_win, 2, 0, "Logging has been enabled");
			}
			else if(!strcmp(option, "n") || !strcmp(option, "nolog"))
			{
				datalog = false;
				mvwprintw(comm_win, 2, 0, "Logging has been disabled");
			}
			else if(!strcmp(option, "f") || !strcmp(option, "file"))
			{
				char *filename = strtok(NULL, "=");
				fp = fopen(filename, "a");
			}
		}
		mvwprintw(comm_win, 2, 0, "The target address is %s", server_addr);
		init(server_addr);
		setupActive = 0;
		main();
	}
	else if(!strcmp(parsedComm[0], "exit"))
	{
		sendMessage("close");
		closeSocket();
		setupActive = 0;
		systemRunning = 0;
	}
}
