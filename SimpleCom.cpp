/*

	RoboCore SimpleCom Library
		(v1.0 - 28/03/2013)

  Simple Communication functions for Arduino
    (tested with Arduino 1.0.1)

  Copyright 2013 RoboCore (François) ( http://www.RoboCore.net )
  
  ------------------------------------------------------------------------------
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
  ------------------------------------------------------------------------------
  
  This library implements 2 classes: SCtransmitter
  and SCreceiver.
  The two classes communicate with each other on
  the same channel and with the same ID (ex:
  receiver with ID=1 and Channel=1 will receive
  a message from a transmitter in channel 1 and with
  an ID set to 1). The ID of the Receiver is set
  on its creation, whereas the ID of the Transmitter
  depends on the target Receiver, and therefore can
  be set whenever convenient.
  
  The purpose of this library is to allow multiple
  transmitters and receivers for the same Arduino and
  let user change the base protocol to match their
  needs.
  
  There is a limit of how many Transmitters and Receivers
  can be created for each program. All set and valid
  instances are tracked by the program. Invalid instances
  cannot send nor receive.
  
  The signals' times can be configured by the user by
  calling the SetInterval() and SetStart() methods.
  This allows each user to have its own protocol, so
  the library can be used with multiple applications.
  The custom times obviously must be greater than the
  minimum and lesser than the maximum defined.
  Each Transmitter/Receiver can have its own protocol,
  which is derived from the base protocol.
  
  The library uses Timer 0 (8 bit) in CTC mode for the
  communication, so one must be careful when manipulating
  timers. If necessary, Timer definitions can be easily
  changed to use with other timers (perhaps for the next
  version, and we surely appreciate code suggestions).
  
  NOTE: this library is currently valid only for wired
  transmissions (1 wire + GND), and a newer version is
  intended to be compatible with RF transmissions (must
  support modulation and/or noise suppression).
  
*/


#include "SimpleCom.h"
#include <math.h> //only for abs()


//---------------------------------------------------------------------------------------------------------------------

// *************************************************************************
// ********************** Transmitters & Receivers *************************
// *************************************************************************

uint8_t ReceiversNumber = 0; //DO NOT change outside this library
SCreceiver *Receivers[SC_MAX_RECEIVERS];

uint8_t TransmittersNumber = 0; //DO NOT change outside this library
SCtransmitter *Transmitters[SC_MAX_TRANSMITTERS];

void printlala(void){ //TESTE
      Serial.print("Rec: ");
      Serial.println(ReceiversNumber);
      Serial.print("Tr: ");
      Serial.println(TransmittersNumber);
}


// Add a Receiver to the list
//  (returns the number of receivers or 0 if not added)
uint8_t AddReceiver(SCreceiver *receiver){
  if(ReceiversNumber < SC_MAX_RECEIVERS){
    Receivers[ReceiversNumber++] = receiver;
  } else {
    return 0;
  }
  return ReceiversNumber;
}

// -------------------------------------------------------------------------

// Add a Transmitter to the list
//  (returns the number of transmitters or 0 if not added)
uint8_t AddTransmitter(SCtransmitter *transmitter){
  if(TransmittersNumber < SC_MAX_TRANSMITTERS){
    Transmitters[TransmittersNumber++] = transmitter;
  } else {
    return 0;
  }
  return TransmittersNumber;
}

// -------------------------------------------------------------------------

// Remove a Receiver from the list
//  (returns 0 if the receiver was not found, 1 otherwise)
uint8_t RemoveReceiver(SCreceiver *receiver){
  uint8_t found = 0;
  for(uint8_t i=0 ; i < ReceiversNumber ; i++){
    if(Receivers[i] == receiver){
      if(i < (ReceiversNumber - 1)){
        Receivers[i]->Stop(); //stop the receiver
        Receivers[i] = Receivers[ReceiversNumber - 1]; //replace by last one
        Receivers[ReceiversNumber - 1] = NULL; //remove last one
        ReceiversNumber--; //update counter
        found = 1; //found something
      } else { //is last index
        Receivers[i]->Stop(); //stop the receiver
        Receivers[i] = NULL; //remove
        ReceiversNumber--; //update counter
        found = 1; //found something
      }
    }
  }
  
  return found;
}

// -------------------------------------------------------------------------

// Remove a Transmitter from the list
//  (returns 0 if the receiver was not found, 1 otherwise)
uint8_t RemoveTransmitter(SCtransmitter *transmitter){
  uint8_t found = 0;
  for(uint8_t i=0 ; i < TransmittersNumber ; i++){
    if(Transmitters[i] == transmitter){
      if(i < (TransmittersNumber - 1)){
        Transmitters[i]->Stop(); //stop the transmitter
        Transmitters[i] = Transmitters[TransmittersNumber - 1]; //replace by last one
        Transmitters[TransmittersNumber - 1] = NULL; //remove last one
        TransmittersNumber--; //update counter
        found = 1; //found something
      } else { //is last index
        Transmitters[i]->Stop(); //stop the transmitter
        Transmitters[i] = NULL; //remove
        TransmittersNumber--; //update counter
        found = 1; //found something
      }
    }
  }
  
  return found;
}

