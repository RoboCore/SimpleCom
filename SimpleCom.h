#ifndef RC_SIMPLE_COM_H
#define RC_SIMPLE_COM_H

/*

	RoboCore SimpleCom Library
		(v1.0 - 28/03/2013)

  Simple Communication functions for Arduino
    (tested with Arduino 1.0.1)

  Released under the Beerware license
  Written by François
  
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


#if defined(ARDUINO) && (ARDUINO >= 100)
#include <Arduino.h> //for Arduino 1.0 or later
#else
#include <WProgram.h> //for Arduino 22
#endif

/*
            Timer Tunning
  
  - SC_SIGNAL_DEVIATION : depends on the value of SC_TIMER_INTERVAL (must be greater than this value)
  - SC_TIMER_INTERVAL : increase if using too many Transmitters & Receivers
                        if too high, signal loses precision, must therefore increase Default values & Deviation
                        if too low, cannot handle all signals.
  - SC_DEFAULT_x : depend on the value of SC_SIGNAL_DEVIATION (recommended to be a multiple of this value)
  - SC_MAX_RECEIVERS : timer interrupt cannot handle too many
  - SC_MAX_TRANSMITTERS : timer interrupt cannot handle too many
  
  - Signal duration is recommended to be a multiple of SC_TIMER_INTERVAL
*/


// state of the transmitter/receiver
#define SC_STATE_EMPTY 0
#define SC_STATE_IDLE 1
#define SC_STATE_SENDING 2
#define SC_STATE_LISTENNING 3
#define SC_STATE_MESSAGE_READY 4
#define SC_STATE_VALIDATING 5 //when validating messages
#define SC_STATE_ERROR_DEFINITIONS 254 //wrong values in library definitions
#define SC_STATE_ERROR_OVERFLOW 255 //buffer overflow


// signal value
#define SC_ZERO 0
#define SC_ONE 1
#define SC_START 2
#define SC_FOUND 0x80

// Signal Constants
#define SC_SIGNAL_DEVIATION 100 //deviation of the signal value in [us]
#define SC_SIGNAL_MAX_TIME 65530 //because of uint16_t

// MINIMUM values
#define SC_MIN_DURATION 400
#define SC_MIN_DURATION_INTERVAL (2 * SC_SIGNAL_DEVIATION) //must consider deviation
#define SC_MIN_START_DURATION 1000
#define SC_MIN_START_INTERVAL (3 * SC_SIGNAL_DEVIATION) //must consider deviation

// DEFAULT values
#define SC_DEFAULT_CHANNEL 0x1 //ONLY 4 bits
#define SC_DEFAULT_DURATION_HIGH 700
#define SC_DEFAULT_DURATION_LOW 400
#define SC_DEFAULT_START_DURATION_HIGH 4000
#define SC_DEFAULT_START_DURATION_LOW 2000

// size of the message & of the buffer
#define SC_MESSAGE_SIZE 30 //in bytes
#define SC_TOTAL_MESSAGE_SIZE (SC_MESSAGE_SIZE + 3) //include (ID + Channel) + (message_length) + (CheckSum)



// TIMER definitions ----------
#ifndef F_CPU
#error F_CPU not defined!
#endif

#define SC_TIMER_INTERVAL 100 // in [us]

#if ((SC_TIMER_INTERVAL * F_CPU / 1000000) > 255)          //prescaler of 8
#define T_OCR0A (SC_TIMER_INTERVAL * F_CPU / 1000000 / 8)
#define T_PRESCALER 0x02
#else                                                      //prescaler of 1
#define T_OCR0A (SC_TIMER_INTERVAL * F_CPU / 1000000)
#define T_PRESCALER 0x01
#endif



//---------------------------------------------------------------------------------------------------------------------

class SCtransmitter{
  private:
    uint8_t _initialized; // TRUE if initialized (pins and id set)
    uint8_t _pin;
    uint8_t _id; // [1 - 15] # 0 means no destination (is SET for each transmission, depends of target Receiver's ID)
    uint8_t _channel; // [1 - 15] # 0 means no channel
    uint8_t _state; // the state of the transmitter
    
    uint16_t _start_duration_high;
    uint16_t _start_duration_low;
    
    uint16_t _duration_high;
    uint16_t _duration_low;
    
