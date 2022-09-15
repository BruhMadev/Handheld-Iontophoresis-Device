#include <LiquidCrystal.h>

int voltageInc = A0, voltageDec = 12, timeInc = 11, timeDec = 10;
int startReset = 8, state = 0;
long initTime = 0, nowTime, voltageTime = 0, timeStep = 1;
volatile long state0time = 0; 
//requiredVoltage is fixed once the state is 1 and shown on lcd
//voltage is the output we apply after correction
double requiredVoltage = 0,voltageStep = 0.5, voltage=0; //voltage is the corrected output
bool V_inc = 1, V_dec = 1, T_inc = 1, T_dec = 1, SRvalue, isCorrecting = false, changeBattery = false;

const int rs = A5, en = 3, d4 = 4, d5 = 5, d6 = 6, d7 = 7, restart=2, backlight=A3;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

int outputVoltage = 9;
int feedbackPin = A2;
double voltageRead=0; 

// smoothing of analog read
const int numReadings = 20;

double readings[numReadings];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
double total = 0;                  // the running total
double average = 0;                // the average

void setup()
{
  Serial.begin(9600);
  lcd.begin(16, 2);
  pinMode(startReset, INPUT_PULLUP);
  pinMode(voltageInc, INPUT_PULLUP);
  pinMode(voltageDec, INPUT_PULLUP);
  pinMode(timeInc, INPUT_PULLUP);
  pinMode(timeDec, INPUT_PULLUP);
  pinMode(outputVoltage, OUTPUT);
  pinMode(restart, INPUT_PULLUP);
  attachInterrupt(0, Power_up, LOW); //interrupt for waking up
}

void loop()
{ 
  pinMode(backlight, OUTPUT);
  digitalWrite(backlight, HIGH);
  SRvalue = digitalRead(startReset);
  
  if (state == 0){
    if(millis()-state0time>15000){
      Power_down();
      }
    analogWrite(outputVoltage, 0);
    V_inc = digitalRead(voltageInc);
    V_dec = digitalRead(voltageDec);
    T_inc = digitalRead(timeInc);
    T_dec = digitalRead(timeDec);
    if (V_inc == LOW && requiredVoltage < 5 && requiredVoltage >= 0){
      requiredVoltage += voltageStep;
      state0time=millis();
      delay(300);
    } 
    if (V_dec == LOW && requiredVoltage <= 5 && requiredVoltage > 0){
      requiredVoltage -= voltageStep;
      state0time=millis();
      delay(300);
    }
    if (T_inc == LOW && voltageTime < 5 && voltageTime >= 0){
      voltageTime += timeStep;
      state0time=millis();
      delay(300);
    }
    if (T_dec == LOW && voltageTime <= 5 && voltageTime > 0){
      voltageTime -= timeStep;
      state0time=millis();
      delay(300);
    }
  }

   if (SRvalue == LOW && state == 0){
      delay(300);
      initTime = millis();
      state = 1;
      SRvalue = HIGH;
      voltage = requiredVoltage;
      // initialize all the readings to 0:
      for (int thisReading = 0; thisReading < numReadings; thisReading++) {
      readings[thisReading] = 0;
      total = 0;
      }
    }

    if (state == 1){
        total=0;
        //it will take 10 readinds and then we will apply correction
        //10 readings by a for loop and then when it exits the loop we apply the correction
        for (int thisReading = 0; thisReading < numReadings; thisReading++) {
          readings[thisReading] = ((double)analogRead(feedbackPin))*5/1023;
          total = total + readings[thisReading];
          delay(10);
          }

        average = total / numReadings; 
        analogWrite(outputVoltage, (255*voltage)/5);
        if(abs(average-requiredVoltage) > 0.02 && isCorrecting == false && (millis()-initTime)>4000){
          isCorrecting = true;
          delay(2);
          }

        if(isCorrecting == true){
          
          // send it to the computer as ASCII digits
          Serial.println(average, 4);  
          voltage += (requiredVoltage - average)*0.7;
          total = 0;
          delay(400);   // delay before reads and after changing value for stability
          if(voltage>5){voltage=5;}
          if(voltage<0){voltage=0;}
         }
        if(abs(average-requiredVoltage)<0.02 && isCorrecting == true){
          isCorrecting = false;
          delay(2);
          // initialize all the readings to 0:
          for (int thisReading = 0; thisReading < numReadings; thisReading++) {
          readings[thisReading] = 0;
          total = 0;
          }
        }
        
        //Send data by serial for printing 
        Serial.print(average, 4);
        Serial.print(" ");
        Serial.print(voltage, 4);
        Serial.print(" ");  
        Serial.println(requiredVoltage, 4);
        
        nowTime = millis();
        if ((nowTime-initTime) > (voltageTime*60*1000)){
          state = 0;
          requiredVoltage = 0.0;
          voltageRead = 0;
          voltage = 0;
          voltageTime = 0;
          initTime = 0;
          isCorrecting = false;
          analogWrite(outputVoltage, (255*voltage)/5);
          lcd.clear();
          state0time=millis();
        }
      }

    if (SRvalue == LOW && state == 1){
      delay(300);
      state = 0;
      voltageRead = 0;
      requiredVoltage = 0.0;
      voltage = 0;
      voltageTime = 0;
      initTime = 0;
      isCorrecting = false; 
      analogWrite(outputVoltage, (255*voltage)/5);
      SRvalue = HIGH;
      lcd.clear();
      state0time=millis();
    }
  if(changeBattery == false){
    lcd.setCursor(0,0);
    lcd.print("S:"+String(requiredVoltage, 1)+ "mA ");
//    lcd.print("R:"+String(voltageRead, 1)+ "mA");s
    if(initTime == 0){
      lcd.setCursor(0,1);
      lcd.print(String(voltageTime)+" minutes ");
    }else{
      lcd.setCursor(0,1);
       lcd.print(String((voltageTime*60*1000-millis()+initTime)/1000)+ " sec left"); 
    }
  }
}

void Power_down(){
  lcd.clear();
  lcd.noDisplay();
  pinMode(backlight, INPUT);
//  lcd.setCursor(0,0);
//  pinMode(A3, INPUT);
//  lcd.print("Power OFF mode");
  //Disable ADC - don't forget to flip back after waking up if using ADC in your application ADCSRA |= (1 << 7);
  ADCSRA &= ~(1 << 7);
  
  //ENABLE SLEEP - this enables the sleep mode
  SMCR |= (1 << 2); //power down mode
  SMCR |= 1;//enable sleep  
  
  //BOD DISABLE - this must be called right before the __asm__ sleep instruction
  MCUCR |= (3 << 5); //set both BODS and BODSE at the same time
  MCUCR = (MCUCR & ~(1 << 5)) | (1 << 6); //then set the BODS bit and clear the BODSE bit at the same time
  __asm__  __volatile__("sleep");//in line assembler to go to sleep
}

void Power_up(){
  // Turning the ADC back on
  lcd.display();
  ADCSRA |= (1 << 7);
  state0time=millis();
  lcd.clear();
  }
