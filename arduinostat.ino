// Project Name: zerostat/unostat
// Version: 1.0
// Description: Arduino Due based potentiostat 
// Author: Subrata Sarker <subrata_sarker@yahoo.com>
// Date: 2020.09.09
// License: MIT
// Description: This firmware is for duestat a member of zerostat series

// hardware information
#define MODEL "DUESTAT"
#define VERSION "1.0"
#define DAC_BR 12     // DAC bit
#define ADC_BR 12     // ADC bit
#define MAX_ADC 4096
#define MAX_DAC 4096
#define BAUD_RATE 115200


// DAC and ADC pin on arduino Due
int DAC_OUT_R = DAC0;   // DAC (12 bit for voltage output)
int DAC_OUT_W = DAC1;   // DAC (12 bit for voltage output)
float ADC_IN_C = A0;    // ADC (12 bit current input)
float ADC_IN_V = A1;    // ADC (12 bit voltage input)

// global variables
float cdgt = 0;         // digital value for current
float vdgt = 0;         // digital value for voltage
int rvdgt = 0;          // digital value for running voltage
int inputs = 0;         // serial inputs in integer
int interval = 0;       // delay time for each step
int halt = 1;           // 0 for stop and 1 for run            
int mode = 0;           // 0 for lsv and 1 for cv
int ncycles = 0;        // number of cycles from 0 to onward
int pcom = 0;           // (0 ~ 4095)
int pstart = 0;         // (0 ~ 4095)
int pend = 0;           // (0 ~ 4095)
int pstep = 1;          // (1 ~ 100)
int const ndata = 10;   // number of data points to be averaged
int cdata[ndata];       // array of data to be averaged
float avgc = 0;         // average of the input/ADC
String inputstr;        // input string to cast the user input

void setup() {
  // setting up the device
  Serial.begin(BAUD_RATE);
  
  pinMode(DAC_OUT_R,OUTPUT);
  pinMode(DAC_OUT_W,OUTPUT);
  delay(500);
  
  pinMode(ADC_IN_C,INPUT);
  pinMode(ADC_IN_V,INPUT);
  delay(250);

  // Change read/write resolution
  analogReadResolution(ADC_BR);
  analogWriteResolution(DAC_BR);
  delay(250);
  
  Serial.print(MODEL);
  Serial.print(",");
  Serial.print(VERSION);
  Serial.print(",");
  Serial.println("READY,,,,");
  delay(500);
}

void loop() {
  halt = 0;
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
  broadcast(false);
  
  // check for inputs and respond accordingly
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
  
  for(rvdgt = pstart; rvdgt <= pend && halt == 0; rvdgt += pstep)
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
  for(rvdgt = pstart; rvdgt >= pend && halt == 0; rvdgt -= pstep)
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
  for(rvdgt = pstart; rvdgt <= pend && halt == 0; rvdgt += pstep)
  {
    // if halt then break
    inputs = get_inputs();
    if (inputs > 0 && halt == 1) { break; }
    
    // else data write and read    
    dataReadWrite();
    
    // send data to the serial    
    broadcast(true);
  }

  for(rvdgt = pend; rvdgt >= pstart && halt == 0; rvdgt -= pstep)
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
  for(rvdgt = pstart; rvdgt >= pend && halt == 0; rvdgt -= pstep)
  {
    // if halt then break
    inputs = get_inputs();
    if (inputs > 0 && halt == 1) { break; }

    // else data write and read    
    dataReadWrite();

    // send data to the serial
    broadcast(true);
  }

  for(rvdgt = pend; rvdgt <= pstart && halt == 0; rvdgt += pstep)
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

void dataReadWrite() {
  // Change read/write resolution
  analogWrite(DAC_OUT_R,rvdgt); // set potential at the RE
  analogWrite(DAC_OUT_W,pcom);  // set potential at the WE
  pause(); // wait
  
  // get data and average
  for (int i = 0; i < ndata; i++) {
    cdgt = analogRead(ADC_IN_C);
    cdata[i] = cdgt;
  }
  avgc = average(cdata, ndata);
}

void softReset() {
  interval = 0;
  halt = 1;
  mode = 0;
  ncycles = 0;
  pcom = 2048;
  pstart = 0;
  pend = 0;
  pstep = 1;
  avgc = 0;
  rvdgt = 2048;
}

void resetPotentials() {
  analogWrite(DAC_OUT_R,rvdgt); // set rvdgtential and
  analogWrite(DAC_OUT_W,pcom); // set rvdgtential and
  delay(50); // wait
  vdgt = analogRead(ADC_IN_V);
  vdgt = vdgt * MAX_DAC / MAX_ADC;
  avgc = analogRead(ADC_IN_C);
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
    
    // s => "interval,halt,mode, ncycles, pcom, pstart,pend,pstep"
    result = sscanf(inputstr.c_str(), 
      "%d,%d,%d,%d,%d,%d,%d,%d\r", 
      &interval, &halt ,&mode, &ncycles, &pcom, &pstart, &pend, &pstep);
  }
  return result;
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
  Serial.print(rvdgt);
  Serial.print(",");
  Serial.print(avgc);
  Serial.print(",");
  Serial.print(vdgt);
  Serial.print(",");
  Serial.print(interval);
  Serial.print(",");
  Serial.println();
}

void pause() {
  delay(interval - 1);
  delayMicroseconds(780);
}
