/* This files provides address values that exist in the system */

#define SDRAM_BASE            0xC0000000
#define FPGA_ONCHIP_BASE      0xC8000000
#define FPGA_CHAR_BASE        0xC9000000

/* Cyclone V FPGA devices */
#define LEDR_BASE             ((volatile long *)0xFF200000)
#define HEX3_HEX0_BASE        ((volatile long *)0xFF200020)
#define HEX5_HEX4_BASE        ((volatile long *)0xFF200030)
#define SW_BASE               0xFF200040
#define KEY_BASE              0xFF200050
#define TIMER_BASE            0xFF202000
#define PIXEL_BUF_CTRL_BASE   0xFF203020
#define CHAR_BUF_CTRL_BASE    0xFF203030

/* VGA colors */
#define WHITE 0xFFFF
#define YELLOW 0xFFE0
#define RED 0xF800
#define GREEN 0x07E0
#define BLUE 0x001F
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define GREY 0xC618
#define PINK 0xFC18
#define ORANGE 0xFC00
#define BLACK 0x0000
#define DEEP_BLUE 0x1A11
#define BROWN 0x5A06
#define DARK_GREEN 0x006400
#define DARK_ORANGE 0xE9672D
#define PURPLE 0x6876

#define ABS(x) (((x) > 0) ? (x) : -(x))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

/* Screen size. */
#define RESOLUTION_X 320
#define RESOLUTION_Y 240

/* Constants for animation */
#define BOX_LEN 14
#define TOP

#define FALSE 0
#define TRUE 1

#include <stdlib.h>
#include <stdio.h>
#include <math.h> // Added this to support sqrt
#include <stdbool.h> // Added this to support bool
#include <time.h> // Added this to support rng

volatile int pixel_buffer_start; // global variable



// ==================================================================== //
/* Initialize the banked stack pointer register for IRQ mode */
void set_A9_IRQ_stack(void) 
// Copied from http://www-ug.eecg.toronto.edu/msl/handouts/DE1-SoC_arm_Computer.pdf, ~P35; not guaranteed to work
{
	int stack, mode;
	stack = 0xFFFFFFFF-7; // top of A9 on-chip memory, aligned to 8 bytes
	/* change processor to IRQ mode with interrupts disabled */
	mode = 0b11010010;
	asm("msr cpsr, %[ps]" : : [ps] "r" (mode));
	/* set banked stack pointer */
	asm("mov sp, %[ps]" : : [ps] "r" (stack));
	/* go back to SVC mode before executing subroutine return! */
	mode = 0b11010011;
	asm("msr cpsr, %[ps]" : : [ps] "r" (mode));
}

/* Turn on interrupts in the ARM processor */
void enable_A9_interrupts(void)
{
	int status = 0b01010011;
	asm("msr cpsr, %[ps]" : : [ps]"r"(status));
}

/* Configure the Generic Interrupt Controller (GIC) */
void config_GIC(void)
// Copied from http://www-ug.eecg.toronto.edu/msl/handouts/DE1-SoC_arm_Computer.pdf; not guaranteed to work
{
	/* configure the HPS timer interrupt */
	*((int *) 0xFFFED8C4) = 0x01000000;
	*((int *) 0xFFFED118) = 0x00000080;
	/* configure the FPGA interval timer and KEYs interrupts */
	*((int *) 0xFFFED848) = 0x00000101;
	*((int *) 0xFFFED108) = 0x00000300;
	// Set Interrupt Priority Mask Register (ICCPMR). Enable interrupts of all priorities
	*((int *) 0xFFFEC104) = 0xFFFF;
	// Set CPU Interface Control Register (ICCICR). Enable signaling of interrupts
	*((int *) 0xFFFEC100) = 1; // enable = 1
	// Configure the Distributor Control Register (ICDDCR) to send pending interrupts to CPUs
	*((int *) 0xFFFED000) = 1; // enable = 1
}

