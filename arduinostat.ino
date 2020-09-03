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

int interval = 0;        // delay time for each step
int halt = 1;            // 0 for stop and 1 for run            
int mode = 0;            // 0 for lsv and 1 for cv
int ncycles = 0;         // number of cycles from 0 to onward
int pcom = 127;          // should be smaller than end point (5, 250)
int pstart = 0;          // should be smaller than end point (0, 255)
int pend = 0;            // should be larger than start point (0, 255)

int const ndata = 10;
int cdata[ndata];
float avgc = 0;   // average of the input/ADC

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
  
  // reset potentails and inputs
  softReset();

  // get the reference voltages
  resetPotentials();
  
  Serial.println("ready,,,,,,");
  delay(500);
}

void loop() {
  halt = 0;
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);

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
  for(pot = pstart; pot <= pend && halt == 0; pot++)
  {
    // if halt then break
    inputs = get_inputs();
    if (inputs > 0 && halt == 1) { break; }
    
    // else data write and read    
    dataReadWrite();
    
    // send data to the serial    
    broadcast(true);
  }
}

// pstart > pend
void reverseScanLSV() {
  for(pot = pstart; pot >= pend && halt == 0; pot--)
  {
    // if halt then break
    inputs = get_inputs();
    if (inputs > 0 && halt == 1) { break; }

    // else data write and read    
    dataReadWrite();

    // send data to the serial
    broadcast(true);
  }
}

// pstart  < pend
void forwardScanCV() {
  for(pot = pstart; pot <= pend && halt == 0; pot++)
  {
    // if halt then break
    inputs = get_inputs();
    if (inputs > 0 && halt == 1) { break; }
    
    // else data write and read    
    dataReadWrite();
    
    // send data to the serial    
    broadcast(true);
  }

  for(pot = pend; pot >= pstart && halt == 0; pot--)
  {
    // if halt then break
    inputs = get_inputs();
    if (inputs > 0 && halt == 1) { break; }
    
    // else data write and read    
    dataReadWrite();
    
    // send data to the serial    
    broadcast(true);
  }
}

// pstart > pend
void reverseScanCV() {
  for(pot = pstart; pot >= pend && halt == 0; pot--)
  {
    // if halt then break
    inputs = get_inputs();
    if (inputs > 0 && halt == 1) { break; }

    // else data write and read    
    dataReadWrite();

    // send data to the serial
    broadcast(true);
  }

  for(pot = pend; pot <= pstart && halt == 0; pot++)
  {
    // if halt then break
    inputs = get_inputs();
    if (inputs > 0 && halt == 1) { break; }

    // else data write and read    
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
  if (is_running) {
    Serial.print(1);
  } else {
    softReset();
    resetPotentials();
    Serial.print(0);
  }
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

void softReset() {
  interval = 0;
  halt = 1;
  mode = 0;
  ncycles = 0;
  pcom = 127;
  pstart = 0;
  pend = 0;
  avgc = 0;
  pot = 0;
}

void resetPotentials() {
  analogWrite(DAC_OUT_R,pcom); // set potential and
  analogWrite(DAC_OUT_W,pcom); // set potential and
  delay(50); // wait
  v = analogRead(ADC_IN_V);
  v = v * 256 / 1024;
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
    result = sscanf(inputstr.c_str(), 
      "%d,%d,%d,%d,%d,%d,%d\r", 
      &interval, &halt ,&mode, &ncycles, &pcom, &pstart, &pend);
  }
  
  return result;
}
