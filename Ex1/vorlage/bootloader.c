#ifndef _CODE16GCC_H_
#define _CODE16GCC_H_
asm(".code16gcc\n");
#endif

/* needs to stay the first line */
asm("jmp $0, $main");

/* space for additional code */

int printstring(char* word) { //Print String to console
	asm(
	"mov $0x0E, %%ah;"
	"loop:"
		"lodsb;"
		"cmp $0, %%al;"
		"je done;"
		"int $0x10;"
		"jmp loop;"
 	"done:"
		"mov $0x0D, %%al;"
		"int $0x10;"
	::    "S"(word)
	);
	return 0;
}

//korrekte Interrupt Code müsste hier noch rausgesucht werden und dann an AH (code) AL (wert) an den Interrupt übergeben werden
void reboot(){
	printstring("Reboot!\n");
	asm volatile(
		"int $0x19;"
	);
}

void input(){
	//char* tmp= "";
	char string[7];
	asm("mov $0 ,%edx;");  //reset counter
	for (int i = 0; i<=7;i++){
		asm(
				"mov $0x00, %%ah;" 	//command read Keyboard Char input
				"int $0x16;"  		//call keyboard service 16h
				"cmp $0x0D, %%al;"	//check for ENTER
				"je enter;"
				"cmp $8 , %%edx;"	//max 8 chars
				"je enter;"
				"mov %%al, %0;"		//move acsci to output =a -> der Wert von AL scheint selbst kein ASCII Zeichen zu sein, nicht so wie Wiki dies angegeben hatte für 16h 00, müsste man wahrscheinlich einmal konvertieren
   				"mov $0x0E, %%ah;"	//change mode to display
 				"mov $'.', %%al;"	//switch to dot
				"int $0x10;"		//call display service 10h
				"jmp next;"			//quit step
			"enter:"				//pressed enter or while char count 8
				"mov $8, %%edx;"
				"jmp end;"
			"next:"
			"end:"
 			: "=r"(string[i]),"=d"(i):"d"(i)
		);
		//string[i] = tmp;
	};

	// Insert a Linebreak
	asm (
		"mov $0x0E, %ah;"
		"mov $0x0D, %al;"
		"int $0x10;"
	);


// Schleife läuft noch in endlosschleife, da werte im Char-Array string nicht korrekt sind, sonst sollte es dann aber klappen
//	for (int j = 0; j<=6;j++){
		asm(
			"mov $0x0E,%%ah;"
			"mov %0, %%al;"
			"int $0x10;"
			::"r"(string[0])
		);
//	};

	// Insert a Linebreak
	asm (
		"mov $0x0E, %ah;"
		"mov $0x0D, %al;"
		"int $0x10;"
	);


	// Muss noch implementiert werden, bei einem leerem String zu rebooten, oder nach einem Enter
	//if (string[i] == "") { reboot(); }

    reboot();
}

	void main(void)
{
	printstring("Hello World!\n");
	input();
	asm("jmp .");

}
