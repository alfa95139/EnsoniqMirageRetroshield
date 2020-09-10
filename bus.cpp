
#include "bus.h"

#include "via.h"
#include "fdc.h"
#include <stdint.h>
#include <Arduino.h>

////////////////////////////////////////////////////////////////////
// Monitor Code
////////////////////////////////////////////////////////////////////
#include "EnsoniqRom.h"
#include "CartridgeROM.h"

////////////////////////////////////////////////////////////////////
// Debug Vectors
////////////////////////////////////////////////////////////////////
#define fdccmd   0x8000
#define fdcrtry  0x8001
#define fdctrk   0x8002
#define fdcsect  0x8003
#define fdcbuff  0x8004
#define fdcstat  0x8006
#define fdcerr   0x8007
#define var1    0xbf70
#define var2    0xbf71
#define var3    0xbf72
#define var4    0xbf73
#define var5    0xbf74
#define var6    0xbf75
#define var7    0xbf76
#define var8    0xbf77
#define var9    0xbf78
#define var10   0xbf79
#define var11   0xbf7a
#define var12   0xbf7b
#define var13   0xbf7c
#define var14   0xbf7d
#define var15   0xbf7e
#define var16   0xbf7f
#define var17   0xbf80
#define var18   0xbf81
#define var19   0xbf82
#define var20   0xbf83
#define var21   0xbf84
#define var22   0xbf85
#define var23   0xbf8c

#define firqvec    0x800B
#define osvec     0x800E // location of osentry
#define irqentry    0x893C // IRQ service routine in OS 3.2
#define firqentry   0xA151 // FIRQ Service Routine
#define osentry   0xB920 // OS 3.2 OS Entry point
#define fdcreadsector   0xF000
#define fdcskipsector   0xF013
#define fdcwritesector  0xF024
#define fdcfillsector   0xF037
#define fdcreadtrack    0xF04A
#define fdcwritetrack   0xF058
#define fdcrestore    0xF066
#define fdcseektrack    0xF06F
#define fdcseekin   0xF07D
#define fdcseekout    0xF086
#define fdcforceinterrupt  0xF08F
#define countdown   0xF0A7
#define nmivec    0xF0B0
#define coldstart   0xF0F0
#define runopsys    0xF146
#define hwsetup   0xF15D
#define qchipsetup    0xF1BB
#define clearram    0xF1E5
#define loadopsys   0xF20D // load OS in PROGRAM RAM
#define readsysparams   0xF2AF
#define checkos   0xF306
#define showerrcode   0xF33C
#define preparefd   0xF38C
#define loadossector    0xF3AC
#define gototrack   0xF3F1
#define seterrcode    0xF413
#define saveparams    0xF425
#define restoreparams   0xF437
#define readsector    0xF448
#define writesector   0xF476
#define gototrack2    0xF4A4
#define enablefd    0xF4C6
#define disablefd   0xF4D6

uint8_t WAV_RAM[4][WAV_END - WAV_START+1];

uint8_t PRG_RAM[RAM_END - RAM_START+1];
int page = 0;

const char* address_name(uint16_t address) {
  if (address == loadopsys)           return "LOAD OS IN PRG RAM";
  if (address == osentry)             return "*OS ENTRY";
  if (address == irqentry)            return "IRQ INTERRUPT ROUTINE ENTRY POINT";
  if (address == firqentry)           return "FIRQ INTERRUPT ROUTINE ENTRY POINT";
  if (address == firqvec)             return "firqvec";
  //if (address == irqvec)              return "irqvec";
  if (address == osvec)               return "*osvec"; 
  if (address == fdcreadsector)       return "fdcreadsector"; 
  if (address == fdcskipsector)       return "fdcskipsector"; 
  if (address == fdcwritesector)      return "fdcwritesector"; 
  if (address == fdcfillsector)       return "fdcfillsector"; 
  if (address == fdcreadtrack)        return "fdcreadtrack"; 
  if (address == fdcwritetrack)       return "fdcwritetrack"; 
  if (address == fdcrestore)          return "fdcrestore"; 
  if (address == fdcseektrack)        return "fdcseektrack"; 
  if (address == fdcseekin)           return "fdcseekin"; 
  if (address == fdcseekout)          return "fdcseekout"; 
  if (address == fdcforceinterrupt)   return "fdcforceinterrupt"; 
  if (address == countdown)           return "countdown"; 
  if (address == nmivec)              return "nmivec"; 
  if (address == coldstart)           return "coldstart"; 
  if (address == runopsys)            return "*runopsys"; 
  if (address == hwsetup)             return "hwsetup"; 
  if (address == qchipsetup)          return "qchipsetup"; 
  if (address == clearram)            return "clearram"; 
  if (address == readsysparams)       return "readysysparams"; 
  if (address == checkos)             return "checkos"; 
  if (address == showerrcode)         return "showerrorcode"; 
  if (address == preparefd)           return "preparefd"; 
  if (address == loadossector)        return "loadossector"; 
  if (address == gototrack)           return "gototrack";
  if (address == seterrcode)          return "seterrcode"; 
  if (address == saveparams)          return "saveparams"; 
  if (address == restoreparams)       return "restoreparams"; 
  if (address == readsector)          return "readsector"; 
  if (address == writesector)         return "writesector"; 
  if (address == gototrack2)          return "gototrack2"; 
  if (address == enablefd)            return "enablefd";
  if (address == disablefd)           return "disablefd";
  if (address & 0xFF00 == 0x7f00)     return "*cpucrash";

  if ((WAV_START <= address) && (address <= WAV_END)) return "wav data section";

  if ((address & 0xFF00) == VIA6522) return "VIA6522";
  if ((address & 0xFF00) == FDC1770) return "FDC1770";
  if ((address & 0xFF00) == DOC5503) return "DOC5503";
  if ((address & 0xFF00) == 0xE100)  return "ACIA";

  return "?";
}

