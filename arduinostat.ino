// Project Name: zerostat
// Version: 1.0
// Description: Arduino Uno based potentiostat 
// Author: Subrata Sarker <subrata_sarker@yahoo.com>
// Date: 2020.08.31
// License: MIT

// PWM and ADC pin on arduino uno
int DAC_OUT_R = 9;      // PWM
int DAC_OUT_W = 10;     // PWM

float ADC_IN_C = A0;    // ADC (Current)
float ADC_IN_V = A1;    // ADC (Voltage)

float c = 0;
float v = 0;
float r = 0;
int pot = 0;
int inputs = 0;

String inputstr;

unsigned int interval;        // delay time for each step
unsigned int halt;            // 0 for stop and 1 for run            
unsigned int mode;            // 0 for lsv and 1 for cv
int ncycles;                  // number of cycles from 0 to onward
int pcom = 127;      // should be smaller than end point (5, 250)
int pstart;          // should be smaller than end point (0, 255)
int pend;            // should be larger than start point (0, 255)


// fixed due to the DAC resolution (1/0.019608 == 51)
// float potstep = 0.019608; 
int const ndata = 10;
int cdata[ndata];
float avgc;   // average of the input/ADC

void setup() {
  // setting up the device
  Serial.begin(9600);
  
  pinMode(DAC_OUT_R,OUTPUT);
  pinMode(DAC_OUT_W,OUTPUT);
  // Set PWM frequency to 31 kHz for pin 9 and 10 (Timer1)
  TCCR1B = TCCR1B & 0b11111000 | 0b00000001; //Set dividers to change PWM frequency
  
  delay(500);
  
  pinMode(ADC_IN_C,INPUT);
  pinMode(ADC_IN_V,INPUT);
  delay(500);
  
  // get the reference voltages
  analogWrite(DAC_OUT_R,pcom); // set potential and
  analogWrite(DAC_OUT_W,pcom); // set potential and
  delay(200); // wait
  v = analogRead(ADC_IN_V);
  v = v * 255.0 / 1023;
  
  // reset pot
  pot = 0;

  Serial.println("ready,,,,,,");
  delay(500);
}

void loop() {
  halt = 0;
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
  
  analogWrite(DAC_OUT_R, pcom); // set potential
  analogWrite(DAC_OUT_W, pcom); // set potential
  
  inputs = get_inputs();
  if (inputs > 0 && halt == 0) {
    inputs = 0;
    switch(mode) {
      // if mode = 0 then LSV
      case 0:
        if (pstart < pend) {
          forwardScanLSV();
        } else {
          reverseScanLSV();
        }
        broadcast(false);
        break;

      // if mode = 1 then CV
      case 1:
        for (int i = 0; i < ncycles; i++) {
          if (pstart < pend) {
            forwardScanCV();
          } else {
            reverseScanCV();
          }
        }
        broadcast(false);
        break;
    }
  }  
}

// pstart < pend
void forwardScanLSV() {
  for(pot = pstart; pot <= pend; pot++)
  {
    inputs = get_inputs();
    if (inputs > 0 && halt == 1) {
      inputs = 0;
      halt = 0;
      break;
    }
    
    // data write and read    
    dataReadWrite();
    
    // send data to the serial    
    broadcast(true);
  }
}

// pstart > pend
void reverseScanLSV() {
  for(pot = pstart; pot >= pend; pot--)
  {
    inputs = get_inputs();
    if (inputs > 0 && halt == 1) {
      inputs = 0;
      halt = 0;
      break;
    }

    // data write and read    
    dataReadWrite();

    // send data to the serial
    broadcast(true);
  }
}

// pstart  < pend
void forwardScanCV() {
  for(pot = pstart; pot <= pend; pot++)
  {
    inputs = get_inputs();
    if (inputs > 0 && halt == 1) {
      inputs = 0;
      halt = 0;
      break;
    }
    
    // data write and read    
    dataReadWrite();
    
    // send data to the serial    
    broadcast(true);
  }

  for(pot = pend; pot >= pstart; pot--)
  {
    inputs = get_inputs();
    if (inputs > 0 && halt == 1) {
      inputs = 0;
      halt = 0;
      break;
    }
    
    // data write and read    
    dataReadWrite();
    
    // send data to the serial    
    broadcast(true);
  }
}


// pstart > pend
void reverseScanCV() {
  for(pot = pstart; pot >= pend; pot--)
  {
    inputs = get_inputs();
    if (inputs > 0 && halt == 1) {
      inputs = 0;
      halt = 0;
      break;
    }

    // data write and read    
    dataReadWrite();

    // send data to the serial
    broadcast(true);
  }

  for(pot = pend; pot <= pstart; pot++)
  {
    inputs = get_inputs();
    if (inputs > 0 && halt == 1) {
      inputs = 0;
      halt = 0;
      break;
    }

    // data write and read    
    dataReadWrite();

    // send data to the serial
    broadcast(true);
  }
}


float average(int numbers[], int count) {
  int sum = 0;
  for (int i = 0; i < count; i++) {
    sum += numbers[i];
  }
  return ((float)sum / count);
}

void broadcast(bool is_running) {
  (is_running) ? Serial.print(1) : Serial.print(0);
  Serial.print(",");
  Serial.print(pot);
  Serial.print(",");
  Serial.print(avgc);
  Serial.print(",");
  Serial.print(v);
  Serial.print(",");
  Serial.print(interval);
  Serial.print(",");
  Serial.println();
}

void dataReadWrite() {
  analogWrite(DAC_OUT_R,pot);
  analogWrite(DAC_OUT_W,pcom); // set potential and
  delay(interval); // wait

  // get data and average
  for (int i = 0; i < ndata; i++) {
    c = analogRead(ADC_IN_C);
    cdata[i] = c;
  }
  avgc = average(cdata, ndata);
}

int get_inputs() {
  int result = 0;
  
  if (Serial.available()) {
    inputstr = String("");
    while (Serial.available())
    {
      delay(10);
      if (Serial.available() > 0)
      {
        char c = Serial.read();
        inputstr = String(inputstr + c);
      }
    }
    
    // s => "interval,halt,mode, ncycles, pcom, pstart,pend"
    result = sscanf(inputstr.c_str(), "%u,%u,%u,%d,%d,%d,%d\r", &interval, &halt ,&mode, &ncycles, &pcom, &pstart, &pend);
  }
  
  return result;
}