//---------------------------------------------------------------------------------------------------------------------

// *************************************************************************
// ******************************* TIMER ***********************************
// *************************************************************************

uint8_t SC_TIMER_STARTED = 0; // 1 if started (DO NOT change from outside this library)

// Configure Timer 0 to reset on compare for 10 us

//  TCCR0B = 0x00; .............. Disable Timer0 while we set it up
//  TCNT0 = 0; .................. Reset counter
//  OCR0A = 160; ................ Set compare to 160
//  TIFR0  = 0x00; .............. Timer0 INT Flag Reg: Clear Timer Compare A Flag
//  TIMSK0 = 0x02; .............. Timer0 INT Reg: Timer0 Compare A Interrupt Enable
//  TCCR0A = 0x00; .............. Timer0 Control Reg A: CTC operation, Wave Gen Mode normal
//  TCCR0B = 0x01; .............. Timer0 Control Reg B: Timer Prescaler set to 1 (and Timer ON)


//configure Timer 0 (8 bits) to 10 us
//  (Arduino > 16 MHz and Prescaler de 1)
#define TIMER0_CONFIGURE() ({ \
  TIMSK0 = 0x00; \
  TCCR0B = T_PRESCALER; \
  TCCR0A = 0x02; \
  TCNT0  = 0; \
  OCR0A = T_OCR0A; \
  TIFR0  = 0x00; \
  TIMSK0 = 0x02; \
})

#define TIMER0_DISABLE (TIMSK0 = 0x00)

#define TIMER0_ENABLE (TIMSK0 = 0x02)

#define TIMER0_RESET ({ \
  TIFR0  = 0x00; \
})

// -------------------------------------------------------------------------

// Timer Compare Interrupt
//   Handles communication >> Transmitter also uses timer because receiver
//                            interrupt can cause interference if using delay
ISR(TIMER0_COMPA_vect){
  // do not disable & enable timer here, because frequency must remain constant
  
  // send signals
  for(uint8_t i=0 ; i < TransmittersNumber ; i++){
    Transmitters[i]->Transmit();
  }
  
  // handle incoming signals
  for(uint8_t i=0 ; i < ReceiversNumber ; i++){
    Receivers[i]->Receive();
  }
}



//---------------------------------------------------------------------------------------------------------------------

// *************************************************************************
// ************************** SC Transmitter *******************************
// *************************************************************************

// Constructor (default)
SCtransmitter::SCtransmitter(void){
  _initialized = 0;
  _state = SC_STATE_EMPTY;
  
  //do not set PIN
  _id = 0;
  _channel = SC_DEFAULT_CHANNEL; //set channel
  _state = SC_STATE_IDLE;
  _start_duration_high = SC_DEFAULT_START_DURATION_HIGH;
  _start_duration_low = SC_DEFAULT_START_DURATION_LOW;
  _duration_high = SC_DEFAULT_DURATION_HIGH;
  _duration_low = SC_DEFAULT_DURATION_LOW;
  _buffer_length = 0;
}

//---------------

// Constructor
SCtransmitter::SCtransmitter(uint8_t pin){
  _initialized = 0;
  _state = SC_STATE_EMPTY;
  Create(pin);
}

// -------------------------------------------------------------------------

// Destructor
SCtransmitter::~SCtransmitter(void){
  Stop(); //just to be sure
  
  //remove from the Transmitters list
  RemoveTransmitter(this);
}

// -------------------------------------------------------------------------

// Create the transmitter (for when calling the default constructor)
void SCtransmitter::Create(uint8_t pin){
  //check if initialized - set only once
  if(_initialized)
    return;
  
  //add to the Transmitters list
  if(!AddTransmitter(this))
    return;
  
  _initialized = 1;
  
  _pin = pin; //set pin
  pinMode(_pin, OUTPUT); //set as output
  _id = 0;
  _channel = SC_DEFAULT_CHANNEL; //set channel
  _state = SC_STATE_IDLE;
  _start_duration_high = SC_DEFAULT_START_DURATION_HIGH;
  _start_duration_low = SC_DEFAULT_START_DURATION_LOW;
  _duration_high = SC_DEFAULT_DURATION_HIGH;
  _duration_low = SC_DEFAULT_DURATION_LOW;
  _buffer_length = 0;
  digitalWrite(_pin, LOW); //send idle value
}