extern bool debug_mode;
extern bool do_continue;

void CPU6809::write(uint16_t address, uint8_t data) {
  /*if (data == 0x7f && address > WAV_END && address < DEVICES_START) {
    Serial.printf("Wrote 0x%02x to %04x.\n", data, address);
    do_continue = false;
    debug_mode = true;
    set_debug(true);
  }*/
  // RAM?
  if ((RAM_START <= address) && (address <= RAM_END)) {
    PRG_RAM[address - RAM_START] = data;
    //Serial.printf("Writing to PRG_RAM, address = %04x : DATA = %02x\n", address, data);
    if(address == 0x800F) Serial.printf("************* WRITING OS ENTRY JMP 0x800F = %02X *********************\n", data);
    if(address == 0x8010) Serial.printf("*************                      0x8010 = %02X *********************\n", data);
    if(address == 0xBDEB) Serial.printf("************* WRITING TO 0xBDEB = %02X *********************\n", data);

  } else if ( (WAV_START <= address) && (address <= WAV_END) ) {
    // WAV RAM
    page = via_rreg(0) & 0b0011;
    WAV_RAM[page][address - WAV_START] = data;
    //Serial.printf("Writing to WAV RAM: PAGE %hhu address = %04x, DATA = %02x\n", page, address, data);

  } else if ( (address & 0xFF00) == VIA6522) {
    Serial.printf("Writing to VIA 6522 %04x %02x\n", address, data);
    via_wreg(address & 0xFF, data);

  } else if ( (address & 0xFF00) == FDC1770) {
    Serial.printf("Writing to FDC 1770 %04x %02x\n", address, data);
    fdc_wreg(address & 0xFF, data);

  } else if ((address & 0xFF00) == DOC5503) {
    Serial.printf("Writing to DOC: Register %02X\n", address & 0x00FF);

  } else if ((address & 0xFF00) == 0xE100) {
    // FTDI?
    Serial.printf("Writing to ACIA (not implemented) %c\n", data);
    //Serial.write(data);
  } else if ((address & 0xFF00) == 0xE400) {
    Serial.printf("=====>> FILTERS: %04x, %02x\n", address, data);
  }
}

uint8_t CPU6809::read(uint16_t address) {
  uint8_t out;

  // ROM?
  if ((ROM_START <= address) && (address <= ROM_END)) {
    out = ROM [ (address - ROM_START) ];

  } else if ((RAM_START <= address) && (address <= RAM_END)) {
    // RAM?

    const char* adrname = address_name(address);
    if (adrname[0] != '?')
      Serial.printf("  *** %04x : %s ***  \n", address, adrname);

    if (address == osvec)  Serial.printf("****  OS VEC ***  OS VEC ***  OS VEC ***  OS VEC ***  OS VEC %04x = %02x %02x\n", address, PRG_RAM[address - RAM_START+1], PRG_RAM[address - RAM_START +2] );
    //if (address == irqvec) Serial.printf("**** IRQ VEC *** IRQ VEC *** IRQ VEC *** IRQ VEC *** IRQ VEC %04x = %04x\n", address, PRG_RAM[address - RAM_START]);
    out = PRG_RAM[address - RAM_START];
  } else if ((WAV_START <= address) && (address <= WAV_END)) {
    // WAVE RAM? NOTE: VIA 6522 PORT B contains info about the page, when we will be ready to manage 4 banks
    page = via_rreg(0) & 0b0011;
    out = WAV_RAM[page][address - WAV_START];
  } else if ((CART_START <= address) && (address <= CART_END)) {
    //out = CartROM[ (address - CART_START) ];
    out = 0xFF; // we will enable when everything (but DOC5503) is working, we will need working UART
    Serial.printf("Reading from Expansion Port: address = %X, DATA = FF\n", address, out);

  } else if ((address & 0xFF00) == VIA6522) {
    // FTDI?
    //if ( (address & 0xFF00) == 0xE100) 
    //      out = FTDI_Read();
    //else
    Serial.printf("Reading from VIA 6522\n");
    out = via_rreg(address & 0xFF);

  } else if ((address & 0xFF00) == FDC1770) {
    out = fdc_rreg(address & 0xFF); 
    //Serial.printf("**** Reading from FDC 1770. Value ====> out = %02x\n", out);

  } else if ((address & 0xFF00) == DOC5503 ) {
    Serial.printf("Reading from DOC 5503: Register %02X\n", address & 0x00FF);
    out = 0xFF;
  } else if ((address & 0xFF00) == 0xE400) {
    Serial.printf("Reading from Filters addresses 0xE400 to 0xE41F (which is WRONG) ADDRESS = %04x\n", address);
  } else if ((CART_START <= address) && (address <= CART_END) ) {
    //DATA_OUT = CartROM[ (uP_ADDR - CART_START) ];
    out = 0xFF; // we will enable when everything (but DOC5503) is working, we will need working UART
    Serial.printf("Reading from Expansion Port: uP_ADDR = %X, DATA = FF\n", address, out);
  } else
    out = 0xFF;

  return out;
}