    uint16_t _elapsed_time; // used to get values
    uint8_t _signal; //signal to send
    uint8_t _signal_state; // signal state
    
    uint8_t _buffer[SC_TOTAL_MESSAGE_SIZE];
    uint8_t _buffer_length;
    uint8_t _index; //index of the message to send
    int8_t _bit; //bit of the index to send
  
  public:
    SCtransmitter(void);
    SCtransmitter(uint8_t pin);
    ~SCtransmitter(void);
    
    void Create(uint8_t pin); //for when the default constructor is called
    
    uint8_t GetChannel(void);
    uint16_t GetDurationHIGH(void);
    uint16_t GetDurationLOW(void);
    uint8_t GetPin(void);
    uint16_t GetStartDurationHIGH(void);
    uint16_t GetStartDurationLOW(void);
    uint8_t GetState(void);
    
    uint8_t isSending(void);
    int8_t Send(uint8_t *message, uint8_t length);

    void SetChannel(uint8_t channel); //set the channel of the communication
    void SetID(uint8_t id); //set the id of the receiver
    uint8_t SetInterval(uint16_t high_time, uint16_t low_time);
    uint8_t SetStart(uint16_t high_time, uint16_t low_time);
    
    void Stop(void);
    void Transmit(void); //DO NOT call from outside the library (is public because of timer interrupt)
};






//---------------------------------------------------------------------------------------------------------------------

class SCreceiver{
  private:
    uint8_t _initialized; // TRUE if initialized (pins and id set)
    uint8_t _pin;
    uint8_t _id; // [1 - 15] # 0 means not initialized (is FIXED for the Receiver)
    uint8_t _channel; // [1 - 15] # 0 means no channel
    uint8_t _state; // the state of the receiver
    
    uint16_t _start_duration_high;
    uint16_t _start_duration_low;
    
    uint16_t _duration_high;
    uint16_t _duration_low;
    
    uint16_t _elapsed_time; // used to get values
    uint16_t _signal[2]; // the times of the signal [0 - HIGH ; 1 - LOW]
    uint8_t _previous_signal; // the previous value received
    uint8_t _signal_state; // signal state + (byte 8) to check if ignore previous signal
    
    uint8_t _buffer[SC_TOTAL_MESSAGE_SIZE];
    uint8_t _buffer_length;
    int8_t _bit; //bit of the index received
    
    uint8_t ValidateMessage(void); //called when Receive() has finished
  
  public:
    SCreceiver(void);
    SCreceiver(uint8_t pin, uint8_t id);
    ~SCreceiver(void);
    
    uint8_t ClearBuffer(void); //manually reset buffer so one can identify when new message has arrived
    void Create(uint8_t pin, uint8_t id); //for when the default constructor is called
    
    uint8_t GetChannel(void);
    uint16_t GetDurationHIGH(void);
    uint16_t GetDurationLOW(void);
    uint8_t GetID(void);
    uint8_t GetMessage(uint8_t *buffer);
    uint8_t GetMessageLength(void);
    uint8_t GetPin(void);
    uint16_t GetStartDurationHIGH(void);
    uint16_t GetStartDurationLOW(void);
    uint8_t GetState(void);
    
    uint8_t isListenning(void);
    int8_t Listen(void);
    void Receive(void); //DO NOT call from outside the library (is public because of timer interrupt)
    void Reset(void); //stop the communication and reset the buffer length
    
    void SetChannel(uint8_t channel);
    uint8_t SetInterval(uint16_t high_time, uint16_t low_time);
    uint8_t SetStart(uint16_t high_time, uint16_t low_time);
    
    void Stop(void);
};


//---------------------------------------------------------------------------------------------------------------------

const uint8_t SC_MAX_RECEIVERS = 5;
uint8_t AddReceiver(SCreceiver *receiver);
uint8_t RemoveReceiver(SCreceiver *receiver);

const uint8_t SC_MAX_TRANSMITTERS = 5;
uint8_t AddTransmitter(SCtransmitter *transmitter);
uint8_t RemoveTransmitter(SCtransmitter *transmitter);


uint8_t SC_CheckSum(uint8_t *message, uint8_t length);
void SC_Start_Timer(void);
void SC_Stop_Timer(void);


//---------------------------------------------------------------------------------------------------------------------

#endif //RC_SIMPLE_COM_H