/* setup the KEY interrupts in the FPGA */
void config_KEYs()
// Copied from http://www-ug.eecg.toronto.edu/msl/handouts/DE1-SoC_arm_Computer.pdf; not guaranteed to work
{
	volatile int * KEY_ptr = (int *) 0xFF200050; // pushbutton KEY address
	*(KEY_ptr + 2) = 0xF; // enable interrupts for all four KEYs
}

volatile int key_pressed;
void interval_timer_ISR () {}
void pushbutton_ISR()
// Copied from http://www-ug.eecg.toronto.edu/msl/handouts/DE1-SoC_arm_Computer.pdf; not guaranteed to work
{
	volatile int * KEY_ptr = (int *) 0xFF200050;
	int press;
	press = *(KEY_ptr + 3); // read the pushbutton interrupt register
	*(KEY_ptr + 3) = press; // clear the interrupt
	if (press & 0x1) // KEY0
		key_pressed = 0;
	else if (press & 0x2) // KEY1
		key_pressed = 1;
	else if (press & 0x4) // KEY2
		key_pressed = 2;
	else if (press & 0x8)// press & 0x8, which is KEY3
		key_pressed = 3;
	else
		key_pressed = -1;
	return;
}

/* Define the IRQ exception handler */
void __attribute__ ((interrupt)) __cs3_isr_irq (void)
// Copied from http://www-ug.eecg.toronto.edu/msl/handouts/DE1-SoC_arm_Computer.pdf; not guaranteed to work
{
	// Read the ICCIAR from the processor interface
	int int_ID = *((int *) 0xFFFEC10C);
	if (int_ID == 72) // check if interrupt is from the Intel timer
		interval_timer_ISR ();
	else if (int_ID == 73) // check if interrupt is from the KEYs
		pushbutton_ISR ();
	else
		while (1); // if unexpected, then stay here
	// Write to the End of Interrupt Register (ICCEOIR)
	*((int *) 0xFFFEC110) = int_ID;
	return;
}
// ==================================================================== //



void clear_screen();
void swap(int*, int*);
void draw_line(int, int, int, int, int);
void draw_rect(int, int, int, int, int);
void draw_circ(int, int, int, int);
void draw_text(int, int, char*);
void plot_pixel(int, int, short int);
void wait(int);
void hex_update(int, int);
// Game specific functions
void init();
void draw_board();
void draw_pieces(bool, bool);
int diceroll();
bool is_safe();
const int colorList[5] = {DARK_GREEN, PURPLE, DEEP_BLUE, ORANGE};
const int bgcolorList[5] = {GREEN, PINK, CYAN, YELLOW};
const int startPos[5] = {1,14,27,40};
const int endPos[5] = {51,12+52,25+52,38+52};
const int hexList[11] = {0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07};
// Global vars
int playerTurn, pieceSelect; // the player that is playing and the piece selected; 0-3.
int piecePos[5][5]; // the positions of the pieces; [0][0] to [3][3]. larger array for error containment. count total length of path. 
int diceVal;
bool completion[5] = {0,0,0,0};

