#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//function prototypes
void starting_screen();
void difficulty_screen();
void clear_screen();
void clear_char_buffer(int hi_score);
void clear_chars();
void plot_pixel(int x, int y, short int line_color);
void wait_for_vsync();
void light_up_colours(int colour, bool light); //colour (0: green, 1: red, 2: yellow, 3:blue, 4:all), light (0: dim, 1: bright)
void delay_timer(float count);
void display_sequence(int level);
bool check_user_input(int level);
void draw_text(char* text, int x_position, int y_position);
void display_random_sequence(int level);
void moving_boxes();
void light_up_moving_colours(int colour, bool light, int x, int y); 
		
//global variable
volatile int pixel_buffer_start;
volatile int * pixel_ctrl_ptr;
volatile int * edge_capture_ptr;
int colourPressed;
int hi_score;
char difficulty[12]; 

//array to hold sequence
int sequence[50] = {};

void write_char(int x, int y, char c) {
  // VGA character buffer
  volatile char * character_buffer = (char *) (0xC9000000 + (y<<7) + x);
  *character_buffer = c;
}

int main(void)
{
	// set front pixel buffer to start of FPGA On-chip memory
	pixel_ctrl_ptr = (int *) 0xFF203020;
				
	// first store the address in the back buffer
	*(pixel_ctrl_ptr + 1) = 0xC8000000;
	
	// now, swap the front/back buffers, to set the front buffer location
	wait_for_vsync();
	
	// initialize a pointer to the pixel buffer, used by drawing functions
	pixel_buffer_start = *pixel_ctrl_ptr; 
	
	// set back pixel buffer to start of SDRAM memory
	*(pixel_ctrl_ptr + 1) = 0xC0000000;
	
	pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw on the back buffer
	
	//set up the back buffer drawing
	clear_screen();
	light_up_colours(4, 0); //set all lights off
	
	wait_for_vsync(); //swap buffers
	
	//set up the swapped back buffer drawing
	clear_screen();
	light_up_colours(4, 0); //set all lights off
	
	//set up edge capture pointer
	edge_capture_ptr = (int *) 0xFF20005C;
    *edge_capture_ptr = 15;
	
	//default score and difficulty
	int level = 0;
	hi_score = 0;
	strncpy(difficulty, "Normal", 12);
	
	
	//char to be converted from int
	char slevel[1]; 
	char shi_score[1];
	
	clear_char_buffer(hi_score);
	starting_screen();
	clear_char_buffer(hi_score);
	
	//display game mode
	draw_text("Mode: ", 1, 58);
	draw_text(difficulty, 6, 58);
	
	if(strcmp(difficulty, "MovingBoxes") == 0){
		moving_boxes();
	}
		
	while (1){

		draw_text("LEVEL", 37, 13);
		
		itoa(level+1, slevel, 10);
		draw_text(slevel, 43, 13);

		delay_timer(720000);

		if(strcmp(difficulty, "Random") == 0){
			printf("Random\n");
			display_random_sequence(level);
		}
		else{
			display_sequence(level);
		}

		wait_for_vsync();
		//check user input, comparing every button press in sequence so far
		for (int i = 0; i < level+1; i++){
			if (!check_user_input(i)){
                draw_text("Game Over! Press Any KEY to Continue", 23, 15);

				if(level > hi_score){
					//update Hi-Score number
					hi_score = level;
					itoa(hi_score, shi_score, 10);
                    draw_text(shi_score, 77, 1);

                    draw_text("New Hi-Score!", 33, 13);

				}

				//key press to try again
				while(*edge_capture_ptr == 0);
                *edge_capture_ptr = 15;

				level = -1;
                
                //set all colours dim
                wait_for_vsync();
				light_up_colours(4, 0);

				clear_char_buffer(hi_score);
				starting_screen();
				clear_char_buffer(hi_score);
				
				if(strcmp(difficulty, "MovingBoxes") == 0){
					moving_boxes();
				}

			}
			else{
				//increment level if user passed
				if (level == 50){
					return 0;
				}

				wait_for_vsync();

				light_up_colours(colourPressed, 0);

			}
		}

		level++;

		//print score number
		itoa(level, slevel, 10);
		draw_text(slevel, 8, 1);

		wait_for_vsync(); // swap front and back buffers on VGA vertical sync
	}
}

