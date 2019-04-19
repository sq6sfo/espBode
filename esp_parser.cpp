#include "esp_parser.h"
#include "esp_fy6800.h"

volatile char *gReadBuffer = NULL;

/* Function parses number from the msg string expects natural a number. */
static uint32_t parseNumber(char *msg)
{
    uint32_t number = 0;
    while(0<=(msg[0]-'0') && (msg[0]-'0')<=9)
    {
        number*=10;
        number+=msg[0]-'0';
        msg++;
    }
    return number;
}

/* Similar to parseNumber, but handles also decimal '.' and '-' sign.0
Return value is multiplied *1000 to include the decimal part:
    123.345 -> 123456
    -1.2    ->  -1200 */
static int32_t parseDecimal(char *msg)
{
    uint8_t dot = 0;
    int32_t multiplier = 1000;
    int32_t number = 0;

    if(msg[0] == '-')
    {
        multiplier*=-1;
        msg++;
    }

    while((0<=(msg[0]-'0') && (msg[0]-'0')<=9) || msg[0] == '.')
    {
        if(msg[0] == '.')
        {
            dot = 1;
            msg++;
            continue;
        }
        if(dot) multiplier/=10;
        number*=10;
        number+=msg[0]-'0';
        msg++;
    }
    return number*multiplier;
}

void handleWriteMsg(char *msg, uint8_t len)
{
    uint8_t selectedChannel = 1;
    while(len>0)
    {
        /* ID request message, we preapare answer in gReadBuffer */
        if(0 == strncmp(msg, "IDN-SGLT-PRI?", 13))
        {
            if(gReadBuffer != NULL) free((void*)gReadBuffer); /* Prevent memory leaks */
            gReadBuffer = (char*)malloc(strlen(ID)+1);
            strcpy((char*)gReadBuffer, ID);
            break;
        }

        if(0 == strncmp(msg, "C1:", 3))
        {
            selectedChannel = 1;
            msg+=3;
            len-=3;
        }

        if(0 == strncmp(msg, "C2:", 3))
        {
            selectedChannel = 2;
            msg+=3;
            len-=3;
        }

        if(0 == strncmp(msg, "BSWV WVTP,", 10))
        {
            msg+=10;
            len-=10;

            if(0 == strncmp(msg, "SINE,",5))
            {
                msg+=5;
                len-=5;
                selectedChannel==1?setCh1Wave(EWaveType_Sine):setCh2Wave(EWaveType_Sine);
            }
        }

        if(0 == strncmp(msg, "PHSE,", 5))
        {
            msg+=5;
            len-=5;
            selectedChannel==1?setCh1Phase(parseNumber(msg)):setCh2Phase(parseNumber(msg));
        }

        if(0 == strncmp(msg, "FRQ,", 4))
        {
            msg+=4;
            len-=4;
            selectedChannel==1?setCh1Freq(parseNumber(msg)):setCh2Freq(parseNumber(msg));
        }

        if(0 == strncmp(msg, "AMP,", 4))
        {
            msg+=4;
            len-=4;
            selectedChannel==1?setCh1Ampl(parseDecimal(msg)):setCh2Ampl(parseDecimal(msg));
        }

        if(0 == strncmp(msg, "OFST,", 5))
        {
            msg+=5;
            len-=5;
            selectedChannel==1?setCh1Offset(parseDecimal(msg)):setCh2Offset(parseDecimal(msg));
        }

        if(0 == strncmp(msg, "OUTP ON", 7))
        {
            msg+=7;
            len-=7;
            selectedChannel==1?setCh1Output(1):setCh2Output(1);
        }

        if(0 == strncmp(msg, "OUTP OFF", 8))
        {
            msg+=8;
            len-=8;
            selectedChannel==1?setCh1Output(0):setCh2Output(0);
        }

        /* Unknown string, skip one character */
        msg++;
        len--;
    }
}
