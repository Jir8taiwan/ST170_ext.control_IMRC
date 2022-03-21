// ****** Using Arduino board to control IMRC active signal externally for Ford Focus MK1 ST170
// Version 2.0 in 2022.03.21
//
// ****** define environment:
// **digital pin 12 is control +5V realy coil
// **digital pin 7 is the hall pin of signal input
int hall_pin = 7;
// **set number of hall trips for RPM reading (higher value scale improves accuracy)
float hall_thresh = 400.0;
//float hall_thresh = 100.0;
//float hall_thresh = 50.0;

// **initial usage varies
int signalstatus = 0;
int oldsignalstatus = 0;
int relaystatus = 0;
int rpm_val = 0;


// ******setup code to run once:
void setup() {
Serial.begin(115200); // **initialize serial communication at 115200 bits per second

pinMode(hall_pin, INPUT);// **make the hall pin an input type

pinMode(12, OUTPUT); // **Relay signal active output as pin 12
digitalWrite(12, LOW); // **Relay signal default is OFF status
delay(500);

//pinMode(8,OUTPUT); //Pin 8 borrow as power 5v for Relay
//digitalWrite(8,HIGH); //Pin 8 borrow as power 5v for Relay
//delay(500);

}


// ******the loop routine runs over and over again forever:
void loop() {
// **preallocate values for tachometer
float hall_count = 1.0;
float start = micros();
bool on_state = false;

// **counting number of times the hall sensor is tripped
// **but without double counting during the same trip
while(true){
if (digitalRead(hall_pin)==0){
if (on_state==false){
on_state = true;
hall_count+=1.0;
}
} else{
on_state = false;
}

// **when count samples in time enough and break out loop
if (hall_count>=hall_thresh){
break;
}
}

// print information about Time and RPM values in monitor window
float end_time = micros();
float time_passed = ((end_time-start)/1000000.0);
Serial.print("Time Passed: ");
Serial.print(time_passed);
Serial.println("s");
float rpm_val = (hall_count/time_passed)*60.0;
Serial.print(rpm_val);
Serial.println(" RPM");
delay(1); // delay in between reads for stability

SIGNALFILTER:
signalstatus = digitalRead(12); // **memory pin 12 condition to compute
if ((signalstatus == HIGH) && (oldsignalstatus == LOW)){
relaystatus = 1 - relaystatus; // **relaystatus: 0=1-0=1 or 1=1-1=0
delay(300);
}

oldsignalstatus == signalstatus; // **both HIGH or LOW between now and old signal
if (relaystatus == 1){
digitalWrite(12, HIGH); // **when still relaystatus=1, output to start ON
}
else {
digitalWrite(12, LOW); // **when change relaystatus=0, output to stop OFF
}

COMPARERPM:
rpm_val=rpm_val/12; // **fix filter 12 times of possible error

if(rpm_val>=3900){ 
delay(500); // **ensure more then 3900rpm and wait 500ms
if(rpm_val>=4000){
digitalWrite(12, HIGH); // **ensure more then 3900rpm and do output to start ON
delay(500);
}
} else {
digitalWrite(12, LOW); // **Otherwise do relay keep NC OFF
delay(500);
}

}