// -------------------------------------------------------------------------

// Get the channel of the transmission
uint8_t SCtransmitter::GetChannel(void){
  return _channel;
}

// -------------------------------------------------------------------------

// Get the high time duration for the ONE interval in [us]
uint16_t SCtransmitter::GetDurationHIGH(void){
  return _duration_high;
}

// -------------------------------------------------------------------------

// Get the high time duration for the ONE interval in [us]
uint16_t SCtransmitter::GetDurationLOW(void){
  return _duration_low;
}

// -------------------------------------------------------------------------

// Get the associated pin
uint8_t SCtransmitter::GetPin(void){
  return _pin;
}

// -------------------------------------------------------------------------

// Get the HIGH duration for the start signal in [us]
uint16_t SCtransmitter::GetStartDurationHIGH(void){
  return _start_duration_high;
}

// -------------------------------------------------------------------------

// Get the LOW duration for the start signal in [us]
uint16_t SCtransmitter::GetStartDurationLOW(void){
  return _start_duration_low;
}

// -------------------------------------------------------------------------

// Get the state
uint8_t SCtransmitter::GetState(void){
  return _state;
}

// -------------------------------------------------------------------------

// Check if is sending
//  (returns 1 if sending, 0 otherwise)
uint8_t SCtransmitter::isSending(void){
  //check if initialized
  if(!_initialized)
    return 0;
  
  //check state
  if(_state == SC_STATE_SENDING)
    return 1;
  else
    return 0;
}

// -------------------------------------------------------------------------

// Send the message with given length
//  (returns 1 on start of transmission, -1 if not initialized,
//    -2 if invalid ID, -3 if invalid channel, -4 if invalid length)
int8_t SCtransmitter::Send(uint8_t *message, uint8_t length){
  //check if initialized
  if(!_initialized)
    return -1;
  
  //check id
  if(_id == 0)
    return -2;
  if((_id & 0xF) == 0)
    return -2;
    
  //check channel
  if(_channel == 0)
    return -3;
  if((_channel & 0xF) == 0)
    return -3;
  
  //check message size
  if(length > SC_MESSAGE_SIZE)
    return -4;
  
  //start timer if necessary
  if(!SC_TIMER_STARTED)
    SC_Start_Timer();
  
  //create message
  _buffer[0] = ((_id & 0x0F) << 4); //msb
  _buffer[0] |= (_channel & 0x0F); //lsb
  _buffer[1] = length;
  for(uint8_t i=0 ; i < length ; i++)
    _buffer[i+2] = message[i];
  _buffer[length + 2] = SC_CheckSum(message, length);
  _buffer_length = length + 3;
  
  _elapsed_time = 0; //reset
  _state = SC_STATE_SENDING; //set state
  _signal_state = SC_START; //set initial signal to send
  _signal = HIGH; //set for the 1st time
  
  return 1;
}

// -------------------------------------------------------------------------

// Set the channel to transmit
void SCtransmitter::SetChannel(uint8_t channel){
  _channel = (channel & 0xF); //assign only 4 bits
}

// -------------------------------------------------------------------------

// Set the ID of the receiver
void SCtransmitter::SetID(uint8_t id){
  _id = (id & 0xF); //asssign only 4 bits
}

// -------------------------------------------------------------------------

// Set the high and low times for the ONE signal in [us]
//  (returns 0 on invalid values or 1 if successful)
//  NOTE: must call Send() again after changing the values
//  NOTE: ZERO signal has the same times but with the order inverted
//  NOTE: all values must match the correspondant values in SCtransmitter
//  NOTE: the functions automatically corrects values if they are
//          incompatible with library definitions
uint8_t SCtransmitter::SetInterval(uint16_t high_time, uint16_t low_time){
  //check values
  if((high_time + low_time) < SC_MIN_DURATION)
    return 0;
  if(high_time < SC_MIN_DURATION_INTERVAL)
    return 0;
  if(low_time < SC_MIN_DURATION_INTERVAL)
    return 0;
  
  Stop(); //stop the transmission before changing values
  
  //assign duration high
  if(high_time > SC_SIGNAL_MAX_TIME)
    _duration_high = SC_SIGNAL_MAX_TIME;
  else
    _duration_high = high_time;
  //assign duration low
  if(low_time > SC_SIGNAL_MAX_TIME)
    _duration_low = SC_SIGNAL_MAX_TIME;
  else
    _duration_low = low_time;
  
  //check difference
  if(abs(_duration_high - _duration_low) < (2 * SC_SIGNAL_DEVIATION)){
    if(_duration_high >= _duration_low){
      _duration_low = _duration_high - 2 * SC_SIGNAL_DEVIATION;
      if(_duration_low < SC_MIN_DURATION_INTERVAL){ //check for low limit
        _duration_low = SC_MIN_DURATION_INTERVAL;
        _duration_high = _duration_low + 2 * SC_SIGNAL_DEVIATION;
      }
      if(_duration_high > SC_SIGNAL_MAX_TIME){ //SHOULD NEVER ENTER HERE !!! (means wrong value definitions)
        _state = SC_STATE_ERROR_DEFINITIONS;
        return 0;
      }
    } else {
      _duration_high = _duration_low - 2 * SC_SIGNAL_DEVIATION;
      if(_duration_high < SC_MIN_DURATION_INTERVAL){ //check for low limit
        _duration_high = SC_MIN_DURATION_INTERVAL;
        _duration_low = _duration_low + 2 * SC_SIGNAL_DEVIATION;
      }
      if(_duration_low > SC_SIGNAL_MAX_TIME) //SHOULD NEVER ENTER HERE !!! (means wrong value definitions)
        _state = SC_STATE_ERROR_DEFINITIONS;
        return 0;
    }
  }
  
  return 1;
}

