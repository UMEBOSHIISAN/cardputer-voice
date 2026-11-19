#pragma once
#include <USB.h>
#include <esp32-hal-tinyusb.h>
#include <tusb.h>
#include "midi.hpp"

static constexpr char instrumentName[]="Cardputer Voice";
inline uint16_t midiDescriptor(uint8_t* dst, uint8_t* interfaceNumber) {
    const uint8_t in=tinyusb_get_free_in_endpoint(), out=tinyusb_get_free_out_endpoint();
    if (!in || !out) return 0;
    uint8_t label=tinyusb_add_string_descriptor(instrumentName);
    const uint8_t data[]={TUD_MIDI_DESCRIPTOR(*interfaceNumber,label,out,uint8_t(in|0x80),64)};
    memcpy(dst,data,sizeof(data)); *interfaceNumber+=2; return sizeof(data);
}
inline bool startMidiUsb() {
    USB.productName(instrumentName);
    USB.usbClass(0); USB.usbSubClass(0); USB.usbProtocol(0);
    return tinyusb_enable_interface(USB_INTERFACE_MIDI,TUD_MIDI_DESC_LEN,midiDescriptor)==ESP_OK && USB.begin();
}
