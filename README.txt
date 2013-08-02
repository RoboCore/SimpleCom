
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