// -------------------------------------------------------------------------

// Set the high and low times for the start signal in [us]
//  (returns 0 on invalid values or 1 if successful)
//  NOTE: must call Send() again after changing the values
//  NOTE: all values must match the correspondant values in SCtransmitter
//  NOTE: the functions automatically corrects values if they are
//          incompatible with library definitions
uint8_t SCtransmitter::SetStart(uint16_t high_time, uint16_t low_time){
  //check values
  if((high_time + low_time) < SC_MIN_START_DURATION)
    return 0;
  if(high_time < SC_MIN_START_INTERVAL)
    return 0;
  if(low_time < SC_MIN_START_INTERVAL)
    return 0;
  
  Stop(); //stop the transmission before changing values
  
  //assign duration high
  if(high_time > SC_SIGNAL_MAX_TIME)
    _start_duration_high = SC_SIGNAL_MAX_TIME;
  else
    _start_duration_high = high_time;
  //assign duration low
  if(low_time > SC_SIGNAL_MAX_TIME)
    _start_duration_low = SC_SIGNAL_MAX_TIME;
  else
    _start_duration_low = low_time;
  
  return 1;
}

// -------------------------------------------------------------------------

// Stop the communication
void SCtransmitter::Stop(void){
  _state = SC_STATE_IDLE; //reset
  digitalWrite(_pin, LOW); //reset signal
}

// -------------------------------------------------------------------------

// Transmit the message
void SCtransmitter::Transmit(void){
  /*
      1) check if sending
      2) update elapsed time
      3) send signal
      
      {ID+Chn, Len,   mes,    CS} --> (message to send)
      {0x11, 0x03, 1, 2, 3, 0x06} --> {00010001, 00000011, 00000001, 00000010, 00000011, 00000110}
  */
  
  //check if initialized
  if(!_initialized)
    return;
  
  //check state
  if(_state != SC_STATE_SENDING)
    return;
  
  _elapsed_time += SC_TIMER_INTERVAL; //update
  
  if(_signal_state == SC_START){ // send START
    digitalWrite(_pin, _signal);
    if((_elapsed_time >= _start_duration_high) && (_signal == HIGH)){ //finished with HIGH
      _elapsed_time = 0; //reset
      _signal = LOW; //next signal is LOW
    } else if ((_elapsed_time >= _start_duration_low) && (_signal == LOW)){ //finished with START
      _elapsed_time = 0; //reset
      _signal = HIGH; //next signal is HIGH
      _index = 0; //reset
      _bit = 7; //reset (start with msb)
      
      if(_index >= _buffer_length) //no more data
        Stop();
      else if(_buffer[_index] & (1 << _bit)) //next byte is 1
        _signal_state = SC_ONE;
      else //next byte is 0
        _signal_state = SC_ZERO;
    }
  } else if(_signal_state == SC_ONE){ // send ONE
    digitalWrite(_pin, _signal);
    if((_elapsed_time >= _duration_high) && (_signal == HIGH)){ //finished with HIGH
      _elapsed_time = 0; //reset
      _signal = LOW; //next signal is LOW
    } else if ((_elapsed_time >= _duration_low) && (_signal == LOW)){ //finished with ONE
      _elapsed_time = 0; //reset
      _signal = HIGH; //next signal is HIGH
      _bit--; //decrease
      
      //check for byte sent
      if(_bit < 0){
        _bit = 7;
        _index++;
      }
      
      //check what is the next data to send
      if(_index >= _buffer_length) //no more data
        Stop();
      else if(_buffer[_index] & (1 << _bit)) //next byte is 1
        _signal_state = SC_ONE;
      else //next byte is 0
        _signal_state = SC_ZERO;
    }
  } else if(_signal_state == SC_ZERO){ // send ZERO
    digitalWrite(_pin, _signal);
    if((_elapsed_time >= _duration_low) && (_signal == HIGH)){ //finished with HIGH
      _elapsed_time = 0; //reset
      _signal = LOW; //next signal is LOW
    } else if ((_elapsed_time >= _duration_high) && (_signal == LOW)){ //finished with ONE
      _elapsed_time = 0; //reset
      _signal = HIGH; //next signal is HIGH
      _bit--; //decrease
      
      //check for byte sent
      if(_bit < 0){
        _bit = 7;
        _index++;
      }
      
      //check what is the next data to send
      if(_index >= _buffer_length) //no more data
        Stop();
      else if(_buffer[_index] & (1 << _bit)) //next byte is 1
        _signal_state = SC_ONE;
      else //next byte is 0
        _signal_state = SC_ZERO;
    }
  } else { // end of transmission
    Stop();
  }
}


