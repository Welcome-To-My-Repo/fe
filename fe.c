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
void up();	
void down();
void left();
void right();
void end();
void start();
typedef struct l {
	unsigned long long int length;
	unsigned long long int size;
	char *contents;
} Line;
Line *Buffer;
unsigned long long int Length = 0;
void insert(unsigned long long int x, unsigned long long int y, char c);
void delete(unsigned long long int x, unsigned long long int y);
void deleteright(unsigned long long int x, unsigned long long int y);
void concatenate(unsigned long long int y);
void RenderScreen();
void RenderLine();

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
	ioctl(1, TIOCGWINSZ, &window);

	char input[128], size;
GET_KEY:
	size = 0;
	for (int i = 0; i < 128; i ++) {
		input[i] = 0;
		read(0, &input[i], 1);
		if (input[i] == 0) break;
		size ++;
	}
	if (size == 0) goto GET_KEY;
INTERPRET:
	ioctl(1, TIOCGWINSZ, &window);
	char c = 0; 
	if (size == 1) switch(c = input[0])
	{
		case 27: goto QUIT;
		case 8:
		{
			delete((CurX + OffX), (CurY + OffY));
			break;
		}
		case 127:
		{
			deleteright((CurX + OffX), (CurY + OffY));
			break;
		}
		case 10:
		{
			insert((CurX + OffX), (CurY + OffY), c);
			down();
			start();
			break;
		}
		default:
		{
			if (c < ' ') goto GET_KEY;
			else insert((CurX + OffX), (CurY + OffY), c);
		}
	}
	else switch(c = input[size - 1])
	{
		case 'A': //up
			up(); break;
		case 'B': //down
			down(); break;
		case 'C': //right
			right(); break;
		case 'D': //left
			left(); break;
		default:
		{}
	}
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

void up()
{
	if (CurY > 0) CurY --, RenderScreen();
	else if (OffY > 0) OffY --, RenderScreen();
	else {
		CurY = window.ws_row - 1;
		OffY = Length - CurY;
	}
}
void down()
{
	if (CurY + OffY < Length)
	{
		if (CurY < window.ws_row) CurY ++;
		else OffY ++;
		RenderScreen();
	}
	else
	{
		CurY = 0;
		OffY = 0;
	}
}
void right()
{
	if (CurX + OffX > Buffer[CurY + OffY].length)
	{
		if (CurX < window.ws_col) CurX++;
		else OffX ++;
		RenderLine();
	}
	else
	{
		down();
		start();
		RenderScreen();
	}
}
void left()
{
	if (CurX > 0) CurX--, RenderLine();
	else if (OffX > 0) OffX--, RenderLine();
	else {
		up();
		end();
		RenderScreen();
	}
}
void end()
{
	CurX = window.ws_col - 1;
	OffX = Buffer[CurY + OffY].length - CurX;
}
void start()
{
	CurX = 0;
	OffX = 0;
}
void insert (unsigned long long int x, unsigned long long int y, char c)
{
	if (Length == 0)	
		Buffer = malloc(sizeof(Line)),
		Buffer[0].length = 0,
		Buffer[0].size = 0,
		Buffer[0].contents = 0;
	
	if (y >= Length) return;

	Buffer[y].length ++;
	
	if (x >= Buffer[y].length) return;

	if (c != 10) {
	
		if (Buffer[y].size = 0 || Buffer[y].size <= Buffer[y].length)
			Buffer[y].size += 80,
			Buffer[y].contents = realloc(Buffer[y].contents, sizeof(char) * Buffer[y].size);

		if (x < Buffer[y].length)
			for (int i = Buffer[y].length - 1; i > x; i --)
				Buffer[y].contents[i] = Buffer[y].contents[i - 1];
	
		Buffer[y].contents[x] = c;
	}
	else {
		
		Length++;
		Buffer = realloc(Buffer, sizeof(Line) * Length);
		if (y < Length - 1)
			for (int i = Length - 1; i > y; i --)
				Buffer[i] = Buffer[i - 1];
		Buffer[y + 1].length = 0, Buffer[y + 1].size = 0, Buffer[y + 1].contents = 0;
		if (x < Buffer[y].length) {
			while(Buffer[y + 1].size < (Buffer[y].length - x)) Buffer[y + 1].size += 80;
			Buffer[y + 1].contents = malloc(sizeof(char) * Buffer[y + 1].size);
			for (int i = x; i < Buffer[y].length; i++)
				Buffer[y + 1].contents[i] = Buffer[y].contents[i], 
				Buffer[y + 1].length ++;
		}
	}
	return;
}

