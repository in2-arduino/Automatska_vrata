/*
 * Your reader should have at least 4 connections (some readers have more).  Connect the Red wire 
 * to 5V.  Connect the black to ground.  Connect the green wire (DATA0) to Digital Pin 2 (INT0).  
 * Connect the white wire (DATA1) to Digital Pin 3 (INT1).  That's it!
*/
// unlock, wait 60s, if dor closed lock
// unlock, door open, door close, wait 1s and lock
// unlock, door open, wait 60s and sound if not closed

#include <EEPROM.h>
#include "Wiegand.h"
#include "HIDCardRader.h"
#include "Door.h"
#include "U8glib.h"
#include "Resources.h"

#define WIEGAND_DATA_0 2			// Wiegand lines
#define WIEGAND_DATA_1 3

									// motor control pins, connect to L298N
#define LEFT_MOTOR_A_PIN 4			// pin IN1
#define LEFT_MOTOR_B_PIN 5			// pin IN2
#define RIGHT_MOTOR_A_PIN 7			// pin IN3
#define RIGHT_MOTOR_B_PIN 8			// pin IN4
#define MOTOR_PWM_PIN 6				// pin ENA & ENB, must be hardware PWM enabled

#define LEFT_DOOR_SENSOR_PIN A0		// pinovi za senzore vrata
#define RIGHT_DOOR_SENSOR_PIN A1

#define SPEAKER_PIN A2				// pin za kontrolu speakera u čitaču kartica

#define EEPROM_CARD_COUNT_ADDRESS 0		// address of EEPROM byte used to store number of card codes stored in EEPROM

// instantiate global Wiegand protocol communication class using pins 2 for Data0 and 3 for Data1
Wiegand wiegand(WIEGAND_DATA_0, WIEGAND_DATA_1);

// instantiate door objects
Door left_door("Left", LEFT_DOOR_SENSOR_PIN, LEFT_MOTOR_A_PIN, LEFT_MOTOR_B_PIN, MOTOR_PWM_PIN);
Door right_door("Right", RIGHT_DOOR_SENSOR_PIN, RIGHT_MOTOR_A_PIN, RIGHT_MOTOR_B_PIN, MOTOR_PWM_PIN);

// instantiate OLED driver
U8GLIB_SSD1306_128X64 u8g(10, 9);		// HW SPI Com: CS = 10, A0 = 9 (Hardware Pins are  SCK = 13 and MOSI = 11)
unsigned long msg_timeout = 0;
char * msg_text;

byte buffer[5];
byte master[4] = {TIPKA4, TIPKA3, TIPKA2, TIPKA1};


void setup()
{
	pinMode(LED_BUILTIN, OUTPUT);		// LED  
	pinMode(SPEAKER_PIN, OUTPUT);		// Speaker

	digitalWrite(LED_BUILTIN, LOW);		// LED off
	digitalWrite(SPEAKER_PIN, HIGH);	// Speaker off

	u8g.setColorIndex(1);				// OLED BW mode

	Serial.begin(115200);

	Serial.println("Starting in...");
	for(byte to = 0; to < 3; to++)
	{
		Serial.println(to);
		//delay(1000);
	}

	Serial.println("Automatska vrata v5");
	Serial.print(EEPROM.read(EEPROM_CARD_COUNT_ADDRESS));
	Serial.println(" cards stored in EEPROM");

	// Initialize doors
	left_door.begin();
	right_door.begin();

	left_door.print();
	right_door.print();
  
	buffer[0] = 0;

	if (wiegand.begin()) 
		Serial.println("Wiegand init succesfull");
	else
		Serial.println("Wiegand init failed");

}

inline static void setMessage(char * text, unsigned long timeout)
{
	msg_timeout = millis() + timeout;
	msg_text = text;
}