//---------------------------------------------------------------------------------------------------------------------

// *************************************************************************
// **************************** SC Receiver ********************************
// *************************************************************************

// Constructor (default)
SCreceiver::SCreceiver(){
  _initialized = 0;
  _state = SC_STATE_EMPTY;
  
  //do not set PIN
  _id = 0; //not initialized
  _channel = SC_DEFAULT_CHANNEL; //set channel
  _state = SC_STATE_IDLE;
  _signal[0] = 0;
  _signal[1] = 0;
  _start_duration_high = SC_DEFAULT_START_DURATION_HIGH;
  _start_duration_low = SC_DEFAULT_START_DURATION_LOW;
  _duration_high = SC_DEFAULT_DURATION_HIGH;
  _duration_low = SC_DEFAULT_DURATION_LOW;
  _buffer_length = 0;
}

//---------------

// Constructor
SCreceiver::SCreceiver(uint8_t pin, uint8_t id){
  _initialized = 0;
  _state = SC_STATE_EMPTY;
  Create(pin, id);
}

// -------------------------------------------------------------------------

// Destructor
SCreceiver::~SCreceiver(void){
  Stop();
  
  //remove from the Receivers list
  RemoveReceiver(this);
}

// -------------------------------------------------------------------------

// Manually reset buffer so one can identify when new message has arrived
//  (returns 0 if no message, 1 otherwise)
//  NOTE: returns the state to SC_STATE_LISTENNING
uint8_t SCreceiver::ClearBuffer(void){
  //only clear if there is a message
  if(_state != SC_STATE_MESSAGE_READY)
    return 0;
  
  for(uint8_t i=0 ; i < SC_TOTAL_MESSAGE_SIZE ; i++)
    _buffer[i] = 0;
  _buffer_length = 0;
  _state = SC_STATE_LISTENNING;
  _signal_state = 0;
  return 1;
}

// -------------------------------------------------------------------------

// Create the transmitter (for when calling the default constructor)
void SCreceiver::Create(uint8_t pin, uint8_t id){
  //check if initialized - set only once
  if(_initialized)
    return;
  
  //check id
  if(id == 0)
    return;
  if((id & 0xF) == 0)
    return;
  
  //add to the Receivers list
  if(!AddReceiver(this))
    return;
  
  _initialized = 1;
  
  _pin = pin; //set pin
  pinMode(_pin, INPUT); //set as input
  _id = (id & 0xF); //asssign only 4 bits
  _channel = SC_DEFAULT_CHANNEL; //set channel
  _state = SC_STATE_IDLE;
  _signal[0] = 0;
  _signal[1] = 0;
  _start_duration_high = SC_DEFAULT_START_DURATION_HIGH;
  _start_duration_low = SC_DEFAULT_START_DURATION_LOW;
  _duration_high = SC_DEFAULT_DURATION_HIGH;
  _duration_low = SC_DEFAULT_DURATION_LOW;
  _buffer_length = 0;
  
}

// -------------------------------------------------------------------------

// Get the channel of the transmission
uint8_t SCreceiver::GetChannel(void){
  return _channel;
}

// -------------------------------------------------------------------------

// Get the high time duration for the ONE interval in [us]
uint16_t SCreceiver::GetDurationHIGH(void){
  return _duration_high;
}

// -------------------------------------------------------------------------

// Get the high time duration for the ONE interval in [us]
uint16_t SCreceiver::GetDurationLOW(void){
  return _duration_low;
}

// -------------------------------------------------------------------------

// Get the ID of the receiver
uint8_t SCreceiver::GetID(void){
  return _id;
}

// -------------------------------------------------------------------------

// Get the message
//  (returns 0 if no message or 1 if successful)
uint8_t SCreceiver::GetMessage(uint8_t *buffer){
  //check if message available
  if(_state != SC_STATE_MESSAGE_READY)
    return 0;
  
  for(uint8_t i=2 ; i < (_buffer_length - 1) ; i++) //ignore (ID + Channel) & Length & CheckSum
    buffer[i-2] = _buffer[i];
  
  return 1;
}

