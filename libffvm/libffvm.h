#ifndef __LIBFFVM_H__
#define __LIBFFVM_H__

void libffvm_init(void);
void libffvm_exit(void);

int  getch (void);
int  kbhit (void);
void clrscr(void);
void gotoxy(int x, int y);
void mdelay(int ms);

#endif