void refreshDisplay()
{
	// picture loop
	u8g.firstPage();  
	do
	{
		u8g.setColorIndex(1);

		// graphic commands to redraw the complete screen should be placed here  
		u8g.setFont(u8g_font_unifont);
		//u8g.setFont(u8g_font_osb21);
		u8g.drawStr(0, 10, "Automatska vrata");
		u8g.drawLine(0, 13, 128, 13);
		u8g.setFont(u8g_font_helvR08);
		u8g.drawStr(0, 26,"v1.0        IN2 Arduino 2014");

		if (left_door.status == Door::Locked)
			u8g.drawXBMP( 16, 32, 32, 32, lock_bitmap);
		else if (left_door.status == Door::Closed)
			u8g.drawXBMP( 16, 32, 32, 32, unlock_bitmap);
		else if (left_door.status == Door::Open)
			u8g.drawXBMP( 16, 32, 32, 32, door_bitmap);

		if (right_door.status == Door::Locked)
			u8g.drawXBMP( 80, 32, 32, 32, lock_bitmap);
		else if (right_door.status == Door::Closed)
			u8g.drawXBMP( 80, 32, 32, 32, unlock_bitmap);
		else if (right_door.status == Door::Open)
			u8g.drawXBMP( 80, 32, 32, 32, door_bitmap);


		if (millis() < msg_timeout)
		{
			u8g.setFont(u8g_font_unifont);
			int h = u8g.getFontAscent()-u8g.getFontDescent();
			int w = u8g.getStrWidth(msg_text);
			int start_w = (128 - w) / 2;
			int start_h = 32 + (32 - h) / 2;		
			u8g.setColorIndex(0);
			u8g.drawRBox(start_w - 4, start_h - 3, w + 8, h + 6, 2) ;
			u8g.setColorIndex(1);
			u8g.drawRBox(start_w - 3, start_h - 2, w + 6, h + 4, 2) ;
			u8g.setColorIndex(0);
			u8g.drawRBox(start_w - 2, start_h - 1, w + 4, h + 2, 2) ;
			u8g.setColorIndex(1);
			u8g.drawStr(start_w, start_h + u8g.getFontAscent(), msg_text);
		}

	} while( u8g.nextPage() );
}

void loop()
{

	// We need to check that we have some actual data to proccess
	if(wiegand.finishRead())
	{
		Serial.println("");
		wiegand.print();


		switch(wiegand.bit_count)
		{
			case 36 :      
     
				if (buffer[0] == 5)
				{
					learnCode();
					printCards();
					buffer[0] = 0;
				}
     
				if (checkCard())
				{   
					for(int i = 0; i<=255;i++)
						analogWrite(6,i);delay(5);
			
					left_door.unlock();
					right_door.unlock();
				}
    
				break;

			case 8 : 
				
				switch(wiegand.rcv_buffer[0])
				{
					case TIPKA1: 
						Serial.println("Tipka 1");
						//left_door.print();
						//right_door.print();
						setMessage("Tipka 1", 500);
						break;
					case TIPKA2: 
						Serial.println("Tipka 2");      
						//   digitalWrite(6, !digitalRead(6)); 
						setMessage("Tipka 2", 500);
						break;
					case TIPKA3: 
						Serial.println("Tipka 3");      
						// activating speaker sends some data to controller which we want to ignore
						//wiegand.suspend();
						//digitalWrite(SPEAKER_PIN, LOW);
						//delay(1000);
						//digitalWrite(SPEAKER_PIN, HIGH);
						//wiegand.resume();
						setMessage("Tipka 3", 500);
						break;
					case TIPKA4: 
						Serial.println("Tipka 4");
						//left_door.unlock();
						setMessage("Tipka 4", 500);
						break;
					case TIPKA5: 
						Serial.println("Tipka 5");
						left_door.lock();
						setMessage("Tipka 5", 500);
						break;
					case TIPKA6: 
						Serial.println("Tipka 6");
						setMessage("Tipka 6", 500);
						break;
					case TIPKA7: 
						Serial.println("Tipka 7");
						right_door.unlock();
						setMessage("Tipka 7", 500);
						break;
					case TIPKA8: 
						Serial.println("Tipka 8");
						right_door.lock();
						setMessage("Tipka 8", 500);
						break;
					case TIPKA9: 
						Serial.println("Tipka 9");
						setMessage("Tipka 9", 500);
						break;
					case TIPKA0: 
						Serial.println("Tipka 0");
						setMessage("Tipka 0", 500);
						break;
					case TIPKA_STAR: 
						Serial.println("Tipka *");
						setMessage("Tipka *", 500);
						break;
					case TIPKA_HASH: 
						Serial.println("Tipka #");
						setMessage("Tipka #", 500);
						break;
				}

				if (wiegand.rcv_buffer[0] == TIPKA_STAR)
					buffer[0] = 0;
				else if (buffer[0] == 0 && wiegand.rcv_buffer[0] == master[buffer[0]])
					buffer[++buffer[0]] = wiegand.rcv_buffer[0];
				else if (buffer[0] == 1 && wiegand.rcv_buffer[0] == master[buffer[0]])
					buffer[++buffer[0]] = wiegand.rcv_buffer[0];
				else if (buffer[0] == 2 && wiegand.rcv_buffer[0] == master[buffer[0]])
					buffer[++buffer[0]] = wiegand.rcv_buffer[0];
				else if (buffer[0] == 3 && wiegand.rcv_buffer[0] == master[buffer[0]])
					buffer[++buffer[0]] = wiegand.rcv_buffer[0];
				else if (buffer[0] == 4 && wiegand.rcv_buffer[0] == TIPKA_HASH)
				{
					Serial.println("Spreman za čitanje kartice");
					buffer[0] = 5;    
				}
				else
					buffer[0] = 0;

				break;

			default :
				Serial.println("Kartica s nedozvoljenim brojem bita");
				setMessage("Kartica nepodržana", 1000);
				break;  
		}

		// clean up and get ready for the next card
		wiegand.clear();

	}
	else if (wiegand.status == Wiegand::Error)
	{
		Serial.println("");
		wiegand.print();
		wiegand.clear();
	}

	// we must poll doors
	switch (left_door.check())
	{
		case CHECK_ALERT:
			setMessage("Zatvorite vrata", 1000);
			refreshDisplay();
			wiegand.suspend();
			digitalWrite(SPEAKER_PIN, LOW);
			delay(1000);
			digitalWrite(SPEAKER_PIN, HIGH);
			delay(1000);
			wiegand.resume();
			break;
		case CHECK_ALARM:
			setMessage("!!!PROVALA!!!", 1000);
			refreshDisplay();
			wiegand.suspend();
			digitalWrite(SPEAKER_PIN, LOW);
			delay(1000);
			digitalWrite(SPEAKER_PIN, HIGH);
			wiegand.resume();
			break;
	}
	switch(right_door.check())
	{
		case CHECK_ALERT:
			setMessage("Zatvorite vrata", 1000);
			refreshDisplay();
			wiegand.suspend();
			digitalWrite(SPEAKER_PIN, LOW);
			delay(1000);
			digitalWrite(SPEAKER_PIN, HIGH);
			delay(1000);
			wiegand.resume();
			break;
		case CHECK_ALARM:
			setMessage("!!!PROVALA!!!", 1000);
			refreshDisplay();
			wiegand.suspend();
			digitalWrite(SPEAKER_PIN, LOW);
			delay(1000);
			digitalWrite(SPEAKER_PIN, HIGH);
			wiegand.resume();
			break;
	}

	if (buffer[0] == 5)
		setMessage("Nova kartica", 250);

	refreshDisplay();
}