// -------------------------------------------------------------------------

// Get the length of the message
//  (returns 0 if no message)
uint8_t SCreceiver::GetMessageLength(void){
  //check if message available
  if(_state != SC_STATE_MESSAGE_READY)
    return 0;
  
  return (_buffer_length - 3); //ignore (ID + Channel) & Length & CheckSum
}

// -------------------------------------------------------------------------

// Get the associated pin
uint8_t SCreceiver::GetPin(void){
  return _pin;
}

// -------------------------------------------------------------------------

// Get the HIGH duration for the start signal in [us]
uint16_t SCreceiver::GetStartDurationHIGH(void){
  return _start_duration_high;
}

// -------------------------------------------------------------------------

// Get the LOW duration for the start signal in [us]
uint16_t SCreceiver::GetStartDurationLOW(void){
  return _start_duration_low;
}

// -------------------------------------------------------------------------

// Get the state
uint8_t SCreceiver::GetState(void){
  return _state;
}

// -------------------------------------------------------------------------

// Check if is listenning
//  (returns 1 if listenning or there is a message, 0 otherwise)
uint8_t SCreceiver::isListenning(void){
  //check if initialized
  if(!_initialized)
    return 0;
  
  //check state
  if((_state == SC_STATE_LISTENNING) || (_state == SC_STATE_MESSAGE_READY))
    return 1;
  else
    return 0;
}

// -------------------------------------------------------------------------

// Start listenning to incoming messages
//  (returns 1 on start of transmission, -1 if not initialized,
//    -2 if invalid ID, -3 if invalid channel)
int8_t SCreceiver::Listen(void){
  //check if initialized
  if(!_initialized)
    return -1;
  
  //check id
  if(_id == 0)
    return -2;
  if((_id & 0xF) == 0)
    return -2;
    
  //check channel
  if(_channel == 0)
    return -3;
  if((_channel & 0xF) == 0)
    return -3;
  
  //start timer if necessary
  if(!SC_TIMER_STARTED)
    SC_Start_Timer();
  
  _state = SC_STATE_LISTENNING;
  _elapsed_time = 0; //set for the 1st time
  _previous_signal = LOW; //set for the 1st time
  _signal_state = 0; //set for the 1st time
  _buffer_length = 0; //reset
  
  return 1;
}

// -------------------------------------------------------------------------