int main(void)
{
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
    /* Read location of the pixel buffer from the pixel buffer controller */
    pixel_buffer_start = *pixel_ctrl_ptr;
	
	//volatile int * key_ptr = (int *) 0xFF200050;
	
	init();
	set_A9_IRQ_stack();
	config_GIC();
	config_KEYs();
	enable_A9_interrupts();
	
	srand(time(0)); // Sets randomizer to time-based
	
	while (1) {
		wait(1);
		
		// Handle interrupts / states?
		if (key_pressed==-1) {
			while (key_pressed<0) {*LEDR_BASE = 0x2aa; wait(15); *LEDR_BASE = 0x155; wait(15);}
			if (key_pressed>0) {key_pressed=0;*LEDR_BASE = 0x0;hex_update(0,-1);}}
		if (key_pressed==0) { // KEY0: roll dice & select piece
			if (completion[playerTurn]) {playerTurn++; playerTurn = playerTurn%4; hex_update(1, playerTurn);continue;}
			
			for (int i=0;i<9;i++) {diceroll();wait(1);} // Animation
			diceVal = diceroll(); // Records value
			
			bool isYard = true;// Checks if there are movable pieces
			for (int i=0;i<4;i++) { 
				if (piecePos[playerTurn][i]>-1&&piecePos[playerTurn][i]<1006) {isYard = false; break;}}
			if ((isYard && diceVal<6)||diceVal<1) { // No pieces movable then continue
				wait(60);
				playerTurn++; playerTurn = playerTurn%4;
				hex_update(1, playerTurn);
				continue;
			}
			
			pieceSelect = -1;hex_update(3, pieceSelect);
			while (1) {
				if (key_pressed==0) { // KEY0: Selecting piece
					pieceSelect = pieceSelect+1; pieceSelect = pieceSelect%4;
					hex_update(3, pieceSelect);
					// Continue if a piece is not selectable
					if ((diceVal<6&&piecePos[playerTurn][pieceSelect]<=-1)||piecePos[playerTurn][pieceSelect]>1005) continue; 
					
					// Do draws around selected piece here
					draw_pieces(1,0);wait(1);
					draw_pieces(0,1);wait(1);
					
					key_pressed=-1;continue;
				}
				if (key_pressed>0) {// Something happened; end piece selection.
					//draw_circ(160,120,20,GREEN);//test
					break;
				} 
			}
			
		}
		if (key_pressed==1) { // KEY1: confirms piece & starts moving
			draw_pieces(1,0);wait(1); // Clears current spaces
			
			// If already on board then move as usual
			if (piecePos[playerTurn][pieceSelect]>-1) {
				piecePos[playerTurn][pieceSelect] = piecePos[playerTurn][pieceSelect] + diceVal;
				if ((piecePos[playerTurn][pieceSelect] > endPos[playerTurn])&&(piecePos[playerTurn][pieceSelect]<1000)) {
					piecePos[playerTurn][pieceSelect] = 1000+(piecePos[playerTurn][pieceSelect]-endPos[playerTurn]);
				}
				
				bool isCheck = false;
				
				for (int i=0;i<4;i++) { // Check for checks
					if (is_safe(piecePos[playerTurn][pieceSelect]%52)||piecePos[playerTurn][pieceSelect]>1005) break; // Do not check if on safespot or has won
					if (i==playerTurn) continue; // Do not check own piece
					for (int j=0;j<4;j++) {
						if (piecePos[i][j]<1000&&
							piecePos[i][j]%52==piecePos[playerTurn][pieceSelect]%52) {piecePos[i][j]=-1;isCheck = true;
																					  hex_update(4,2);break;}
					}
					if (isCheck) break;
				}
				if (!isCheck) {playerTurn++; playerTurn = playerTurn%4;hex_update(1, playerTurn);}
				
			}
			// Else move onto board
			else {
				piecePos[playerTurn][pieceSelect] = startPos[playerTurn];
				hex_update(4,1);
			}
			
			// Check win conditions
			int isWin = 4, complete=1;
			
			for (int i=0;i<4;i++) {
				if (piecePos[playerTurn][i]<1006) {complete=0;break;}
			}
			if (complete) completion[playerTurn]=1;
			for (int i=0;i<4;i++) {
				for (int j=0;j<4;j++) {
					if (piecePos[i][j]<1006) {isWin--;break;}
				}
				if (isWin<3) break;
			}
			if (isWin>=3) {draw_pieces(0,0);wait(1);key_pressed = 3;continue;}
			
			draw_pieces(0,0);wait(1); // Draws new pieces
			hex_update(0,-1);key_pressed = 0; continue;
		}
		else if (key_pressed==3) { // KEY3: start new game
			init();wait(300);
			key_pressed = -1;
		}
		
		//key_pressed = -1;
	}
}

// code not shown for clear_screen() and draw_line() subroutines

