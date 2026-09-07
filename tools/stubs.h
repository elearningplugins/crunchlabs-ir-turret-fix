#pragma once
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>

// ---- Arduino core stubs -------------------------------------------------
#define F(x) (x)
#define VERSION_IRREMOTE "sim"
#define ENABLE_LED_FEEDBACK 1
#define UNKNOWN 0
#define IRDATA_FLAGS_IS_REPEAT 0x01
#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

extern unsigned long g_virtual_ms;
inline void delay(unsigned long ms){ g_virtual_ms += ms; }

inline unsigned long millis(){ return g_virtual_ms; }

struct SerialStub {
    std::string inbuf;
    int available(){ return (int)inbuf.size(); }
    char read(){ if(inbuf.empty()) return 0; char c=inbuf[0]; inbuf.erase(0,1); return c; }
    void begin(long){}
    void print(const char*){}
    void println(const char*){}
    void println(){}
    void print(int){}
    void println(int){}
    void print(unsigned long){}
    void println(unsigned long){}
};
extern SerialStub Serial;
inline void printActiveIRProtocols(SerialStub*){}

// ---- Servo stub: records every write ------------------------------------
struct Servo {
    int pin = -1;
    bool attached = false;
    int last = -999;
    std::vector<int> writes;
    void attach(int p){ pin = p; attached = true; }
    void detach(){ attached = false; }
    void write(int v){
        if(!attached){ printf("  !! write(%d) to DETACHED servo pin %d\n", v, pin); }
        last = v; writes.push_back(v);
    }
    void writeMicroseconds(int){}
};

// ---- IR receiver stub ---------------------------------------------------
struct IRData { int command = 0; int flags = 0; int protocol = 1; };
struct IrReceiverStub {
    IRData decodedIRData;
    int pending = 0;              // frames waiting in the buffer
    void begin(int, int){}
    bool decode(){ if(pending>0){ pending--; return true; } return false; }
    void resume(){}
    void printIRResultShort(SerialStub*){}
    void printIRSendUsage(SerialStub*){}
    void printIRResultRawFormatted(SerialStub*, bool){}
};
extern IrReceiverStub IrReceiver;
