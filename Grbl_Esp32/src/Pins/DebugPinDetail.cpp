/*
    Part of Grbl_ESP32
    2021 -  Stefan de Bruijn

    Grbl_ESP32 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Grbl_ESP32 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Grbl_ESP32.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "DebugPinDetail.h"

#include "../Grbl.h"         // for printf
#include <Arduino.h>         // for timer
#include <HardwareSerial.h>  // HW serial
#include <cstdio>            // vsnprintf

namespace Pins {
    inline void WriteSerial(const char* format, ...) {
        char    buf[50];
        va_list arg;
        va_list copy;
        va_start(arg, format);
        va_copy(copy, arg);
        size_t len = vsnprintf(buf, 50, format, arg);
        va_end(copy);
        Serial.print("[MSG: ");
        Serial.print(buf);
        Serial.println(" ]");
        va_end(arg);
    }

    // I/O:
    void DebugPinDetail::write(int high) {
        if (high != _isHigh) {
            _isHigh = high;
            if (shouldEvent()) {
                WriteSerial("Write %s < %d", toString().c_str(), high);
            }
        }
        _implementation->write(high);
    }

    int DebugPinDetail::read() {
        auto result = _implementation->read();
        if (shouldEvent()) {
            WriteSerial("Read  %s > %d", toString().c_str(), result);
        }
        return result;
    }
    void DebugPinDetail::setAttr(PinAttributes value) {
        char buf[10];
        int  n = 0;
        if (value.has(PinAttributes::Input)) {
            buf[n++] = 'I';
        }
        if (value.has(PinAttributes::Output)) {
            buf[n++] = 'O';
        }
        if (value.has(PinAttributes::PullUp)) {
            buf[n++] = 'U';
        }
        if (value.has(PinAttributes::PullDown)) {
            buf[n++] = 'D';
        }
        if (value.has(PinAttributes::ISR)) {
            buf[n++] = 'E';
        }
        if (value.has(PinAttributes::Exclusive)) {
            buf[n++] = 'X';
        }
        if (value.has(PinAttributes::InitialOn)) {
            buf[n++] = '+';
        }
        buf[n++] = 0;

        if (shouldEvent()) {
            WriteSerial("Set pin attr %s = %s", toString().c_str(), buf);
        }
        _implementation->setAttr(value);
    }

    PinAttributes DebugPinDetail::getAttr() const { return _implementation->getAttr(); }

    void DebugPinDetail::CallbackHandler::handle(void* arg) {
        auto handler = static_cast<CallbackHandler*>(arg);
        if (handler->_myPin->shouldEvent()) {
            WriteSerial("Received ISR on %s", handler->_myPin->toString().c_str());
        }
        handler->callback(handler->argument);
    }

    // ISR's:
    void DebugPinDetail::attachInterrupt(void (*callback)(void*), void* arg, int mode) {
        _isrHandler._myPin   = this;
        _isrHandler.argument = arg;
        _isrHandler.callback = callback;

        if (shouldEvent()) {
            WriteSerial("Attaching interrupt to pin %s, mode %d", toString().c_str(), mode);
        }
        _implementation->attachInterrupt(_isrHandler.handle, &_isrHandler, mode);
    }
    void DebugPinDetail::detachInterrupt() { _implementation->detachInterrupt(); }

    bool DebugPinDetail::shouldEvent() {
        // This method basically ensures we don't flood users:
        auto time = millis();

        if (_lastEvent + 1000 < time) {
            _lastEvent  = time;
            _eventCount = 1;
            return true;
        } else if (_eventCount < 10) {
            _lastEvent = time;
            ++_eventCount;
            return true;
        } else if (_eventCount == 10) {
            _lastEvent = time;
            ++_eventCount;
            WriteSerial("Suppressing events...");
            return false;
        } else {
            _lastEvent = time;
            return false;
        }
    }
}
