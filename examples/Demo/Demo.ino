
/*

	RoboCore SimpleCom Example
		(28/03/2013)

  Written by François
  
  This example creates 5 Transmitters and
  5 Receivers in the same Arduino.
  To send the message, send 't' through
  the serial (all transmitters will send
  the same message).
  Check the images with the code to pair up
  the receivers with the transmitters. Then
  the same message sent should be received
  and displayed for the receivers.
  
*/


#include "SimpleCom.h"

  SCreceiver Rcvr(5,1);
  SCreceiver Rcvr2(7,2);
  SCreceiver Rcvr3(11,3);
  SCreceiver Rcvr4(12,4);
  SCreceiver Rcvr5(13,5);
  
  SCtransmitter Trmtr(4);
  SCtransmitter Trmtr2(6);
  SCtransmitter Trmtr3(8);
  SCtransmitter Trmtr4(9);
  SCtransmitter Trmtr5(10);

byte received_message[SC_MESSAGE_SIZE];

//  *** 2 examples of arrays >> not to use ***
//  SCtransmitter transmitters[] = { SCtransmitter(0,1), SCtransmitter(1,1) }; --> array of class
//  SCtransmitter transmitters[2]; --> array of class with default constructor


char c;

void setup(){
  Serial.begin(9600);
  
  Serial.print(Rcvr.Listen());
  Serial.print(',');
  Serial.print(Rcvr2.Listen());
  Serial.print(',');
  Serial.print(Rcvr3.Listen());
  Serial.print(',');
  Serial.print(Rcvr4.Listen());
  Serial.print(',');
  Serial.println(Rcvr5.Listen());
  
  Serial.print("OCR0A: ");
  Serial.println(T_OCR0A);
  Serial.print("Prescaler: ");
  Serial.println(T_PRESCALER);
  Serial.print("Interval: ");
  Serial.println(SC_TIMER_INTERVAL);
  
  SC_Start_Timer(); //start the timer for the communication
  
  Trmtr.SetStart(3000, 1500);
  Trmtr.SetInterval(500, 500); //cannot have the same value, so function will change automatically to (500, 300)
  
  //change durations
  Trmtr2.SetStart(2500, 1700);
  Trmtr2.SetInterval(900, 300);
  Rcvr.SetStart(2500, 1700);
  Rcvr.SetInterval(900, 300);
  Rcvr.Listen(); //restart communication
  
  Serial.println("--- start ---");
}




void loop(){
  if(Serial.available()){
    c = Serial.read();
    
    //send message
    if(c == 't'){
      byte message[] = {0,1,6,0,1};
      byte message_length = 5;
      Trmtr.SetID(1); //with oscilloscope
      Trmtr2.SetID(1); //with Rcvr
      Trmtr3.SetID(2); //with Rcvr2
      Trmtr4.SetID(4); //with Rcvr4
      Trmtr5.SetID(5);

Trmtr.Send(message, message_length);
Trmtr2.Send(message, message_length);
Trmtr3.Send(message, message_length);
Trmtr4.Send(message, message_length);
Trmtr5.Send(message, message_length);

      Serial.println("\tdone! ");
    }
  }
  
  
  //receive message in Rcvr
  if(Rcvr.GetMessage(received_message)){
    Serial.print(Rcvr.GetMessageLength());
    Serial.print(" - { ");
    for(int i=0 ; i < (Rcvr.GetMessageLength() - 1) ; i++){
      Serial.print(received_message[i]);
      Serial.print(", ");
    }
    Serial.print(received_message[Rcvr.GetMessageLength() - 1]);
    Serial.println(" }");
    Rcvr.ClearBuffer(); //clear buffer because of loop, or else will always enter here once a message is ready
  }
  
  
  //receive message in Rcvr2
  if(Rcvr2.GetMessage(received_message)){
    Serial.print(Rcvr2.GetMessageLength());
    Serial.print(" # { ");
    for(int i=0 ; i < (Rcvr2.GetMessageLength() - 1) ; i++){
      Serial.print(received_message[i]);
      Serial.print(", ");
    }
    Serial.print(received_message[Rcvr2.GetMessageLength() - 1]);
    Serial.println(" }");
    Rcvr2.ClearBuffer(); //clear buffer because of loop, or else will always enter here once a message is ready
  }
  
  
  //receive message in Rcvr4
  if(Rcvr4.GetMessage(received_message)){
    Serial.print(Rcvr4.GetMessageLength());
    Serial.print(" $ { ");
    for(int i=0 ; i < (Rcvr4.GetMessageLength() - 1) ; i++){
      Serial.print(received_message[i]);
      Serial.print(", ");
    }
    Serial.print(received_message[Rcvr4.GetMessageLength() - 1]);
    Serial.println(" }");
    Rcvr4.ClearBuffer(); //clear buffer because of loop, or else will always enter here once a message is ready
  }
}