void starting_screen(){

	volatile int *key_ptr = (int *) 0xFF200050;
    volatile int *edge_capture_ptr = (int *) 0xFF20005C;
    //reset edge capture
	*edge_capture_ptr = 15;
    
	//pixel_buffer_start = *(pixel_ctrl_ptr + 1);

	draw_text("SIMON GAME", 35, 13);
	
	//display game mode
	draw_text("Mode: ", 1, 58);
	draw_text(difficulty, 6, 58);

	draw_text("Start", 8, 29);
	
	draw_text("Game Mode", 26, 29);
	
	draw_text("Instructions", 44, 29);
	
	draw_text("Quit", 67, 29);
		
	while(*edge_capture_ptr == 0);	
	int value = *edge_capture_ptr;	
	
	if(value == 8){ //Start
		*edge_capture_ptr = 8;	//reset edge capture point by write a 1 bit into that location
		
		light_up_colours(0, 1); //light up when pressed
		while (*key_ptr != 0); //wait for press to go away
		
		wait_for_vsync(); //display on		
		delay_timer(720000);
		
		wait_for_vsync(); //display off
		light_up_colours(0, 0); 
		

	}
	
	if(value == 4){ //Difficulty				
		
		light_up_colours(1, 1); 
		*edge_capture_ptr = 4;
		while (*key_ptr != 0); //wait for press to go away
		
		wait_for_vsync(); //display on
		delay_timer(720000);
		
		wait_for_vsync();
		light_up_colours(1, 0); //display off
		
		difficulty_screen();
	}
	
	if(value == 2){ //Instructions
		
		*edge_capture_ptr = 2;
		
		light_up_colours(2, 1); //light up when pressed
		while (*key_ptr != 0); //wait for press to go away
		
		wait_for_vsync(); //display on		
		delay_timer(720000);
		
		wait_for_vsync(); //display off
		light_up_colours(2, 0);
		
		pixel_buffer_start = *(pixel_ctrl_ptr + 1);
		clear_screen();
		wait_for_vsync();

		clear_chars();
		
		draw_text("Instructions:", 36, 7);
		draw_text("Once the game starts, a randomly coloured button will be lit up.", 10, 11);
		draw_text("To pass the first level, press the key that corresponds to the", 10, 13);
		draw_text("colour. With each increasing level, another light will added into", 10, 15);	
		draw_text("the sequence, and you will have to repeat these increasing", 10, 17); 
		draw_text("sequences for as long as you can.", 10, 19);
		draw_text("Game Modes", 36, 23); 
		draw_text("Normal: Sequence will be shown as normal", 10, 27);
		draw_text("Hard: Sequence is sped up x3", 10, 29);
		draw_text("Random: Sequence is randomized at every level", 10, 31);
		draw_text("Moving Boxes: The buttons will bounce around the screen", 10, 33);
		draw_text("Press any KEY to return to menu", 10, 39);
		
		while (*key_ptr == 0);  //wait for key press to exit
		while (*key_ptr != 0);  //wait till key is unpressed
		
		clear_char_buffer(hi_score);
				
		wait_for_vsync();
		light_up_colours(4, 0);
		
		starting_screen();

	}	
	if(value == 1){ //Quit
		
		*edge_capture_ptr = 1;
		
		light_up_colours(3, 1); //light up when pressed
		while (*key_ptr != 0); //wait for press to go away
		
		wait_for_vsync(); //display on		
		delay_timer(720000);
		
		pixel_buffer_start = *(pixel_ctrl_ptr + 1);
		
		clear_screen();
		
		//clear character buffer
		clear_chars();

		wait_for_vsync(); //display off
		clear_screen();


		while(1);
	}

	
}

