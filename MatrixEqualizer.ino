#define LATCHPIN 10
#define CLOCKPIN 12
#define DATAPIN 11
#define CLOCK 9
#define RESET 8
#define AUDIOPIN A7          // Аудиовход

#define LIN_OUT 1
#define FFT_N 64
#include <FFT.h>

//
//int pictureSmile[10] = {
//  strtol("0111111110",NULL,2),
//  strtol("1000000001",NULL,2),
//  strtol("1011001101",NULL,2),
//  strtol("1011001101",NULL,2),
//  strtol("1000000001",NULL,2),
//  strtol("1000000001",NULL,2),
//  strtol("1010000101",NULL,2),
//  strtol("1001111001",NULL,2),
//  strtol("1000000001",NULL,2),
//  strtol("0111111110",NULL,2)
//};
//
//
//int pictureAll[10] = {
//  strtol("1111111111",NULL,2),
//  strtol("1111111111",NULL,2),
//  strtol("1111111111",NULL,2),
//  strtol("1111111111",NULL,2),
//  strtol("1111111111",NULL,2),
//  strtol("1111111111",NULL,2),
//  strtol("1111111111",NULL,2),
//  strtol("1111111111",NULL,2),
//  strtol("1111111111",NULL,2),
//  strtol("1111111111",NULL,2)
//};

int pictureEmpty[10] = {0,0,0,0,0,0,0,0,0,0};
float equalizerVolumes[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int volumesBitmap[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
byte matrix[10][10];

void setup(){
  pinMode(A7, INPUT);
  pinMode(DATAPIN,OUTPUT);
  pinMode(CLOCKPIN,OUTPUT);
  pinMode(LATCHPIN,OUTPUT);
  pinMode(CLOCK,OUTPUT);
  pinMode(RESET,OUTPUT);
  digitalWrite(RESET,HIGH);
  digitalWrite(RESET,LOW);    
  equalizerVolumes[1] = 4;
  equalizerVolumes[8] = 8;
  analogReference(EXTERNAL);  
  Serial.begin(9600);// use the serial port
  TIMSK0 = 0; // turn off timer0 for lower jitter - delay() and millis() killed
  ADCSRA = 0xe5; // set the adc to free running mode
  ADMUX = 0b01000111; // use adc7
  DIDR0 = 0x01; // turn off the digital input for adc0
}

void tenBitsOut(int value){
  for(int i = 0; i < 10; i++){
    digitalWrite(DATAPIN, !((value >> i) & 0x1));
    digitalWrite(CLOCKPIN, HIGH);
    digitalWrite(CLOCKPIN, LOW);
    digitalWrite(DATAPIN, LOW);
  }
}


int val;
unsigned char grenzen[11] = {0,3,4,6,8,11,13,16,20,24,32};
float faktoren[10] = {1, 1.1, 1.15, 1.25, 1.45, 1.55, 1.75, 1.8, 2, 3};
//float faktoren[10] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

void setBalken(unsigned char column, unsigned char height){                   //calculation of the height of each column
  
    unsigned char h;
    if (height > 255) height = 254; 
    height = (unsigned char)(height * faktoren[column]);
    h = (unsigned char)map(height, 0, 255, 0, 9);
    
    //
    if (h < equalizerVolumes[column]){
        equalizerVolumes[column]-=0.3;
    }
    else if (h > equalizerVolumes[column]){
        equalizerVolumes[column] = h;
    }
}

void loop() {
while(true){
    // reduces jitter
  cli();  // UDRE interrupt slows this way down on arduino1.0
       for (int i = 0 ; i < 128 ; i += 2) { // save 256 samples
        while(!(ADCSRA & 0x10)); // wait for adc to be ready
         ADCSRA = 0xf5; // restart adc
         byte m = ADCL; // fetch adc data
         byte j = ADCH;
         int k = (j << 8) | m; // form into an int
         k -= 0x0200; // form into a signed int
         k <<= 6; // form into a 16b signed int
         fft_input[i] = k; // put real data into even bins
         fft_input[i+1] = 0; // set odd bins to 0
       }

       
       // window data, then reorder, then run, then take output
       fft_window(); // window the data for better frequency response
       fft_reorder(); // reorder the data before doing the fft
       fft_run(); // process the data in the fft
       fft_mag_lin(); // take the output of the fft
       sei(); // turn interrupts back on
       
       fft_lin_out[0] = 0;
       fft_lin_out[1] = 0;
       
       unsigned char maxW = 0;
       for(unsigned char i = 0; i < 10; i++){
        maxW = 0;
        for(unsigned char x = grenzen[i]; x < grenzen[i+1];x++){
 
           if((unsigned char)fft_lin_out[x] > maxW){
            maxW = (unsigned char)fft_lin_out[x];
           }
        }
      setBalken(i, maxW);
    }
    TIMSK0 = 1;
  volumesToBitmap();
  drawBitmap(volumesBitmap, 20);
  TIMSK0 = 0;
}
}


void drawCircle(int x, int y, int r)
{
    int x1,y1,yk = 0;
    int sigma,delta,f;

    
    
    x1 = 0;
    y1 = r;
    delta = 2*(1-r);

    do
    {
        matrix[x+x1][y+y1] = 1;
        matrix[x-x1][y+y1] = 1;
        matrix[x+x1][y-y1] = 1;
        matrix[x-x1][y-y1] = 1;

        f = 0;
        if (y1 < yk)
            break;
        if (delta < 0)
        {
            sigma = 2*(delta+y1)-1;
            if (sigma <= 0)
            {
                x1++;
                delta += 2*x1+1;
                f = 1;
            }
        }
        else
        if (delta > 0)
        {
            sigma = 2*(delta-x1)-1;
            if (sigma > 0)
            {
                y1--;
                delta += 1-2*y1;
                f = 1;
            }
        }
        if (!f)
        {
            x1++;
            y1--;
            delta += 2*(x1-y1-1);
            }
    }
    while(1);
}


void volumesToBitmap(){
  for (int i = 0; i < 10; i++){
    volumesBitmap[i] = 0;
  }
  for (int i = 0; i < 10; i++){
    for(int j = 0; j < (int)equalizerVolumes[9-i]; j++){
      volumesBitmap[9-j] |= 0x1 << i;
    }
  }
}

void matrixToBitmap(){ 
  for (int i = 0; i < 10; i++){
    for (int j = 9; j >= 0; j--){
       volumesBitmap[i] |= ((int)matrix[i][j]) << (9-j);
    }
  }
}

int drawBitmap(int *bitmap, int timeToDisplay){
  long timeMilliseconds = millis();  
  while(millis() - timeMilliseconds < timeToDisplay){
    digitalWrite(RESET,HIGH);
    digitalWrite(RESET,LOW);
    for(int i = 0; i < 10; i++){
      digitalWrite(LATCHPIN, LOW);
      tenBitsOut(bitmap[i]);
      digitalWrite(LATCHPIN, HIGH);    
      digitalWrite(CLOCK, HIGH);
      digitalWrite(CLOCK, LOW);
    }
  }
}
