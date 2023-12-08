
float PERSISTENCE_TIME=0.05;  //0.02;  // setting persistence to 2 seconds
int Y_OFFSET=10;

int TRIGGER_LEVEL = 25;  // this is going by pixel, for now.  will be V+ in the future
//char* TRIGGER_MODE = "rising"; // the other options are falling and auto
char* TRIGGER_MODE = "auto"; // the other options are falling and rising
boolean DISPLAY_TRIGGERS=false;

int averaging_cycles=2;  // how many cycles to calculate averages, min, and max from.  effects auto trigger, for now


#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

/*
  the actual next step with this code, other than cleaning it the fuck up,
  is to extend the buffer.  right now, i'm varying the sample rate.
  what i need to be doing is sampling at a fixed rate and varying the 
  scale of the x-axis
  
  
*/

int NumPixels=SCREEN_WIDTH*SCREEN_HEIGHT;  // not used

//  uint8_t  =  1 byte unsigned integer
uint8_t* pixels[SCREEN_WIDTH][SCREEN_HEIGHT];  //not used

//  Teensy 4.1 has 1MB very fast ram

int val=0;  //analog pin value.  updates in the loop()

//float PERSISTENCE_TIME=0.02;  // setting persistence to 2 seconds

// 1000 micros = 1 milli
// 1000 millis = 1 s

int SAMPLE_RATE=1.0/100;  //micros()
int SAMPLE_FREQUENCY=10000;  //hz  ->  1.0/SAMPLE_RATE;
int LOOP_TIME = 26.5;





uint8_t buff[256];
//uint8_t* last_buffer[128] = &buff;
//uint8_t* new_buffer[128] = &buff+128;


int cycle=0;
int persist_cycles = (int)(PERSISTENCE_TIME*1000)/LOOP_TIME;


Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);





/*
Working Teensy 3.2 example code.  for the i2c lcd displays i got on amazon
pins:
  GND teensy -> GND lcd
  5v teensy -> VCC lcd
  19 teensy -> SCL lcd
  18 teensy -> SCA lcd  
 */

void setup() {
  Wire.setClock(400000);

  if(TRIGGER_MODE=="auto"){
    TRIGGER_LEVEL=128;
  }
  Serial.begin(115200);
  pinMode(A0, INPUT);
  pinMode(13, OUTPUT);  
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    for(;;);
  }
  display.display();
  delay(1000); // Pause for 2 seconds
  display.clearDisplay();
  
//Serial.println(SSD1306_WHITE);
//Serial.println(SSD1306_BLACK);

}

void writePixel(int16_t x, int16_t y){
  display.drawPixel(x, y, SSD1306_WHITE);
}

int getSampleRate(int Frequency){  // Frequency in Hz, SampleRate in Milliseconds
  return((int) (1000*Frequency));
}

int getFrequency(int SampleRate){  // Frequency in Hz, SampleRate in Milliseconds
  return((int) (SampleRate/1000));  
}


void sample(uint8_t* buff){
  SAMPLE_RATE = getSampleRate(SAMPLE_FREQUENCY);
  Serial.print("Delay Between Samples: ");
  Serial.println(SAMPLE_RATE);

  // shift
  /*
  for(int x=256;x>128; x=x-1){
    buff[x-128]=buff[x];
   Serial.println(buff[x-128]);
    Serial.println(buff[x]);

  }
*/  

  for(int x=0; x<256; x=x+1){
    buff[x] = 64 - (analogRead(A0) * 0.06256109481915934);  // scaled and all that
    Serial.println(buff[x]);
    delayMicroseconds(SAMPLE_RATE);
  }

}

int rate = 1000;
int distance = 0;
int old_trigger_hits[16];
int old_trigger_hit_index=0;
int avg=0;
int avg_count=0;
int max_read=0;
int min_read=0;