void difficulty_screen(){
	volatile int *key_ptr = (int *) 0xFF200050;
	
	clear_char_buffer(hi_score);
	
	//display game mode 
	draw_text("Mode: ", 1, 58);
	draw_text(difficulty, 6, 58);
	draw_text("Game Mode", 33, 13);
	draw_text("Normal", 8, 29);	
	draw_text("Hard", 28, 29);
	draw_text("Random", 47, 29);
	draw_text("Moving Boxes", 63, 29);
	
	while(*key_ptr == 0);
	int value = *key_ptr;	
	
	if(value == 8){ //Gamemode 1
		strncpy(difficulty, "Normal", 12);
		
		light_up_colours(0, 1); //light up when pressed
		while (*key_ptr != 0); //wait for press to go away
		
		wait_for_vsync(); //display on		
		delay_timer(720000);
		
		wait_for_vsync(); //display off
		light_up_colours(0, 0); 

	}
	
	if(value == 4){ //Gamemode 2
		strncpy(difficulty, "Hard", 12);
		
		light_up_colours(1, 1); //light up when pressed
		while (*key_ptr != 0); //wait for press to go away
		
		wait_for_vsync(); //display on		
		delay_timer(720000);
		
		wait_for_vsync(); //display off
		light_up_colours(1, 0); 

	}
	
	if(value == 2){ //Gamemode 3
		strncpy(difficulty, "Random", 12);
		
		light_up_colours(2, 1); //light up when pressed
		while (*key_ptr != 0); //wait for press to go away
		
		wait_for_vsync(); //display on		
		delay_timer(720000);
		
		wait_for_vsync(); //display off
		light_up_colours(2, 0); 

	}	
	if(value == 1){ //Gamemode 4
		strncpy(difficulty, "MovingBoxes", 12);
		
		light_up_colours(3, 1); //light up when pressed
		while (*key_ptr != 0); //wait for press to go away
		
		wait_for_vsync(); //display on		
		delay_timer(720000);
		
		wait_for_vsync(); //display off
		light_up_colours(3, 0); 
		

	}
	
	clear_char_buffer(hi_score);
	starting_screen();
}

void clear_screen(){
	for(int x=0; x<320; x++){
		for(int y=0; y<240; y++){
			plot_pixel(x, y, 0x0000);
		}
	}
}

void clear_char_buffer(int hi_score){
	
	clear_chars();

	draw_text("Score: 0", 1, 1);
	draw_text("Hi-Score: 0", 67, 1);
	draw_text("KEY3", 9, 40);
	draw_text("KEY2", 28, 40);
	draw_text("KEY1", 48, 40);	
	draw_text("KEY0", 67, 40);
	
	char shi_score[1];
	itoa(hi_score, shi_score, 10);
	draw_text(shi_score, 77, 1);
	
}

void plot_pixel(int x, int y, short int line_color){
	*(short int *)(pixel_buffer_start+(y<<10)+(x<<1)) = line_color;
}

void wait_for_vsync(){
    volatile int *pixel_ctrl_ptr = (int *)0xFF203020;
    volatile int *status =(int *)0xFF20302C;

    *pixel_ctrl_ptr = 1; //request to sync with vga timing

    while((*status & 0x01) != 0){
		status = status; //keep reading status
	}
pixel_buffer_start = *(pixel_ctrl_ptr + 1);

	
    
    return;
}

