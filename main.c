/*
 * An example DMX transmitter
 *
 * This example will increase the value of every channel by 5/255 about
 * every 0.5 seconds.
 */
#include <p24fxxxx.h>
#include <PIC24F_plib.h>

_CONFIG1(JTAGEN_OFF & GCP_OFF & GWRP_OFF & ICS_PGx1 & FWDTEN_OFF)
_CONFIG2(POSCMOD_NONE & FNOSC_FRCPLL & FCKSM_CSDCMD & OSCIOFNC_ON & IESO_ON &
    PLLDIV_NODIV)

#define TXCTL PORTBbits.RB7
#define TXCTLTRIS TRISBbits.TRISB7

    
typedef enum
{
    WAIT,
    BREAK,
    MBB,
    MAB,
    DATA,
    DONE
     
} TxState;

int dimData[255];
char wait;
int val = 0;

int main(void)
{
    long i;
    init();

    for(;;)
    {
        val = val+5;
        if (val > 255)
            val = 0;
        for (i = 0; i < 255; i++)
            dimData[i] = val;
        SendDMX();
        for(i=0;i<1000000L;i++);
    }
}

// device initialization routine
int init()
{
    // make all pins digital
    AD1PCFG = 0xFFFF;

    // assign pins to uart for DMX (8 bit, 2 stop bits, 250 kbaud)
    PPSUnLock;
    PPSInput(PPS_U1RX, PPS_RP8);
    PPSOutput(PPS_RP9, PPS_U1TX);
    PPSLock;

    // enable the uart
	OpenUART1(0x8009, 0x8400, 15);
  	ConfigIntUART1( UART_RX_INT_DIS &
                  	UART_RX_INT_PR2 &
                  	UART_TX_INT_DIS &
                  	UART_TX_INT_PR1);

    // make tx pin output
    TRISBbits.TRISB9 = 0;

    // enable timer
    T1CON = 0x00;        // reset timer
    TMR1 = 0x00;         // clear register
    PR1 = 0x640;          // set period to 100 us
    IPC0bits.T1IP = 5;	 //set interrupt priority
    IFS0bits.T1IF = 0;	 //reset interrupt flag
    IEC0bits.T1IE = 1;	 //turn on the timer1 interrupt
    T1CONbits.TON = 1;  // enable the timer

    return 0;
    
}

int SendDMX()
{
    TxState state = MBB;
    int slot;
    T1CONbits.TON = 1;  // enable the timer
    while (state != DONE)
    {
        wait = TRUE;
        switch (state)
        {
            case MBB:
                TXCTLTRIS = 0;
                TXCTL = 1;
                state = BREAK;
                break;
            case BREAK:
                TXCTLTRIS = 0;
                TXCTL = 0;
                state = MAB;
                break;
            case MAB:
                TXCTLTRIS = 0;
                TXCTL = 1;
                state = DATA;
                break;
            case DATA:
                TXCTLTRIS = 1;
                WriteUART1(0x0);    // start code: 0x0 for data
                for(slot = 0; slot < 255; slot++)
                {
                    WriteUART1(dimData[slot]);
                    while(BusyUART1());
                }
                state = DONE;
                break;
        }
        while(wait);
    }
    T1CONbits.TON = 0;  // disable the timer
    return 0;
}

void __attribute__((interrupt,no_auto_psv)) _T1Interrupt(void)
{
    wait = FALSE;
    IFS0bits.T1IF = 0;
    return;
}