void delete (unsigned long long int x, unsigned long long int y)
{
	if (Length == 0 || y >= Length) return;
	if (x >= Buffer[y].length) return;
	if ((Buffer[y].length == 0 && y > 0) || (x == 0 && y > 0))
		concatenate(y - 1), up(), end();
	else if (Buffer[y].length == 0) return;
	if (x < Buffer[y].length - 1)
		for (int i = x; i < Buffer[y].length - 1; i ++)
			Buffer[y].contents[i] = Buffer[y].contents[i + 1];
	Buffer[y].length --;
	if (Buffer[y].size > (Buffer[y].length + 160))
		Buffer[y].size -= 80,
		Buffer[y].contents = realloc(Buffer[y].contents, sizeof(char) * Buffer[y].size);
	return;
}

void deleteright (unsigned long long int x, unsigned long long int y)
{
	if (Length == 0 || y >= Length) return;
	if (x >= Buffer[y].length) return;
	if (x < Buffer[y].length - 1)
		delete(x + 1, y);
	else 
	concatenate(y)
}
void concatenate(unsigned long long int y)
{
	if (y < Length - 1)
	{
		Buffer[y].size += Buffer[y + 1].size;
		Buffer[y].contents = realloc(Buffer[y].contents, sizeof(char) * Buffer[y].size);
		Buffer[y].length += Buffer[y + 1].length;
		for(int i = 0; i < Buffer[y + 1].length; i ++)
			Buffer[y].contents[Buffer[y].length + i] = Buffer[y + 1].contents[i];
		for (int i = y; i < Length - 1; i ++)
			Buffer[i] = Buffer[i + 1];
		Length --;
		Buffer = realloc(Buffer, sizeof(Line) * Length);
}
void RenderScreen()
{
	char *out = malloc(sizeof(char) * window.ws_row * window.ws_col);
	unsigned long long int out_length = 0;

	write(1, "\x1b[3J\x1b[;H", 8);

	for (int i = OffY; i < Length; i ++)
	{
		if ((i - OffY) == CurY)
		{
			if (CurX + OffX < Buffer[i].length)
				for (int j = OffX; j < Buffer[i].length && j < (window.ws_col + OffX); j++)
				{
					if (Buffer[i].contents[j] == '\t')
						for (int k = 0; k < 8; k ++)
							out[out_length] = ' ', out_length++, j++;
					else
						out[out_length] = Buffer[i].contents[j], out_length++;
				}
			else
			{
				if (Buffer[i].length < window.ws_col)
				{
					for (int j = 0; j < Buffer[i].length; j ++)
						out[out_length] = Buffer[i].contents[j], out_length++;
				}
				else
				{
					for(int j = Buffer[i].length - window.ws_col; j < Buffer[i].length; j++)
						out[out_length] = Buffer[i].contents[j], out_length++;
				}
			}
		}
		else
		{
			for (int j = 0; j < Buffer[i].length && j < window.ws_col; j ++)
			{
				if (Buffer[i].contents[j] == '\t')
					for(int k = 0; k < 8; k ++)
						out[out_length] = ' ', out_length++;
				else 
					out[out_length] = Buffer[i].contents[j], out_length++;
			}
		}
		out[out_length] = '\n';
	}
	write(1, out, out_length);
	free(out);
	return;
}

void RenderLine()
{
	write(1, "\r\x1b[2K", 5);
	char *out = malloc(sizeof(char) * window.ws_col + (OffX * 3) + 1);
	unsigned long long int out_length;
	for (int i = OffX; i < Buffer[OffY + CurY].length && i < window.ws_col + OffX; i ++)
		out[out_length] = Buffer[OffY + CurY].contents[i], out_length++;
	out[out_length] = '\r', out_length++;
	for (int i = 0; i < OffX; i += 3)
		out[out_length] = '\x1b', out_length++,
		out[out_length] = '[', out_length++,
		out[out_length] = 'C', out_length++;
	write(1, out, out_length);
}