void loop(){
  if(avg_count>averaging_cycles){
    avg=0;
    max_read=0;
    min_read=128;
    avg_count=0;
  }
  
  //at the end of averaging_cycles number of reads, adjust auto trigger if auto mode is on
  Serial.print("TRIGGER_LEVEL:   " );
  Serial.println(TRIGGER_LEVEL);
  Serial.print("max voltage:   " );
  Serial.println(max_read);
  Serial.print("min voltage:   " );
  Serial.println(min_read);

  if(TRIGGER_MODE=="auto"){
    if(avg_count==averaging_cycles){
      TRIGGER_LEVEL=(int)(min_read + max_read)/2;
      Serial.print("trigger level:   " );
      Serial.println(TRIGGER_LEVEL);
    }
  }
  
  //display.clearDisplay();
  //Serial.print("sample rate for 100hz:   ");
  //Serial.println(getSampleRate(100)); 
  //Serial.print("frequency Hz for sample rate of 100 microseconds:  ");
  //Serial.println(getFrequency(100)); 

  //one full loop takes 26.5 ms
  // cycles to persist = ( persistence time (seconds) X 1000)/26.5
  if(PERSISTENCE_TIME==-1){
    display.clearDisplay();
  }
  else{
    if(cycle>=persist_cycles){
      display.clearDisplay();
      cycle=0;
    }
  }
  //sample(buff);      

  int trigger_hit_index = 0;
  int trigger_hits[16];  // assuming we wont exceed 16 hits per read cycle

  for(int x=0; x<16; x=x+1){
    trigger_hits[x]=-1;  // -1 is invalid
  }

  bool triggered = false;
  
  for(int x=0; x<128; x=x+1){    
    //buff[x+128]=buff[x]; // copy data further into the buffer
    
    buff[x] = 64 - (analogRead(A0) * 0.06256109481915934) + (-1 * Y_OFFSET) ;  // scaled and all that

    // clamp to outputtable range  
    if(buff[x]>SCREEN_HEIGHT){
      buff[x]=SCREEN_HEIGHT;
    }
    else if(buff[x]<0){
      buff[x]=0;
    }

    // should start averaging these to deal with spikes
    if(buff[x]>=max_read){
      max_read=buff[x];
    }
    // !=0 is important, it makes sure it's an actual signal
    if(buff[x]<=min_read && buff[x]>0){
      min_read=buff[x];
    }
    

  
    //catch triggers
    if( (buff[x-1] < TRIGGER_LEVEL) && (buff[x]>=TRIGGER_LEVEL) && (trigger_hit_index<16)){
      triggered=true;
      trigger_hits[trigger_hit_index] = x;

      if(trigger_hit_index>0){  // changed from 1

        if(trigger_hits[trigger_hit_index]<old_trigger_hits[trigger_hit_index]){   
          distance=abs(old_trigger_hits[trigger_hit_index] - trigger_hits[trigger_hit_index]);        
          //int adjust = (int)rate*(distance/2);
        }else{
          distance = abs(distance=trigger_hits[trigger_hit_index] - trigger_hits[trigger_hit_index-1]);
        }
        
        //int adjust = (int)rate*distance;
        int adjust = (int)rate*(distance/2);
        
        //Serial.print("adjust:  ");
        //Serial.println(adjust);
        //delayMicroseconds(adjust);
      }

      

      trigger_hit_index = trigger_hit_index+1;      
    }

    delayMicroseconds(rate);

    //delayMicroseconds(SAMPLE_RATE);

    if(triggered==true || TRIGGER_MODE=="auto"){
      writePixel(x, buff[x]);
    }
    //basically, don't start until trigger happens.
    //  ensures that a trigger is always in the middle 
    
//    if( triggered==false && x==0 && TRIGGER_MODE!="auto"){
//      x=x-1;  
//    }

    if( triggered==false && x==0){
      if(TRIGGER_MODE=="auto"){
        if(max_read==0 && min_read==128){
          x=x-1; 
        }
      }else{
        x=x-1;
      }
      
    }

    
  }
  //int num_triggers = trigger_hit_index;
  // draw lines where triggers occurred

int freq = 0;
int new_rate = 0;

if(trigger_hit_index>5){
  rate=rate-(rate/4);
  
}
//if(trigger_hit_index>3 && old_trigger_hit_index==3){
//  //donothing....  sloppy
//}else if(trigger_hit_index>3){
//  rate=rate-2;
//}

if(trigger_hit_index>3){
  if(old_trigger_hit_index!=3){
    rate=rate-2;
  }
}

if(trigger_hit_index==1){
  rate=rate+10;
}

// make trigger mode auto
if(trigger_hit_index==0){
  rate=(rate+1)*4;
  
  
}


if(trigger_hit_index<3){
  if(old_trigger_hit_index!=3){
    rate=rate+2;
  }
  
}


if(rate<=0){
  rate=1;
}
if(rate>10000){
  rate=10000;
}

//Serial.println(rate);
  
  for(int x=0; x<trigger_hit_index; x=x+1){

    if(x>0){
      distance = trigger_hits[x]-trigger_hits[x-1];
    }
    if(DISPLAY_TRIGGERS==true){
      display.drawFastVLine(trigger_hits[x], 0, 64, (SSD1306_WHITE+100));
    }
  }

  // saving the old trigger hits into 
  
  for(int x=0; x<trigger_hit_index; x=x+1){
//    Serial.print(old_trigger_hits[x]);
//    Serial.print("   ->   ");
//    Serial.print(trigger_hits[x]);
//    Serial.println("");
    old_trigger_hits[x] = trigger_hits[x];
  }
    
  old_trigger_hit_index = trigger_hit_index;
  
  display.display();
  cycle=cycle+1;
  avg_count=avg_count+1;
  //old_trigger_hits;

}