void learnCode (){
	byte cardCount = EEPROM.read(EEPROM_CARD_COUNT_ADDRESS);
	int nextAddress = cardCount * WIEGAND_MAX_BYTES + 1;
  
  	for (byte i = 0; i < WIEGAND_MAX_BYTES; i++)
		EEPROM.write(nextAddress++, wiegand.rcv_buffer[i]);

	EEPROM.write(EEPROM_CARD_COUNT_ADDRESS, ++cardCount);

	setMessage("Kartica dodana", 1000);
}


void printCards()
{
	byte cardCount = EEPROM.read(EEPROM_CARD_COUNT_ADDRESS);
	Serial.print("Dumping cards, total count = ");
	Serial.println(cardCount);

	for(int c = 0; c < cardCount; c++)
	{
		int cardAddress = c * WIEGAND_MAX_BYTES + 1;
		Serial.print("  card no ");
		Serial.print(c);
		Serial.print(" = {");
		
		for (int8_t offset = WIEGAND_MAX_BYTES - 1; offset >= 0 ; offset--)
		{
			Serial.print(EEPROM.read(cardAddress + offset), HEX);
			if (offset > 0)
				Serial.print(", ");
		}

		Serial.println("}");
	}
}


bool checkCard()
{
	bool cardFound = false;
	byte cardCount = EEPROM.read(EEPROM_CARD_COUNT_ADDRESS);
	Serial.print("Checking cards, total count = ");
	Serial.println(cardCount);

	for(int c=0;c<cardCount;c++)
	{
		int cardAddress = c * WIEGAND_MAX_BYTES + 1;
		Serial.print("  card ");Serial.println(c);
		for (int offset=0; offset <WIEGAND_MAX_BYTES; offset++)
		{
			if (EEPROM.read(cardAddress + offset)!= wiegand.rcv_buffer[offset])
			break;
			else if (offset == WIEGAND_MAX_BYTES - 1)
			{
				Serial.println("Access granted"); 
				setMessage("Pristup odobren", 1000);
				refreshDisplay();
				return true;
			}
		}
	}

	Serial.println("Invalid badge"); 
	setMessage("Pristup odbijen", 1000);
	return false;
}

