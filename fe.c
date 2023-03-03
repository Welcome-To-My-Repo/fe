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
	unsigned long long int size;
	char *contents;
} Line;
Line *Buffer;
unsigned long long int Length = 0;

void up()
{
	if (Length == 0) return;
	if (CurY > 0) CurY --, RenderScreen();
	else if (OffY > 0) OffY --, RenderScreen();
	else {
		CurY = window.ws_row - 1;
		OffY = Length - CurY;
	}
}
void down()
{
	if (Length == 0) return;
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
void left()
{
	if (Length == 0) return;
	if (CurX > 0) CurX--, RenderLine();
	else if (OffX > 0) OffX--, RenderLine();
	else {
		up();
		end();
		RenderScreen();
	}
}
void right()
{
	if (Length == 0) return;
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

void insert(char c)
{
	unsigned long long int x = CurX + OffX, y = CurY + OffY;
	if (Length == 0) 
		Buffer = malloc(sizeof(Line)), 
		Buffer[0].length = 1,
		Buffer[0].size = 80,
		Buffer[0].contents = malloc(sizeof(char) * Buffer[0].size),
		Buffer[0].contents[0] = c,
		OffX = 0, OffY = 0, CurX = 0, CurY = 0;
	else if (c != '\n'){
		Line *b = Buffer[y];
		b.length ++;
		if (!(b.size > b.length)) b.size += 80, b.contents = realloc(b.contents, sizeof(char) * b.size);
		if (x < b.length - 1) 
			for (int i = b.length - 1; i > x; i --)
				b.contents[i] = b.contents[i - 1]; 
		b.contents[x] = c;
	}
	else {
		Length ++;
		Buffer = realloc(Buffer, sizeof(Line) * Length);
		if (y < Length - 2)
			for (int i = Length - 1; i > y + 1; i --)
				Buffer[i] = Buffer[i - 1];
		Buffer[y + 1].size = Buffer[y].size;
		Buffer[y + 1].contents = malloc(sizeof(char) * Buffer[y + 1].size);
		Buffer[y + 1].length = 0;
		for (int i = x; i < Buffer[y].length; i ++)
			Buffer[y + 1].contents[i] = Buffer[y].contents[i], Buffer[y + 1].length ++;
		CurX = 0, OffX = 0;
		if (CurY < window.ws_row) CurY ++;
		else OffY ++;
	}
}
void erase() //backspace
{
	unsigned long long int x = OffX + CurX, y = OffY + CurY;
	if (x > 0)
	{
		Buffer[y].length --;
		for (int i = x - 1; i < Buffer[y].length - 1; i ++)
			Buffer[y].contents[i] = Buffer[y].contents[i + 1];
		if (Buffer[y].size > (Buffer.length + 80))
			Buffer.size -= 80, Buffer.contents = realloc(Buffer[y].contents, sizeof(char) * Buffer[y].size);
		if (CurX > 0) CurX --;
		else OffX --;
	}
	else if (y > 0)
	{
		//concatenate the current line with the line before it
		Buffer[y - 1].size += Buffer[y].size;
		Buffer[y - 1].contents = realloc(Buffer[y - 1].contents, sizeof(char) * Buffer[y - 1].size);
		for (int i = 0; i < Buffer[y].length; i ++)
			Buffer[y - 1].contents[Buffer[y - 1].length + i] = Buffer[y].contents[i];
		Buffer[y - 1].length += Buffer[y].length;
		free(Buffer[y].contents);
		for (int i = y; i < Length - 1; i ++)
			Buffer[i] = Buffer[i + 1];
		Length --;
		Buffer = realloc(Buffer, sizeof(Line) * Length);
	}
}
void delete();
{
	unsigned long long int x = OffX + CurX, y = OffY + CurY;
	if (x < Buffer[y].length)
	{
		if (CurX < window.ws_col) CurX ++;
		else OffX ++;
		erase();
	}
	else if (y < Length - 1)
	{
		Buffer[y].size += Buffer[y + 1].size;
		Buffer[y].contents = realloc(Buffer[y].contents, sizeof(char) * Buffer[y].size);
		for (int i = 0; i < Buffer[y + 1].length; i ++)
			Buffer[y].contents[Buffer[y].length + i] = Buffer[y + 1].contents[i];
		Buffer[y].length += Buffer[y + 1].length;
		free(Buffer[y + 1].contents);
		for (int i = y + 1; i < Length - 1; i ++)
			Buffer[i] = Buffer[i + 1];
		Length --;
		Buffer = realloc(Buffer, sizeof(Line) * Length);
	}
}
void RenderScreen()
{
	char *out = malloc((sizeof(char) * window.ws_col * window.ws_col) + (sizeof(char) * window.ws_row));
	unsigned long long int pos = 0, t = 0;
	for (int i = 0; i < Length; i ++, t = 0)
	{
		/*
			how to render tabs?
			tabs are columns divisible by 8
			so you hit a tab, then you append spaces until a column that's divisible
			but then the cursor may be out of alignment because it's couting offset by characters
		*/
	}
}

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
				case 'A': up(); break;
				case 'B': down(); break;
				case 'C': right(); break;
				case 'D': left(); break;
			}
			else goto QUIT;
			break;
		}
		default:
			if (!(c < ' '))
				insert(c);
			Dirty = 1;
	}
	for (int i = 0; i < 64; i ++)
		input[i] = 0;
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