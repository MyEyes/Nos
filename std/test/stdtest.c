#include <stdio.h>
#include <ipc/port.h>
#include <debug.h>
#include <stdlib.h>
int main(int argc, char** argv)
{
	print("This was printed from user mode!\n");
	
	bochs_break();
	
	char c[2];
	c[0]='a';
	c[1]=0;
	do
	{
		c[0] = getc();
		print(c);
	}while(c[0]!='\n');
	
	return 0;
}