// Receive message
void SCreceiver::Receive(void){
  /*
      1) check if listenning
      2) update elapsed time
      3) check for time overflow
      4) analyze data received (on transition)
        4.1) check for start signal (reset state & buffer_length)
        4.2) check for buffer overflow (when applicable)
        4.3) store value (when applicable)
        4.4) update signal value
      
      {ID+Chn, Len,   mes,    CS} --> (message to send)
      {0x11, 0x03, 1, 2, 3, 0x06} --> {00010001, 00000011, 00000001, 00000010, 00000011, 00000110}
  */
  
  //check if initialized
  if(!_initialized)
    return;
  
  //check state
  if((_state != SC_STATE_LISTENNING) && (_state != SC_STATE_MESSAGE_READY))
    return;
  
  _elapsed_time += SC_TIMER_INTERVAL; //update
  
  //check for time overflow
  if(_elapsed_time >= SC_SIGNAL_MAX_TIME){
    
    _elapsed_time = 0; //reset
    _previous_signal = LOW; //reset
    //check if is end of transmission
    if(_signal_state & SC_FOUND){
      if(abs(_signal[0] - _duration_high) <= SC_SIGNAL_DEVIATION){ // ONE
        //check for buffer overflow
        if(_buffer_length >= SC_TOTAL_MESSAGE_SIZE){
          _state = SC_STATE_ERROR_OVERFLOW;
          return;
        }
        //not overflow, continue
        _buffer[_buffer_length] |= (1 << _bit); //store value (bitwise OR)
        if(_bit <= 0){ //SHOULD ENTER HERE !
          _buffer_length++; //new byte
        }
      } else if(abs(_signal[0] - _duration_low) <= SC_SIGNAL_DEVIATION){ // ZERO
        //check for buffer overflow
        if(_buffer_length >= SC_TOTAL_MESSAGE_SIZE){
          _state = SC_STATE_ERROR_OVERFLOW;
          return;
        }
        //not overflow, continue
        _buffer[_buffer_length] &= ~(1 << _bit); //store value (bitwise AND + using NOT operator)
        if(_bit <= 0){ //SHOULD ENTER HERE !
          _buffer_length++; //new byte
        }
      }
    }
    //validate message if someting was found
    if((_signal_state & SC_FOUND) && (_state == SC_STATE_LISTENNING))
      ValidateMessage();
    _signal_state = 0; //reset
  
  } else { // WAIT NEXT CYCLE TO BEGIN because call to ValidateMessage() can be too much time consuming
  
  uint8_t signal = digitalRead(_pin);
  if(signal != _previous_signal){ //transition
    if((_previous_signal == LOW) && (_signal_state & SC_FOUND)){ //store LOW if already found something
      _signal[1] = _elapsed_time; //previous was LOW
      _elapsed_time = 0; //reset for next signal
      
      //check wich signal was found
      if((abs(_signal[0] - _start_duration_high) <= SC_SIGNAL_DEVIATION) && (abs(_signal[1] - _start_duration_low) <= SC_SIGNAL_DEVIATION)){ // START
        _signal_state = SC_START | SC_FOUND;
        _buffer_length = 0; //reset
        _bit = 7; //reset (start with msb)
        //set state if necessary (overwrite previous message)
        if(_state == SC_STATE_MESSAGE_READY)
          _state = SC_STATE_LISTENNING;
      } else if((abs(_signal[0] - _duration_high) <= SC_SIGNAL_DEVIATION) && (abs(_signal[1] - _duration_low) <= SC_SIGNAL_DEVIATION)){ // ONE        
        //check for buffer overflow
        if(_buffer_length >= SC_TOTAL_MESSAGE_SIZE){
          _state = SC_STATE_ERROR_OVERFLOW;
          return;
        }
        //not overflow, continue
        _signal_state = SC_ONE | SC_FOUND;
        _buffer[_buffer_length] |= (1 << _bit); //store value (bitwise OR)
        if(_bit <= 0){
          _bit = 7; //reset
          _buffer_length++; //new byte
        } else {
          _bit--; //decrease
        }
      } else if((abs(_signal[0] - _duration_low) <= SC_SIGNAL_DEVIATION) && (abs(_signal[1] - _duration_high) <= SC_SIGNAL_DEVIATION)){ // ZERO
        //check for buffer overflow
        if(_buffer_length >= SC_TOTAL_MESSAGE_SIZE){
          _state = SC_STATE_ERROR_OVERFLOW;
          return;
        }
        //not overflow, continue
        _signal_state = SC_ZERO | SC_FOUND;
        _buffer[_buffer_length] &= ~(1 << _bit); //store value (bitwise AND + using NOT operator)
        if(_bit <= 0){
          _bit = 7; //reset
          _buffer_length++; //new byte
        } else {
          _bit--; //decrease
        }
      }
    } else if((_previous_signal == LOW) && ((_signal_state & SC_FOUND) == 0)){ //found first signal
      _signal_state |= SC_FOUND;
      _elapsed_time = 0; //reset for next signal
    } else if((_previous_signal == HIGH) && (_signal_state & SC_FOUND)){ //store HIGH if already found something
      _signal[0] = _elapsed_time; //previous was HIGH
      _elapsed_time = 0; //reset for next signal
    }
    
    _previous_signal = signal; //update
  }
  
  } //end of WAIT NEXT CYCLE
}

// -------------------------------------------------------------------------

// Reset the receiver
void SCreceiver::Reset(void){
  Stop(); //stop the receiver
  _buffer_length = 0; //reset
}

// -------------------------------------------------------------------------

// Set the channel to transmit
void SCreceiver::SetChannel(uint8_t channel){
  _channel = (channel & 0xF); //assign only 4 bits
}

// -------------------------------------------------------------------------

