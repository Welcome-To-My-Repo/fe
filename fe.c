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
unsigned long long int CurX = 0, CurY = 0, OffX = 0, OffY = 0;
typedef struct l {
	unsigned long long int length;
	char *contents;
} Line;
Line *Buffer;
unsigned long long int Length = 0;

int main (int argc, char **argv)
{
	tcgetattr(0, &restore);
	interface = restore;
	interface.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	interface.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	interface.c_cflag &= ~(CSIZE | PARENB);
	interface.c_cflag |= CS8;
	tcsetattr(0, TCSANOW, &interface);
	ioctl(1, TIOCGWINSZ, &window);
	

GET_KEY:
	char c, *input, input_size = 0;
	input = malloc(sizeof(char) * 128);
	while (read(0, &c, 1) > 0 && input_size < 128)
	{
		input[input_size] = c;
		input_size ++;
	}

INTERPRET_KEY:
	char n;
	if (input_size == 1) switch (n = input[0]) 
	{
		case ('q' - ('a' - 1)):
		case 10:
			goto EXIT;
		default:
		{
			write(1, &n, 1);
			goto GET_KEY;
		}
	}
	else switch (n = input[input_size - 1])
	{
	}

PRINT_LINE:
	ioctl(1, TIOCGWINSZ, &window);
	write(1, "\r\x1b[2K", 5);
	if (Buffer[CurY].length > window.ws_col)
	{
			
	}
	else
	{
		write(1, Buffer[CurY].contents, Buffer[CurY].length);
		write(1, "\r", 1);
		char *t = malloc(sizeof(char) * OffX * 3);
		for (int i = 0; i < OffX; i += 3)
			t[i] = '\x1b', 
			t[i + 1] = '[',
			t[i + 2] = 'C';
	}

PRINT_WINDOW:
	ioctl(1, TIOCGWINSZ, &window);
	

QUIT:

	
EXIT:
	tcsetattr(0, TCSANOW, &restore);
	exit(0);
	return 0;
}