void wait(int turns) {
	volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
	volatile int * pixel_stat_ptr = (int *)0xFF20302C;
	
	for (int i=0;i<turns;i++) {
		*pixel_ctrl_ptr=1;
		while ((*pixel_stat_ptr)&1) {(*pixel_stat_ptr) = (*pixel_stat_ptr);}
	}
}

void clear_screen() {
	draw_rect(0,0,320,240, DEEP_BLUE);
}

void swap(int* a, int* b){
	*a = *a + *b;
	*b = *a - *b;
	*a = *a - *b;
}

void draw_line(int x0, int y0, int x1, int y1, int color) {
	bool is_steep = (abs(y1-y0)>abs(x1-x0));
	
	if (is_steep) {swap(&x0, &y0);swap(&x1, &y1);}
	if (x0 > x1) {swap(&x0, &x1);swap(&y0, &y1);}
	
	int dx = x1 - x0, dy = abs(y1 - y0);
	int error = -(dx/2);
	
	int y = y0, y_step;
	if (y0 < y1) {y_step = 1;} else {y_step = -1;}
	
	for (int x = x0; x <= x1; x++) { // x<=x1?
		if (is_steep) {plot_pixel(y, x, color);}
		else {plot_pixel(x, y, color);}
		error+=dy;
		
		if (error>=0) {
			y+=y_step;
			error-=dx;
		}
	}
}

void draw_rect(int x0, int y0, int x1, int y1, int color) {
	for (int i=x0;i<x1;i++) {
		for (int j=y0;j<y1;j++) {
			plot_pixel(i,j,color);
		}
	}
}

void draw_circ(int x0, int y0, int r, int color) {
	for (int i=MAX(0,x0-r);i<=MIN(319, x0+r);i++) {
		for (int j=MAX(0, y0-r);j<=MIN(239, y0+r);j++) {
			if (sqrt((x0-i)*(x0-i)+(y0-j)*(y0-j))<=r)
				plot_pixel(i,j,color);
		}
	}
}

void draw_text(int x, int y, char* c) {
	volatile char * character_buffer = (char *) (0xC9000000 + (y<<7) + x);
	while (*c) {
		character_buffer = (char *) (0xc9000000+(y<<7)+x);
		*character_buffer = *c;
		x++;
		c++;
	}
}

