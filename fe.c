#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

struct termios interface, restore;
struct winsize window;

char *pathname;
int Dirty = 0;
unsigned long long int X = 0, Y = 0, wX = 0, wY = 0;
typedef struct l {
	unsigned long long int length;
	unsigned long long int size;
	char *contents;
} Line;
Line *Buffer;
unsigned long long int Length = 0;

void Insert(char c);
void Erase();
void Delete();
void Up();
void Down();
void Right();
void Left();
void Start();
void End();

int main (int argc, char **argv)
{
	tcgetattr(0, &restore);
	interface = restore;
	interface.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	interface.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	interface.c_cflag &= ~(CSIZE | PARENB);
	interface.c_cflag |= CS8;
	interface.c_cc[VMIN] = 0;
	interface.c_cc[VTIME] = 1;
	tcsetattr(0, TCSANOW, &interface);
	char c;
	char input[64], count;
GET_KEY:
	ioctl(1, TIOCGWINSZ, &window);
	input[0] = 0;
	read(1, input, 64);
	if (input[0] != 0) switch(c = input[0])
	{
		case 27 :
		{
			if (input[1] == '[')
			switch(input[2])
			{
				case 'A': Up(); break;
				case 'B': Down(); break;
				case 'C': Right(); break;
				case 'D': Left(); break;
			}
			else goto QUIT;
			break;
		}
		case '\n':
		{
			Insert('\n');
			break;
		}
		case '\b':
		{
			Erase();
			break;
		}
		case 127:
		{
			Delete();
			break;
		}
		default:
			if (!(c < ' '))
				insert(c);
			Dirty = 1;
	}
	input[0] = 0, input[1] = 0, input[2] = 0;, input[3] = 0;
	goto GET_KEY;


QUIT:
	if (Dirty)
	{
		write(1, "Unsaved Changes!", sizeof("Unsaved Changes!"));
	}

	
EXIT:
	write(1, "\n", 1);
	tcsetattr(0, TCSANOW, &restore);
	exit(0);
	return 0;
}

void Insert(char c)
{
}
void Erase()
{
}
void Delete()
{
}
void Up()
{
	if (Length == 0) return;
	if (Y > 0){
		Y --;
		if (Y < wY) wY = Y;
	}
	else
	{
		Y = Length - 1;
		wY = Length - window.ws_row;
	}
}
void Down()
{
}
void Right()
{
}
void Left()
{
}
void Start()
{
}
void End()
{
}