CPU6809::CPU6809()
{
  reset();
  clock_cycle_count = 0;
}

void CPU6809::tick()
{
  step();
  clock_cycle_count++;
}

extern bool emergency;

void CPU6809::invalid(const char* message) {
  Serial.print("CPU error detected: ");
  if (message)
    Serial.println(message);
  else
    Serial.println("No message specified");
  Serial.println("EMERGENCY.");
  printRegs();

  // stack trace
  Serial.printf("Stack:\n%04x:", s);
  for (uint16_t i = 0; i < 16; i++)
    Serial.printf(" %02x", read(s + i));
  Serial.println();

  // look for addresses in the stack
  for (uint16_t i = 0; i < 16; i++) {
    uint16_t entry = read_word(s + i);
    const char* entry_name = address_name(entry);
    if (entry_name[0] != '?')
      Serial.printf("[%04x] = %04x ~ %s\n", s + i, entry, entry_name);
  }
  
  Serial.flush();

  emergency = true;
}

void CPU6809::printRegs() {
  Serial.println("Register dump:");
  Serial.printf("IR %04x (%s)\n", ir, address_name(ir));
  Serial.printf("PC %04x (%s)\n", pc, address_name(pc));
  Serial.printf("U  %04x (%s)\n", u, address_name(u));
  Serial.printf("S  %04x (%s)\n", s, address_name(s));
  Serial.printf("X  %04x (%s)\n", x, address_name(x));
  Serial.printf("Y  %04x (%s)\n", y, address_name(y));
  Serial.printf("DP %02x\n", dp);
  Serial.printf("A  %02x\n", a);
  Serial.printf("B  %02x\n", b);
  Serial.printf("CC %02x\n", cc.all);
  Serial.println();
}


void CPU6809::on_branch(char* opcode, uint16_t src, uint16_t dst) {
  if (debug) {
    const char* name = address_name(dst);
    Serial.printf("branch with opcode %s from %04x to %04x\n", opcode, src, dst, name);
  }
}
void CPU6809::on_branch_subroutine(char* opcode, uint16_t src, uint16_t dst) {
  if (debug) {
    const char* name = address_name(dst);
    Serial.printf("call with opcode %s from %04x to %04x\n", opcode, src, dst, name);
  }
}

void CPU6809::on_nmi(uint16_t src, uint16_t dst) {
  if (debug) {
    const char* name = address_name(dst);
    Serial.printf("NMI from %04x to %04x (%s)\n", src, dst, name);
  }
}

void CPU6809::on_irq(uint16_t src, uint16_t dst) {
  if (debug) {
    const char* name = address_name(dst);
    Serial.printf("IRQ from %04x to %04x (%s)\n", src, dst, name);
  }
}
void CPU6809::on_firq(uint16_t src, uint16_t dst) {
  if (debug) {
    const char* name = address_name(dst);
    Serial.printf("FIRQ from %04x to %04x (%s)\n", src, dst, name);
  }
}

////////////////////////////////////////////////////////////////////
// Processor Control Loop
////////////////////////////////////////////////////////////////////

#if outputDEBUG
#define DELAY_FACTOR() delayMicroseconds(1000)
#else
#define DELAY_FACTOR_H() asm volatile("nop\nnop\nnop\nnop\n");
#define DELAY_FACTOR_L() asm volatile("nop\nnop\nnop\nnop\n");
#endif