//colour (0: green, 1: red, 2: yellow, 3:blue, 4:all), light (0: dim, 1: bright)
void light_up_colours(int colour, bool light){
	bool plot = false;
	for(int x=0; x<70; x++){
		for(int y=0; y<70; y++){
			if(y==0||y==69){
				plot = (x>=29 && x<41);
			}
			else if(y==1||y==68){
				plot = (x>=25 && x<45);
			}
			else if(y==2||y==67){
				plot = (x>=22 && x<48);
			}
			else if(y==3||y==66){
				plot = (x>=20 && x<50);
			}
			else if(y==4||y==65){
				plot = (x>=18 && x<52);
			}
			else if(y==5||y==64){
				plot = (x>=16 && x<54);
			}
			else if(y==6||y==63){
				plot = (x>=15 && x<55);
			}
			else if(y==7||y==62){
				plot = (x>=13 && x<57);
			}
			else if(y==8||y==61){
				plot = (x>=12 && x<58);
			}
			else if(y==9||y==60){
				plot = (x>=11 && x<59);
			}
			else if(y==10||y==59){
				plot = (x>=10 && x<60);
			}
			else if(y==11||y==58){
				plot = (x>=9 && x<61);
			}
			else if(y==12||y==57){
				plot = (x>=8 && x<62);
			}
			else if((y>=13 && y<=14) || (y>=55 && y<=56)){
				plot = (x>=7 && x<63);
			}
			else if(y==15||y==54){
				plot = (x>=6 && x<64);
			}
			else if((y>=16 && y<=17) || (y>=52 && y<=53)){
				plot = (x>=5 && x<65);
			}
			else if((y>=18 && y<=19) || (y>=50 && y<=51)){
				plot = (x>=4 && x<66);
			}
			else if((y>=20 && y<=21) || (y>=48 && y<=49)){
				plot = (x>=3 && x<67);
			}
			else if((y>=22 && y<=24) || (y>=45 && y<=47)){
				plot = (x>=2 && x<68);
			}
			else if((y>=25 && y<=28) || (y>=41 && y<=44)){
				plot = (x>=1 && x<69);
			}
			else{
				plot = true;
			}
				
			if(plot){
				
				if(colour == 0 || colour == 4){
					if(light){
						plot_pixel(x+8, y+85, 0x07E0); //bright green
					}
					else{
						plot_pixel(x+8, y+85, 0x00E0); //dark green
					}
				}
				
				if(colour == 1 || colour == 4){
					if(light){
						plot_pixel(x+86, y+85, 0xF800); //bright red
					}
					else{
						plot_pixel(x+86, y+85, 0x2800); //dark red
					}
				}
				
				if(colour == 2 || colour == 4){
					if(light){
						plot_pixel(x+164, y+85, 0xFFE0); //bright yellow 
					}
					else{
						plot_pixel(x+164, y+85, 0x2100); //dark yellow 
					}
				}
				
				if(colour == 3 || colour == 4){
					if(light){
						plot_pixel(x+242, y+85, 0x001F); //bright blue
					}
					else{
						plot_pixel(x+242, y+85, 0x0006); //dark blue
					}
				}
				
				plot = false;
			}
			
			else{
				plot_pixel(x+8, y+85, 0x0200); //bright green
				plot_pixel(x+86, y+85, 0x4000); //bright red
				plot_pixel(x+164, y+85, 0x4200); //bright yellow
				plot_pixel(x+242, y+85, 0x00008); //bright blue
			}
			
		}
	}
	
}

void delay_timer(float count){
	if(count == 1){
		if(strcmp(difficulty, "Hard") == 0){
			count = 100000;
		}
		else{
			count = 300000;
		}
	}
	
	if(count != 1){
		while (count > 0){
			count = count - 1;
		}
	}

}
	
void display_sequence(int level){
	//play sequence that player needs to remember
		//0 = green, 1 = red, 2 = yellow, 3 = blue
		sequence[level] = rand()%4;
		printf("%d\n", level);
		printf("%d\n", sequence[level]);
		int colour;
		for (int i = 0; i < level+1 ; i++){
			if ((sequence[i]) == 0){
				colour = 0;
				light_up_colours(colour, 1);
			}
			else if ((sequence[i]) == 1){
				colour = 1;
				light_up_colours(colour, 1);

			}
			else if ((sequence[i]) == 2){
				colour = 2;
				light_up_colours(colour, 1);
			}
			else{
				colour = 3;
				light_up_colours(colour, 1);
			}
			wait_for_vsync();
			
			delay_timer(1); //depends on difficulty
			
			wait_for_vsync();
			light_up_colours(colour, 0);
			
		}
	return;
}

bool check_user_input(int level){
	
	volatile int *edge_capture_ptr = (int *) 0xFF20005C;
	//reset edge capture bits to make sure buttons pressed before checking input (played during sequence) are ignored
    *edge_capture_ptr = 15;
    
	//wait until key has been pressed
	while(*edge_capture_ptr == 0);
	
	//once key has been pressed
	int value = *edge_capture_ptr;
		
	if(value == 8){
		colourPressed = 0;
	}
	
	if(value == 4){
		colourPressed = 1;
	}
	
	if(value == 2){
		colourPressed = 2;
	}	
	if(value == 1){
		colourPressed = 3;
	}
	//reset it to 0
	*edge_capture_ptr = 15;
	
	light_up_colours(colourPressed, 1); //light up when pressed
	wait_for_vsync();
	
    delay_timer(1); //show what the user pressed	
	
	//check if user input the correct colour
	//store pressed colours into array of user input
	if (colourPressed == sequence[level]){
		return true;
	}
	else{
		return false;
	}
}

void draw_text(char* text, int x_position, int y_position){
		int x = x_position; //39 - half of hw (10ish)
		while (*text) {
			write_char(x, y_position, *text);
			x++;
			text++;
		}
}

