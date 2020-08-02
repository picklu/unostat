//
// Based on the paper "Microcontroller based potentiostat"
// DOI: 10.1021/acs.jchemed.5b00961 (J. Chem. Educ. 2016, 93, 1320−1322)
//

// PWM and ADC pin on arduino uno
int a = 11;
float ct = A0; //ADC

int vals[] = {0, 64, 127, 191, 255};

float c = 0;
int val = -1;
int inPos = -1;
char inStr[2];


void setup() {
  TCCR1B = TCCR1B & B11111000 | B00000001; //Set dividers to change PWM frequency
  Serial.begin(9600);
  pinMode(a,OUTPUT);
  pinMode(ct,INPUT);

  // reset val
  val = -1;
}

void loop() {
  // Output headers
  Serial.print("pos");
  Serial.print(",");
  Serial.print("Voltage");
  Serial.print(",");
  Serial.print("Current");
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
      if (inPos >= 0 && inPos < 5) 
      {
        while (Serial.available() == 0)
        {
          Serial.print(inPos);
          Serial.print(",");
          val = vals[inPos];
          analogWrite(a,val);
          Serial.print(val);
          delay(500);
          c = analogRead(ct);
          Serial.print(",");
          Serial.print(c);
          Serial.print(",");
          Serial.println();
        }
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