void plot_pixel(int x, int y, short int line_color)
{
    *(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}

// Game specific functions

void init() {
	clear_screen();
	draw_board();
	playerTurn=0; pieceSelect=0;key_pressed=-1;
	*HEX5_HEX4_BASE=0x00000038;*HEX3_HEX0_BASE=0x1c5e5c00;
	*LEDR_BASE = 0;
	for (int i=0;i<5;i++) {
		for (int j=0;j<5;j++) {
			piecePos[i][j] = -1;
		}
	}
	
	draw_pieces(0,0);
	
	char * playerName;
	playerName = "PLAYER 0"; draw_text(15,2,playerName);
	playerName = "PLAYER 1"; draw_text(58,2,playerName);
	playerName = "PLAYER 2"; draw_text(15,57,playerName);
	playerName = "PLAYER 3"; draw_text(58, 57, playerName);
}

void draw_board() {
	draw_rect(40, 0, 280, 240, BROWN); // 240x240 square; board outer boundary
	
	draw_rect(55, 15, 265, 225, WHITE); // 210x210 square; board inner boundary
	
	for (int i=1;i<=6;i++) { // Draw box borders step 1
		draw_line(55+BOX_LEN*i, 15+BOX_LEN*6, 55+BOX_LEN*i, 15+BOX_LEN*9,BLACK);
		draw_line(55+BOX_LEN*(15-i), 15+BOX_LEN*6, 55+BOX_LEN*(15-i), 15+BOX_LEN*9, BLACK);
		draw_line(55+BOX_LEN*6, 15+BOX_LEN*i, 55+BOX_LEN*9, 15+BOX_LEN*i,BLACK);
		draw_line(55+BOX_LEN*6, 15+BOX_LEN*(15-i), 55+BOX_LEN*9, 15+BOX_LEN*(15-i), BLACK);
	}
	
	for (int i=6;i<=9;i++) { // Draw box borders step 2
		draw_line(55+BOX_LEN*i, 15, 55+BOX_LEN*i, 225, BLACK);
		draw_line(55, 15+BOX_LEN*i, 265, 15+BOX_LEN*i, BLACK);
	}
	
	draw_rect(56+BOX_LEN*1, 16+BOX_LEN*6, 55+BOX_LEN*2, 15+BOX_LEN*7, GREEN);//green piece start position
	for(int i=1;i<=6;i++){//green home path
	draw_rect(56+BOX_LEN*i, 16+BOX_LEN*7, 55+BOX_LEN*(i+1), 15+BOX_LEN*8, GREEN);
	}
	draw_rect(56+BOX_LEN*8, 16+BOX_LEN*1, 55+BOX_LEN*9, 15+BOX_LEN*2, PINK);//purple piece start position
	for(int i=1;i<=6;i++){//yellow home path
	draw_rect(56+BOX_LEN*7, 16+BOX_LEN*i, 55+BOX_LEN*8, 15+BOX_LEN*(i+1), PINK);
	}
	draw_rect(56+BOX_LEN*13, 16+BOX_LEN*8, 55+BOX_LEN*14, 15+BOX_LEN*9, CYAN);//blue piece start position
	for(int i=8;i<=13;i++){//blue home path
	draw_rect(56+BOX_LEN*i, 16+BOX_LEN*7, 55+BOX_LEN*(i+1), 15+BOX_LEN*8, CYAN);
	}
	draw_rect(56+BOX_LEN*6, 16+BOX_LEN*13, 55+BOX_LEN*7, 15+BOX_LEN*14, YELLOW);//orange piece start position
	for(int i=8;i<=13;i++){//orange home path
	draw_rect(56+BOX_LEN*7, 16+BOX_LEN*i, 55+BOX_LEN*8, 15+BOX_LEN*(i+1), YELLOW);
	}
	//HOME
	draw_rect(56+BOX_LEN*6, 16+BOX_LEN*6, 55+BOX_LEN*9, 15+BOX_LEN*9, 0xc800);//side=box_len*3-1
	//purple piece home
}

void draw_pieces(bool cover, bool select) {
	int c_gr = 9*BOX_LEN, c_sm = 2*BOX_LEN;
	int x,y;
	
	for (int i=0;i<4;i++) {
		for (int j=0;j<4;j++) {
			if (piecePos[i][j]<0) { // Draw initial positions
				x = 55+2*BOX_LEN+((i%2)^(i/2))*c_gr+((j%2)^(j/2))*c_sm;
				// For 0 and 3 x will be small
				y = 15+2*BOX_LEN+(i>>1)*c_gr+(j>>1)*c_sm;
				// For 0 and 1 y will be small
				if (cover) draw_rect(x-BOX_LEN/2+1, y-BOX_LEN/2+1, x+BOX_LEN/2, y+BOX_LEN/2, WHITE);
				else { 
					if (select&&playerTurn==i&&pieceSelect==j) 
						draw_rect(x-BOX_LEN/2+1, y-BOX_LEN/2+1, x+BOX_LEN/2, y+BOX_LEN/2, RED);
					draw_circ(x, y, 5, colorList[i]);
					plot_pixel(x+1,y-3, WHITE);plot_pixel(x+2, y-3, WHITE);plot_pixel(x+3, y-2, WHITE);plot_pixel(x+3, y-1, WHITE);
				}
			}
			else {
				if (piecePos[i][j]>1005) {
					x = 55+7*BOX_LEN+((i%2)^(i/2))*BOX_LEN+((j%2)^(j/2))*(8)-4;
					y = 15+7*BOX_LEN+(i>>1)*BOX_LEN+(j>>1)*(8)-4;
				}
				else if (piecePos[i][j]>1000) {
					if (i==0) {x=55+BOX_LEN/2+BOX_LEN*(piecePos[i][j]-1000); y=15+BOX_LEN/2+BOX_LEN*7;}
					else if (i==2) {x=55+BOX_LEN/2+BOX_LEN*(14-(piecePos[i][j]-1000)); y=15+BOX_LEN/2+BOX_LEN*7;}
					else if (i==1) {x=55+BOX_LEN/2+BOX_LEN*7; y=15+BOX_LEN/2+BOX_LEN*(piecePos[i][j]-1000);}
					else if (i==3) {x=55+BOX_LEN/2+BOX_LEN*7; y=15+BOX_LEN/2+BOX_LEN*(14-(piecePos[i][j]-1000));}
				}
				else if (piecePos[i][j]%13==12) {
					if (piecePos[i][j]%52==12) {x=55+BOX_LEN/2+BOX_LEN*7; y=15+BOX_LEN/2;}
					else if (piecePos[i][j]%52==25) {x=55+BOX_LEN/2+BOX_LEN*14; y=15+BOX_LEN/2+BOX_LEN*7;}
					else if (piecePos[i][j]%52==38) {x=55+BOX_LEN/2+BOX_LEN*7; y=15+BOX_LEN/2+BOX_LEN*14;}
					else if (piecePos[i][j]%52==51) {x=55+BOX_LEN/2; y=15+BOX_LEN/2+BOX_LEN*7;}
				}
				else if (piecePos[i][j]%52<6) {x=55+BOX_LEN/2+(piecePos[i][j]%52%6)*BOX_LEN; y=15+BOX_LEN/2+BOX_LEN*6;}
				else if (piecePos[i][j]%52<12) {x=55+BOX_LEN/2+6*BOX_LEN; y=15+BOX_LEN/2+BOX_LEN*(5-piecePos[i][j]%52%6);}
				else if (piecePos[i][j]%52<19) {x=55+BOX_LEN/2+8*BOX_LEN; y=15+BOX_LEN/2+BOX_LEN*((piecePos[i][j]-1)%52%6);}
				else if (piecePos[i][j]%52<25) {x=55+BOX_LEN/2+(9+(piecePos[i][j]-1)%52%6)*BOX_LEN; y=15+BOX_LEN/2+BOX_LEN*6;}
				else if (piecePos[i][j]%52<32) {x=55+BOX_LEN/2+(14-(piecePos[i][j]-2)%52%6)*BOX_LEN; y=15+BOX_LEN/2+BOX_LEN*8;}
				else if (piecePos[i][j]%52<38) {x=55+BOX_LEN/2+8*BOX_LEN; y=15+BOX_LEN/2+BOX_LEN*(9+(piecePos[i][j]-2)%52%6);}
				else if (piecePos[i][j]%52<45) {x=55+BOX_LEN/2+6*BOX_LEN; y=15+BOX_LEN/2+BOX_LEN*(14-(piecePos[i][j]-3)%52%6);}
				else if (piecePos[i][j]%52<51) {x=55+BOX_LEN/2+(5-(piecePos[i][j]-3)%52%6)*BOX_LEN; y=15+BOX_LEN/2+BOX_LEN*8;}
				else { // Board here
				}
				
				int color = WHITE;
				if (cover) {
					if (piecePos[i][j]>1005) color = 0xc800;
					else if (piecePos[i][j]>1000) color = bgcolorList[i];
					else if (piecePos[i][j]%13==1) color = bgcolorList[piecePos[i][j]/13%4];
					draw_rect(x-BOX_LEN/2+1, y-BOX_LEN/2+1, x+BOX_LEN/2, y+BOX_LEN/2, color);
				}
				else { 
					if (select&&playerTurn==i&&pieceSelect==j) 
						draw_rect(x-BOX_LEN/2+1, y-BOX_LEN/2+1, x+BOX_LEN/2, y+BOX_LEN/2, RED);
					if (piecePos[i][j]>1005) {draw_circ(x, y, 2, colorList[i]);continue;}
					else draw_circ(x, y, 5, colorList[i]);
					plot_pixel(x+1,y-3, WHITE);plot_pixel(x+2, y-3, WHITE);plot_pixel(x+3, y-2, WHITE);plot_pixel(x+3, y-1, WHITE);
				}
			}
		}
	}
}

int diceroll() { // Rolls dice and draws at fixed position; supports up to 7 pips.
	
	//int x0 = (playerTurn%2)*280+10, y0 = ABS(playerTurn%2-1)*200+10;
	
	draw_rect(10,10,30,30,WHITE);
	draw_line(10,10,10,30,BLACK);draw_line(10,30,30,30,BLACK);
	draw_line(30,30,30,10,BLACK);draw_line(30,10,10,10,BLACK);
	
	int val = rand()%8;
	if (!val && (rand()%2)) val = rand()%7+1; // zeros has a 50% chance to be rerolled
	
	hex_update(2, val); // Draws on HEX display
	if (val==0) {draw_circ(20,20,7,RED);draw_circ(20,20,5,WHITE);} // Special case 0
	if (val%2) {draw_circ(20,20,2,BLACK);} // Center pip
	for (int i=1;i*2<=MIN(4, val);i++) { // Pips 2~4
		draw_circ(15, 35-10*i, 2, BLACK);
		draw_circ(25, 5+10*i, 2, BLACK);
	}
	if (val>=6) {draw_circ(15, 20, 2, BLACK);draw_circ(25, 20, 2, BLACK);} // Pips 5&6
	
	return val;
}

bool is_safe(int pos) {
	int safespots[5] = {1,14,27,40};
	for (int i=0;i<4;i++)
		if (safespots[i]==pos) return true;
	
	return false;
}

// HEX update: 	0 - init			1 - player change	2 - dice value change 	3 - selected piece change
//				4,1 - flash "bOnUS"	4,2 - flash "CHEC"
void hex_update(int loc, int val) {
	if (loc==0) {
		*HEX5_HEX4_BASE=0x00007300+hexList[playerTurn];
		*HEX3_HEX0_BASE=0;
	}
	else if (loc==1) *HEX5_HEX4_BASE=0x00007300+hexList[val];
	else if (loc==2) *HEX3_HEX0_BASE=((*HEX3_HEX0_BASE)&0xFFFF)+(hexList[val]<<16);
	else if (loc==3) *HEX3_HEX0_BASE=((*HEX3_HEX0_BASE)&0xFFFF0000)+hexList[val];
	else {
		if (val==1) { // "bOnUS"
			int cur54 = *HEX5_HEX4_BASE, cur30 = *HEX3_HEX0_BASE;
			wait(5);
			for (int i=0;i<3;i++) {
				*HEX5_HEX4_BASE = 0x7c3f; *HEX3_HEX0_BASE=0x373e6d00; *LEDR_BASE = 0x2aa; wait(10); 
				*HEX5_HEX4_BASE = 0; *HEX3_HEX0_BASE=0; *LEDR_BASE = 0x155; wait(10); 
			}
			*LEDR_BASE = 0;
			*HEX5_HEX4_BASE = cur54; *HEX3_HEX0_BASE=cur30;
		}
		if (val==2) { // "CHEC"
			int cur54 = *HEX5_HEX4_BASE, cur30 = *HEX3_HEX0_BASE;
			wait(5);
			for (int i=0;i<3;i++) {
				*HEX5_HEX4_BASE = 0x3976; *HEX3_HEX0_BASE=0x79390000; *LEDR_BASE = 0x3ff; wait(10); 
				*HEX5_HEX4_BASE = 0; *HEX3_HEX0_BASE=0; *LEDR_BASE = 0x0; wait(10); 
			}
			*LEDR_BASE = 0;
			*HEX5_HEX4_BASE = cur54; *HEX3_HEX0_BASE=cur30;
		}
	}
}