void display_random_sequence(int level){
	//play sequence that player needs to remember
	//0 = green, 1 = red, 2 = yellow, 3 = blue
	
	for(int i=0; i<level+1; i++){
		sequence[i] = rand()%4;
	}
	
	printf("%d\n", level);
	printf("%d\n", sequence[level]);
	int colour;
	for (int i = 0; i < level+1 ; i++){
		if ((sequence[i]) == 0){
			colour = 0;
			light_up_colours(colour, 1);
		}
		else if ((sequence[i]) == 1){
			colour = 1;
			light_up_colours(colour, 1);

		}
		else if ((sequence[i]) == 2){
			colour = 2;
			light_up_colours(colour, 1);
		}
		else{
			colour = 3;
			light_up_colours(colour, 1);
		}
		wait_for_vsync();

		delay_timer(1); //depends on difficulty

		wait_for_vsync();
		light_up_colours(colour, 0);

		}
	return;
}

void draw_box(int x, int y, int colour){
	for(int i=x-1; i<x+2+72; i++){
		for(int j=y-1; j<y+2+72; j++){
			plot_pixel(i, j, colour);
		}
	}
}

void moving_boxes(){
	
	volatile int *key_ptr = (int *) 0xFF200050;
	volatile int *edge_capture_ptr = (int *) 0xFF20005C;
	
	//reset edge capture bits
	*edge_capture_ptr = 15;
	
	int level = 0;	
	int colour;
	
	//char to be converted from int
	char slevel[1]; 
	char shi_score[1];
	
	//directions to be randomized
	int dx_box[4], dy_box[4], x_box[4], y_box[4];
	
	for(int i=0; i<4; i++){
		dx_box[i] = rand()%2*2-1; //random number either -1 or 1
		dy_box[i] = rand()%2*2-1;
		y_box[i] = 85;
	}	
	
	//default button starting positions
	x_box[0] = 8;
	x_box[1] = 85;
	x_box[2] = 164;
	x_box[3] = 242;
		
	//pixel_buffer_start = *(pixel_ctrl_ptr + 1);
	
	while (1){
		
		//text for the current level	
		draw_text("LEVEL ", 37, 13);
		
		itoa(level+1, slevel, 10);
		draw_text(slevel, 43, 13);

		delay_timer(1);
	
		sequence[level] = rand()%4;
		
		//play sequence that player needs to remember
		//0 = green, 1 = red, 2 = yellow, 3 = blue
		for (int i = 0; i < level+1 ; i++){
			
			clear_screen();

			//while playing the sequence, move the buttons in the randomized directions
			for(int i=0; i<4; i++){

				//adjust dx, dy when a box "hits" one of the screen edges
				if(x_box[i]==1) dx_box[i] = 1;
				if(x_box[i]==318-72) dx_box[i] = -1;

				if(y_box[i]==1) dy_box[i] = 1;
				if(y_box[i]==238-72) dy_box[i] = -1;

				light_up_moving_colours(i, 0, x_box[i], y_box[i]);

				x_box[i]+= dx_box[i];
				y_box[i]+= dy_box[i];

			}
			
			if ((sequence[i]) == 0){
				colour = 0;
				light_up_moving_colours(colour, 1, x_box[colour], y_box[colour]);
			}
			else if ((sequence[i]) == 1){
				colour = 1;
				light_up_moving_colours(colour, 1, x_box[colour], y_box[colour]);
			}
			else if ((sequence[i]) == 2){
				colour = 2;
				light_up_moving_colours(colour, 1, x_box[colour], y_box[colour]);
			}
			else{
				colour = 3;
				light_up_moving_colours(colour, 1, x_box[colour], y_box[colour]);
			}
			
			wait_for_vsync();
			
			//first buffer
			clear_screen();
			//while playing the sequence, move the buttons in the randomized directions
			for(int i=0; i<4; i++){

				//adjust dx, dy when a box "hits" one of the screen edges
				if(x_box[i]==1) dx_box[i] = 1;
				if(x_box[i]==318-72) dx_box[i] = -1;

				if(y_box[i]==1) dy_box[i] = 1;
				if(y_box[i]==238-72) dy_box[i] = -1;

				light_up_moving_colours(i, 0, x_box[i], y_box[i]);

				x_box[i]+= dx_box[i];
				y_box[i]+= dy_box[i];

			}
			
			wait_for_vsync();
			
			//2nd buffer
			clear_screen();
			//while playing the sequence, move the buttons in the randomized directions
			for(int i=0; i<4; i++){

				//adjust dx, dy when a box "hits" one of the screen edges
				if(x_box[i]==1) dx_box[i] = 1;
				if(x_box[i]==318-72) dx_box[i] = -1;

				if(y_box[i]==1) dy_box[i] = 1;
				if(y_box[i]==238-72) dy_box[i] = -1;

				light_up_moving_colours(i, 0, x_box[i], y_box[i]);

				x_box[i]+= dx_box[i];
				y_box[i]+= dy_box[i];

			}

			wait_for_vsync(); // swap front and back buffers on VGA vertical sync
			
		}
		
		wait_for_vsync();
		
		//check user input, comparing every button press in sequence so far
		for (int i = 0; i < level+1; i++){
			
			//wait until key has been pressed
			while(*edge_capture_ptr == 0){
				
				//while waiting, animate the moving buttons
				clear_screen();

				for(int i=0; i<4; i++){

					//adjust dx, dy when a box "hits" one of the screen edges
					if(x_box[i]==1) dx_box[i] = 1;
					if(x_box[i]==318-72) dx_box[i] = -1;

					if(y_box[i]==1) dy_box[i] = 1;
					if(y_box[i]==238-72) dy_box[i] = -1;

					light_up_moving_colours(i, 0, x_box[i], y_box[i]);

					x_box[i]+= dx_box[i];
					y_box[i]+= dy_box[i];

				}
				
				wait_for_vsync(); // swap front and back buffers on VGA vertical sync
				
			}
			//once key has been pressed
			int value = *edge_capture_ptr;

			if(value == 8){
				colourPressed = 0;
			}
			if(value == 4){
				colourPressed = 1;
			}
			if(value == 2){
				colourPressed = 2;
			}	
			if(value == 1){
				colourPressed = 3;
			}
			
			*edge_capture_ptr = 15;
			
			
			clear_screen();
			//update directions as buttons are pressed as well, move the buttons in the randomized directions
			for(int i=0; i<4; i++){

				//adjust dx, dy when a box "hits" one of the screen edges
				if(x_box[i]==1) dx_box[i] = 1;
				if(x_box[i]==318-72) dx_box[i] = -1;

				if(y_box[i]==1) dy_box[i] = 1;
				if(y_box[i]==238-72) dy_box[i] = -1;

				light_up_moving_colours(i, 0, x_box[i], y_box[i]);

				x_box[i]+= dx_box[i];
				y_box[i]+= dy_box[i];

			}
			
			
			light_up_moving_colours(colourPressed, 1, x_box[colourPressed], y_box[colourPressed]); //light up when pressed
			wait_for_vsync();

			delay_timer(720000); //show what the user pressed

			//check if user input the correct colour
			//store pressed colours into array of user input
			if (colourPressed == sequence[i]){
				//increment level if user passed
				if (level == 50){
					while(1); //return
				}
				
				//2nd buffer
				clear_screen();
				//while playing the sequence, move the buttons in the randomized directions
				for(int i=0; i<4; i++){

					//adjust dx, dy when a box "hits" one of the screen edges
					if(x_box[i]==1) dx_box[i] = 1;
					if(x_box[i]==318-72) dx_box[i] = -1;

					if(y_box[i]==1) dy_box[i] = 1;
					if(y_box[i]==238-72) dy_box[i] = -1;

					light_up_moving_colours(i, 0, x_box[i], y_box[i]);

					x_box[i]+= dx_box[i];
					y_box[i]+= dy_box[i];

				}
				

				wait_for_vsync();
				
				clear_screen();
				light_up_moving_colours(0, 0, x_box[0], y_box[0]);
				light_up_moving_colours(1, 0, x_box[1], y_box[1]);
				light_up_moving_colours(2, 0, x_box[2], y_box[2]);
				light_up_moving_colours(3, 0, x_box[3], y_box[3]);

			}
			else{
            	draw_text("Game Over! Press Any KEY to Continue", 23, 15);			            
                                
				if(level > hi_score){
					//update Hi-Score number
					hi_score = level;
                    
					itoa(hi_score, shi_score, 10);
                    draw_text(shi_score, 77, 1);
					
                    draw_text ("New Hi-Score!", 33, 13);                    
				}

				//key press to try again
				while(*key_ptr == 0);

				level = -1;
				
				clear_screen();
				light_up_colours(4, 0);

				//wait until key is lifted
				while(*key_ptr != 0);

				wait_for_vsync();
				
				clear_screen();
				light_up_colours(4, 0);

				clear_char_buffer(hi_score);
				starting_screen();
				clear_char_buffer(hi_score);

				//if still in moving boxes mode, continue, else return
				if(strcmp(difficulty, "MovingBoxes") == 0){
					moving_boxes();
				}
				
				return;
			}
		}

		level++;

		//print score number		
		itoa(level, slevel, 10);
		draw_text(slevel, 8, 1);

		wait_for_vsync(); // swap front and back buffers on VGA vertical sync
    }
}

