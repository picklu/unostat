// simplestat
// Based on the paper "Microcontroller based potentiostat"
// DOI: 10.1021/acs.jchemed.5b00961 (J. Chem. Educ. 2016, 93, 1320−1322)
// For Arduino Due

// PWM and ADC pin on arduino uno
int DAC_OUT_R = DAC0;   // DAC (12 bit for voltage output)
int DAC_OUT_W = DAC1;   // DAC (12 bit for voltage output)
float ADC_IN_C = A0;    // ADC (voltage input)
float ADC_IN_V = A1;    // ADC (Current input)

unsigned int DAC_BR = 12;  // DAC bit resolution
unsigned int ADC_BR = 12;  // ADC bit resolution
unsigned int DAC_DP = 4096; // data points for 12 bit DAC
unsigned int ADC_DP = 4096; // data points for 12 bit ADC

float sp = 0.55; // lowest potential from DAC
float ep = 2.75; // highest potential from DAC
int srs[] = {10,25,50,100,200,300}; //multiple scan rates values (mV/s)

int const count = 6;
int const data_count = 10;
long intervals[count];

int cdata[data_count];
float avg_cdata;
float c = 0;

// int vdata[data_count];
// float avg_vdata;
// float v = 0;

int val = 0;
int inputs = 0;

String inputstr;

unsigned int halt;            // 0 for stop and 1 for run       
unsigned int sr;              // 0, 1, 2, 3, 4, and 5 
unsigned int mode;            // 0 for lsv and 1 for cv
int ref;             // should be smaller than end point (0, 4095)
int pstart;          // should be smaller than end point (0, 4095)
int pend;            // should be larger than start point (0, 4095)
int skip;

void setup() {
  // setting up the device
  Serial.begin(9600);
  Serial.println("==== setting up =====");
  pinMode(DAC_OUT_R,OUTPUT);
  pinMode(DAC_OUT_W,OUTPUT);
  pinMode(ADC_IN_C,INPUT);
  pinMode(ADC_IN_V,INPUT);
  delay(500);


  // Change read/write resolution to 8 bit
  analogReadResolution(ADC_BR);
  analogWriteResolution(DAC_BR);

  delay(200); // wait

  // reset val
  val = 0;
    
  //  populate intervalos
  for(int pos = 0; pos < count; pos++)
  {
    intervals[pos] = 1000000L * (ep - sp) / (DAC_DP * srs[pos]);
  }

  Serial.println("======= done ========");
  broadcast_header();
  
  delay(500);
}

void loop() {

  halt = 0;

  analogWrite(DAC_OUT_R, DAC_DP / 2); // set potential
  
  delay(100);
  
  inputs = get_inputs();
  if (inputs > 0 && halt == 0 && pend > pstart) {
    inputs = 0;

    switch(mode) {
      // if mode = 0 then forward scan
      case 0:
        forwardScan(sr);
        break;

      // if mode = 1 then reverse scan
      case 1:
        reverseScan(sr);
        break;

      // if mode = 2 then reverse scan follows forward scan (cycle)
      case 2:
        forwardScan(sr);
        reverseScan(sr);
        break;

      // if mode = 3 then forward scan follows reverse scan (cycle)
      case 3:
        reverseScan(sr);
        forwardScan(sr);
        break;

      // if mode = 4 or more then reverse scan follows forward scan ((mode -2)  no. of cycles)
      default:
        for (int i = 0; i < mode - 2; i++) {
          forwardScan(sr);
          reverseScan(sr);
        }
    }
  }  
}

void forwardScan(unsigned int pos) {
  for(val = pstart; val <= pend; val=val+skip)
  {
    // check input for interruption    
    inputs = get_inputs();
    if (inputs > 0 && halt == 1) {
      inputs = 0;
      halt = 0;
      break;
    }
    
    // write-read data
    if (update_data(pos)) {
      analogWrite(DAC_OUT_R, 0); // reset potential
      analogWrite(DAC_OUT_W, 0); // reset potential
      Serial.println("======= Overload ========");
      broadcast_header();
      halt = 0;
      break;
    }
    
    // send data to the serial    
    broadcast_data(pos);
  }
}


void reverseScan(unsigned int pos) {
  for(val = pend; val >= pstart; val=val-skip)
  {
    // check input for interruption    
    inputs = get_inputs();
    if (inputs > 0 && halt == 1) {
      inputs = 0;
      halt = 0;
      break;
    }
    
    // write-read data
    if (update_data(pos)) {
        // get the reference voltages
      analogWrite(DAC_OUT_R, 0); // reset potential and
      analogWrite(DAC_OUT_W, 0); // reset potential and
      Serial.println("======= Overload ========");
      broadcast_header();
      halt = 0;
      break;
    }
    
    // send data to the serial
    broadcast_data(pos);
  }
}

float average(int numbers[], int count) {
  int sum = 0;
  for (int i = 0; i < count; i++) {
    sum += numbers[i];
  }
  return ((float)sum / count);
}


bool update_data(unsigned int pos) {
    // get the reference voltages
    analogWrite(DAC_OUT_W, ref); // set potential at WE
    analogWrite(DAC_OUT_R,val); // set potential at RE
    delay(intervals[pos]); // wait

    // get data and average
    for (int i = 0; i < data_count && halt == 0; i++) {
      // v = analogRead(ADC_IN_V);
      c = analogRead(ADC_IN_C);
      if (c > ADC_DP) {
        return true;
      }
      // vdata[i] = v;
      cdata[i] = c;
    }
    // avg_vdata = average(vdata, data_count);
    avg_cdata = average(cdata, data_count);
    return false;
}

void broadcast_header() {
  Serial.print("Voltage");
  Serial.print(",");
  Serial.print("Current");
  Serial.print(",");
  Serial.print("idx");
  Serial.print(",");
  Serial.print("sr");
  Serial.print(",");
  Serial.print("delay");
  Serial.print(",");
  Serial.print("skip");
  Serial.print(",");
  Serial.println();
}

void broadcast_data(int pos) {
  Serial.print(val);
  Serial.print(",");
  Serial.print(avg_cdata);
  Serial.print(",");
  Serial.print(val);
  Serial.print(",");
  Serial.print(srs[pos]);
  Serial.print(",");  
  Serial.print(intervals[pos]);
  Serial.print(",");
  Serial.print(skip);
  Serial.print(",");
  Serial.println();
}

int get_inputs() {
  int result = 0;
  
  if (Serial.available()) {
    inputstr = String("");
    while (Serial.available())
    {
      delay(10);
      if (Serial.available() >0)
      {
        char c = Serial.read();
        inputstr = String(inputstr + c);
      }
    }
    
    // s => "halt,sr,mode,pstart,pend,skip"
    result = sscanf(inputstr.c_str(), "%u,%u,%u,%d,%d, %d, %d\r", &halt, &sr, &mode, &ref, &pstart, &pend, &skip);
  }
  
  return result;
}