// Set the high and low times for the ONE signal in [us]
//  (returns 0 on invalid values or 1 if successful)
//  NOTE: must call Listen() again after changing the values
//  NOTE: ZERO signal has the same times but with the order inverted
//  NOTE: all values must match the correspondant values in SCtransmitter
//  NOTE: the functions automatically corrects values if they are
//          incompatible with library definitions
uint8_t SCreceiver::SetInterval(uint16_t high_time, uint16_t low_time){
  //check values
  if((high_time + low_time) < SC_MIN_DURATION)
    return 0;
  if(high_time < SC_MIN_DURATION_INTERVAL)
    return 0;
  if(low_time < SC_MIN_DURATION_INTERVAL)
    return 0;
  
  Stop(); //stop the transmission before changing values
  
  //assign duration high
  if(high_time > SC_SIGNAL_MAX_TIME)
    _duration_high = SC_SIGNAL_MAX_TIME;
  else
    _duration_high = high_time;
  //assign duration low
  if(low_time > SC_SIGNAL_MAX_TIME)
    _duration_low = SC_SIGNAL_MAX_TIME;
  else
    _duration_low = low_time;
  
  //check difference
  if(abs(_duration_high - _duration_low) < (2 * SC_SIGNAL_DEVIATION)){
    if(_duration_high >= _duration_low){
      _duration_low = _duration_high - 2 * SC_SIGNAL_DEVIATION;
      if(_duration_low < SC_MIN_DURATION_INTERVAL){ //check for low limit
        _duration_low = SC_MIN_DURATION_INTERVAL;
        _duration_high = _duration_low + 2 * SC_SIGNAL_DEVIATION;
      }
      if(_duration_high > SC_SIGNAL_MAX_TIME){ //SHOULD NEVER ENTER HERE !!! (means wrong value definitions)
        _state = SC_STATE_ERROR_DEFINITIONS;
        return 0;
      }
    } else {
      _duration_high = _duration_low - 2 * SC_SIGNAL_DEVIATION;
      if(_duration_high < SC_MIN_DURATION_INTERVAL){ //check for low limit
        _duration_high = SC_MIN_DURATION_INTERVAL;
        _duration_low = _duration_low + 2 * SC_SIGNAL_DEVIATION;
      }
      if(_duration_low > SC_SIGNAL_MAX_TIME) //SHOULD NEVER ENTER HERE !!! (means wrong value definitions)
        _state = SC_STATE_ERROR_DEFINITIONS;
        return 0;
    }
  }
  
  return 1;
}

// -------------------------------------------------------------------------

// Set the high and low times for the start signal in [us]
//  (returns 0 on invalid values or 1 if successful)
//  NOTE: must call Listen() again after changing the values
//  NOTE: all values must match the correspondant values in SCtransmitter
//  NOTE: the functions automatically corrects values if they are
//          incompatible with library definitions
uint8_t SCreceiver::SetStart(uint16_t high_time, uint16_t low_time){
  //check values
  if((high_time + low_time) < SC_MIN_START_DURATION)
    return 0;
  if(high_time < SC_MIN_START_INTERVAL)
    return 0;
  if(low_time < SC_MIN_START_INTERVAL)
    return 0;
  
  Stop(); //stop the transmission before changing values
  
  //assign duration high
  if(high_time > SC_SIGNAL_MAX_TIME)
    _start_duration_high = SC_SIGNAL_MAX_TIME;
  else
    _start_duration_high = high_time;
  //assign duration low
  if(low_time > SC_SIGNAL_MAX_TIME)
    _start_duration_low = SC_SIGNAL_MAX_TIME;
  else
    _start_duration_low = low_time;
  
  return 1;
}

// -------------------------------------------------------------------------

// Stop the communication
void SCreceiver::Stop(void){
  _state = SC_STATE_IDLE; //reset
}

// -------------------------------------------------------------------------

// Stop the communication
uint8_t SCreceiver::ValidateMessage(void){
  /*
    [0] - ID (msb) & Channel (lsb)
    [1] - length
    [2-n] - message
    [n+1] - check sum
  */
  
//  if(!_buffer_length) //TEST
//    return 0; //TEST
  
  _state = SC_STATE_VALIDATING;
  
  if(((_buffer[0] & 0xF0) >> 4) == _id){ //check ID
    if((_buffer[0] & 0x0F) == _channel){ //check Channel
      if(_buffer[1] == _buffer_length - 3){ //check length (subtract ID+Channel & Length & CheckSum)
        //create temporary buffer to calculate the checksum
        uint8_t temp[_buffer[1]];
        //store temporary data
        for(uint8_t i=2 ; i < (_buffer_length - 1) ; i++) //ignore (ID + Channel) & Length & CheckSum
          temp[i-2] = _buffer[i];
        if(SC_CheckSum(temp, _buffer[1]) == _buffer[_buffer_length - 1]){ //check CheckSum
          //store the message as it is >> see GetMessage() for reference
          _state = SC_STATE_MESSAGE_READY;
          return 1;
        }
      }
    }
  }
  
  _state = SC_STATE_LISTENNING; //reset
  return 0;
}


//---------------------------------------------------------------------------------------------------------------------

// *************************************************************************
// ************************** Other Functions ******************************
// *************************************************************************

// Simple CheckSum
uint8_t SC_CheckSum(uint8_t *message, uint8_t length){
  uint16_t cs = 0;
  for(uint8_t i=0 ; i < length ; i++)
    cs += message[i];
  return (cs & 0xFF);
}

// -------------------------------------------------------------------------

void SC_Start_Timer(void){
  TIMER0_CONFIGURE(); //configure timer
  SC_TIMER_STARTED = 1; //set
  TIMER0_ENABLE; //start timer
}

// -------------------------------------------------------------------------

void SC_Stop_Timer(void){
  TIMER0_DISABLE; //stop timer
  SC_TIMER_STARTED = 0; //reset
}

// -------------------------------------------------------------------------