//light up/dim colours at a certain location
void light_up_moving_colours(int colour, bool light, int a, int b){
	
	bool plot = false;
	for(int x=0; x<70; x++){
		for(int y=0; y<70; y++){
			if(y==0||y==69){
				plot = (x>=29 && x<41);
			}
			else if(y==1||y==68){
				plot = (x>=25 && x<45);
			}
			else if(y==2||y==67){
				plot = (x>=22 && x<48);
			}
			else if(y==3||y==66){
				plot = (x>=20 && x<50);
			}
			else if(y==4||y==65){
				plot = (x>=18 && x<52);
			}
			else if(y==5||y==64){
				plot = (x>=16 && x<54);
			}
			else if(y==6||y==63){
				plot = (x>=15 && x<55);
			}
			else if(y==7||y==62){
				plot = (x>=13 && x<57);
			}
			else if(y==8||y==61){
				plot = (x>=12 && x<58);
			}
			else if(y==9||y==60){
				plot = (x>=11 && x<59);
			}
			else if(y==10||y==59){
				plot = (x>=10 && x<60);
			}
			else if(y==11||y==58){
				plot = (x>=9 && x<61);
			}
			else if(y==12||y==57){
				plot = (x>=8 && x<62);
			}
			else if((y>=13 && y<=14) || (y>=55 && y<=56)){
				plot = (x>=7 && x<63);
			}
			else if(y==15||y==54){
				plot = (x>=6 && x<64);
			}
			else if((y>=16 && y<=17) || (y>=52 && y<=53)){
				plot = (x>=5 && x<65);
			}
			else if((y>=18 && y<=19) || (y>=50 && y<=51)){
				plot = (x>=4 && x<66);
			}
			else if((y>=20 && y<=21) || (y>=48 && y<=49)){
				plot = (x>=3 && x<67);
			}
			else if((y>=22 && y<=24) || (y>=45 && y<=47)){
				plot = (x>=2 && x<68);
			}
			else if((y>=25 && y<=28) || (y>=41 && y<=44)){
				plot = (x>=1 && x<69);
			}
			else{
				plot = true;
			}
				
			if(plot){
				
				if(colour == 0){
					if(light){
						plot_pixel(x+a, y+b, 0x07E0); //bright green
					}
					else{
						plot_pixel(x+a, y+b, 0x00E0); //dark green
					}
				}
				
				if(colour == 1){
					if(light){
						plot_pixel(x+a, y+b, 0xF800); //bright red
					}
					else{
						plot_pixel(x+a, y+b, 0x2800); //dark red
					}
				}
				
				if(colour == 2){
					if(light){
						plot_pixel(x+a, y+b, 0xFFE0); //bright yellow 
					}
					else{
						plot_pixel(x+a, y+b, 0x2100); //dark yellow 
					}
				}
				
				if(colour == 3){
					if(light){
						plot_pixel(x+a, y+b, 0x001F); //bright blue
					}
					else{
						plot_pixel(x+a, y+b, 0x0006); //dark blue
					}
				}
				
				plot = false;
			}
			
			else{
				
				plot_pixel(x+8, y+85, 0x0200); //bright green
				plot_pixel(x+86, y+85, 0x4000); //bright red
				plot_pixel(x+164, y+85, 0x4200); //bright yellow
				plot_pixel(x+242, y+85, 0x00008); //bright blue
				
			}
			
		}
	}
}

void clear_chars(){
	
	//clear character buffer
	for(int x=0; x<80; x++){
		for(int y=0; y<60; y++){
			char* clear = " ";
			while (*clear) {
				write_char(x, y, *clear);
				clear++;
			}
		}	
	}
}
