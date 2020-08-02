// 
// Based on the paper "Microcontroller based potentiostat"
// DOI: 10.1021/acs.jchemed.5b00961 (J. Chem. Educ. 2016, 93, 1320−1322)
// 

// PWM and ADC pin on arduino uno
int DAC_OUT = 11;   // PWM
float ADC_IN = A0;  // ADC

float Potstep = 0.0078; // fixed due to the DAC resolution
int vevals[] = {10,25,50,100,200,250,300}; //multiple scan rates values (mV/s)
int const count = 6;
long intervalos[count];

float c = 0;
int inPos = -1; 
int val = 0;
char inStr[2];
bool halt = false;

void setup() {
  TCCR1B = TCCR1B & B11111000 | B00000001; //Set dividers to change PWM frequency
  Serial.begin(9600);
  pinMode(DAC_OUT,OUTPUT);
  pinMode(ADC_IN,INPUT);

  // reset val
  val = 0;
    
  //  populate intervalos
  for(int pos = 0; pos < count; pos++)
  {
    intervalos[pos]=(1000000L/((vevals[pos])*128L));
  }
}

void loop() {
  // Output headers
  halt = false;
  Serial.print("Voltage");
  Serial.print(",");
  Serial.print("Current");
  Serial.print(",");
  Serial.print("pos");
  Serial.print(",");
  Serial.print("SR");
  Serial.print(",");
  Serial.print("step");
  Serial.print(",");
  Serial.println();
    
  if(Serial.available() > 0)
  {
    // Read user input for index of the scan rate (0 ~ 5)
    // Only accept first character of the input          
    int inChar = Serial.read();
    if (isDigit(inChar))
    {  
      sprintf(inStr, "%c", inChar);
      consumeInput(); // Clear serial input
      inPos = atoi(inStr);
      if (inPos >= 0 && inPos <= 5) 
      {
        // sweep potential from -1 V to 1 V and 
        // read corresponding current
        forwardScan(inPos);
        reverseScan(inPos);
      }
      else 
      {
        Serial.print(inPos);
        Serial.println(" is an invalid input");
        delay(2000);
      }
      inPos = -1;
    }
    else
    {
      consumeInput();
      Serial.println("Invalid input");
      delay(2000);
    }
  }
}


void consumeInput() {
  while(Serial.available() > 0)
  {
    Serial.read();
  }
}


void forwardScan(int pos) {
  for(val = 0; val <= 255; val++)
  {
    if(Serial.available() > 0)
    {
      consumeInput(); // Clear serial input
      halt = true;
      break;
    }
    analogWrite(DAC_OUT,val);
    Serial.print(val);
    delay(intervalos[pos]);
    c = analogRead(ADC_IN);
    Serial.print(",");
    Serial.print(c);
    Serial.print(",");
    Serial.print(pos);
    Serial.print(",");
    Serial.print(vevals[pos]);
    Serial.print(",");  
    Serial.print(intervalos[pos]);
    Serial.print(",");
    Serial.println();
  }
}


void reverseScan(int pos) {
  for(val = 255; val >= 0; val--)
  {
    if(halt){break;}
    analogWrite(DAC_OUT,val);
    Serial.print(val);
    delay(intervalos[pos]);
    c = analogRead(ADC_IN);
    Serial.print(",");
    Serial.print(c);
    Serial.print(",");
    Serial.print(pos);
    Serial.print(",");
    Serial.print(vevals[pos]);
    Serial.print(",");
    Serial.print(intervalos[pos]);
    Serial.print(",");
    Serial.println();
  }
}
