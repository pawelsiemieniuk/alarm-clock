#include <msp430x14x.h>
#include "lcd.h"
#include "portyLcd.h"

#define LFXT1_HFREQ         8000000     // Czestotliwosc LFXT1
                                        // w trybie wysokiej czestotliwosci
#define ACLK_DIVIDER        3           // Przesuniecie bitowe w ACLK (Dzielnik)
#define INPUT_DIVIDER       2           // Przesuniecie bitowe w ID (Dzielnik)
#define MODE_CONTROL        1           // Tryb zegara (UP)
#define INTERRUPT_TIME_us   1000        // 1ms
#define sec_us              1000000     // 1s = 1000000us
#define SEC_IN_DAY          86400       // Liczba sekund w ciagu dnia
#define CURR_T              3600000     // Zdefiniowany aktualny czas
#define CLICK_TIME          200         // Liczba przerwan po ktorej zarejestrowany jest przycisk

void init();
void io_config();
void timerConfig();
void clearOFIFG();
void displaySeconds();
void displayTime();
void displaySavedTime();
void setAlarm();

unsigned long B1_TIME = 0, B1_STATUS = 0;                   // Licznik przerwan po wcisnieciu
unsigned long B2_TIME = 0, B2_STATUS = 0;                   // przyciskow B1, B2, B3, B4
unsigned long B3_TIME = 0, B3_STATUS = 0;                   // oraz ich status
unsigned long B4_TIME = 0, B4_STATUS = 0;                   //

unsigned long INTERRUPT_IN_SEC = sec_us / INTERRUPT_TIME_us;// Liczba przerwan w ciagu sekundy
unsigned long COUNTER = CURR_T;                             // Zmienna zliczajaca przerwania

unsigned int SET_ALARM = 0;                                 // Czy alarm jest ustawiany
unsigned int BUZZER_TIME = 0;                               // Czas do konca dzialania alarmu

unsigned int hours = 0;
unsigned int minutes = 0;
unsigned int seconds = 0;
unsigned int miliseconds = 0;

unsigned int SAVED_TIME[3][4] = {{0,0,0,0},                 // Tablica zapisanych alarmow
                                 {0,0,0,0},
                                 {0,0,0,0}};
unsigned int current_saved_id = 0;                          // Id kolejnego alarmu do zapisu

void main()
{
    init();
    io_config();
    timerConfig(ACLK_DIVIDER, INPUT_DIVIDER, MODE_CONTROL, INTERRUPT_TIME_us);
    displayTime();
}

void displayTime()
{
    while(1)
    {
        _BIS_SR(LPM3_bits);                 // Przejscie do trybu LPM3
        SEND_CMD(DD_RAM_ADDR);              // Ustawienie kursora w pierwszym wierszu

        if(SET_ALARM == 0)
        {
            // Wyswietlanie kolejnych cyfr czasu
            SEND_CHAR((hours / 10) + 48);
            SEND_CHAR((hours % 10) + 48);
            SEND_CHAR(':');
            SEND_CHAR((minutes / 10) + 48);
            SEND_CHAR((minutes % 10) + 48);
            SEND_CHAR(':');
            SEND_CHAR((seconds / 10) + 48);
            SEND_CHAR((seconds % 10) + 48);
            SEND_CHAR(':');
            SEND_CHAR((miliseconds / 100) + 48);
            SEND_CHAR(((miliseconds / 10) % 10) + 48);
        }
        else if(SET_ALARM == 1)
        {
            setAlarm();
            displaySavedTime();
        }
        for(int i=0;i<11;i++)
            SEND_CMD(CUR_SHIFT_LEFT);       // Przesuniecie kursora o 11 miejsc w lewo
    }
}

// Ustawianie alarmu
void setAlarm()
{
    if(B1_STATUS == 1)
    {
        SAVED_TIME[current_saved_id][0]++;
        SAVED_TIME[current_saved_id][0] %= 24;
        B1_STATUS = 0;
    }
    if(B2_STATUS == 1)
    {
        SAVED_TIME[current_saved_id][1]++;
        SAVED_TIME[current_saved_id][1] %= 60;
        B2_STATUS = 0;
    }
    if(B3_STATUS == 1)
    {
        SAVED_TIME[current_saved_id][2]++;
        SAVED_TIME[current_saved_id][2] %= 60;
        B3_STATUS = 0;
    }
}

// Wyswietlanie aktualnie edytowanego alarmu
void displaySavedTime()
{
    SEND_CMD(DD_RAM_ADDR);
    if((SAVED_TIME[current_saved_id][0] == 0)
    && (SAVED_TIME[current_saved_id][1] == 0)
    && (SAVED_TIME[current_saved_id][2] == 0)
    && (SAVED_TIME[current_saved_id][3] == 0))
    {
        SEND_CHAR('-');
        SEND_CHAR('-');
        SEND_CHAR(':');
        SEND_CHAR('-');
        SEND_CHAR('-');
        SEND_CHAR(':');
        SEND_CHAR('-');
        SEND_CHAR('-');
        SEND_CHAR(':');
        SEND_CHAR('-');
        SEND_CHAR('-');
    }
    else
    {
        SEND_CHAR((SAVED_TIME[current_saved_id][0] / 10) + 48);
        SEND_CHAR((SAVED_TIME[current_saved_id][0] % 10) + 48);
        SEND_CHAR(':');
        SEND_CHAR((SAVED_TIME[current_saved_id][1] / 10) + 48);
        SEND_CHAR((SAVED_TIME[current_saved_id][1] % 10) + 48);
        SEND_CHAR(':');
        SEND_CHAR((SAVED_TIME[current_saved_id][2] / 10) + 48);
        SEND_CHAR((SAVED_TIME[current_saved_id][2] % 10) + 48);
        SEND_CHAR(':');
        SEND_CHAR((SAVED_TIME[current_saved_id][3] / 100) + 48);
        SEND_CHAR(((SAVED_TIME[current_saved_id][3] / 10) % 10) + 48);
    }
}

void init()
{
    WDTCTL = WDTPW + WDTHOLD;           // Zatrzymanie WDT
    InitPortsLcd();                     // inicjalizacja portow
    InitLCD();                          // inicjalizacja LCD
    clearDisplay();                     // czyszczenie LCD
}

void io_config()
{
    P4DIR &= 0x0F;                      // Konfiguracja B1, B2, B3, B4 jako wejscia
    P4DIR |= 0x0C;                      // Konfiguracja BUZZERA jako wyjscia
}

void timerConfig(unsigned int divA, unsigned int ID, unsigned int mode, unsigned int interrTus)
{
    BCSCTL1 |= XTS;                     // Wlaczenie i ustawianie wysokiej czestotliwosci w LFXT1
    clearOFIFG();

    unsigned int DIVA_X, ID_X, MC_X;
    unsigned long long FREQ = LFXT1_HFREQ >> (divA + ID);// Przesuniecie o divA+ID bitow w prawo
    
    switch(divA)                        // Wybieranie dzielnika ACLK
    {
        case 1:
            DIVA_X = DIVA_1;
        break;
        case 2:
            DIVA_X = DIVA_2;
        break;
        case 3:
            DIVA_X = DIVA_3;
        break;
        default:
            DIVA_X = DIVA_0;
    }
    switch(ID)                          // Wybieranie dzielnika ID
    {
        case 1:
            ID_X = ID_1;
        break;
        case 2:
            ID_X = ID_2;
        break;
        default:
            ID_X = ID_0;
    }
    switch(mode)                        // Wybieranie trybu pracy zegara
    {
        case 1:
            MC_X = MC_1;
        break;
        case 2:
            MC_X = MC_2;
        break;
        case 3:
            MC_X = MC_3;
        break;
        default:
            MC_X = MC_0;
    }

    BCSCTL1 |= DIVA_X;                  // Ustawianie dzielnika dla czestotliwosci w ACLK
    BCSCTL2 |= SELM0 | SELM1;           // Ustawianie trybu MCLK na LFXT1

    TACTL = TASSEL_1 + MC_X + ID_X;     // Ustawianie zrodla na ACLK, trybu oraz dzielnika
    CCTL0 = CCIE;                       // Wlaczenie przerwan od CCR0
    CCR0 = FREQ / interrTus;            // Przerwanie generowane co interrTus mikrosekund

    _EINT();                            // Wlaczenie przerwan
}

void clearOFIFG()
{
    do{
        IFG1 &= ~OFIFG;                 // Czyszczenie flagi OSCFault
        for(int i=0xff; i; i--);        // Odczekanie minimum 50us
    } while ((IFG1 & OFIFG) == OFIFG);  // Powtarzanie jesli flaga nie zostala wyczyszczona
}

// Obsluga przerwan
#pragma vector=TIMERA0_VECTOR
__interrupt void Timer_A (void)
{
    // Obsluga 4 przycisku
    if((P4IN & BIT7) == 0)
    {
        ++B4_TIME;
        if(B4_TIME > CLICK_TIME)
        {
            SET_ALARM ^= 1;
            (SET_ALARM == 0) ? current_saved_id++ : 1;
            current_saved_id %= 3;
            B4_TIME = 0;
        }
    }
    else
    { B4_TIME = 0; }

    // Obsluga 3 przycisku
    if((P4IN & BIT6) == 0)
    {
        ++B3_TIME;
        if(B3_TIME > CLICK_TIME)
        {
            B3_STATUS = 1;
            B3_TIME = 0;
        }
    }
    else
    { B3_TIME = 0; }

    //Obsluga 2 przycisku
    if((P4IN & BIT5) == 0)
    {
        ++B2_TIME;
        if(B2_TIME > CLICK_TIME)
        {
            B2_STATUS = 1;
            B2_TIME = 0;
        }
    }
    else
    { B2_TIME = 0; }

    // Obsluga 1 przycisku
    if((P4IN & BIT4) == 0)
    {
        ++B1_TIME;
        if(B1_TIME > CLICK_TIME)
        {
            B1_STATUS = 1;
            B1_TIME = 0;
        }
    }
    else
    { B1_TIME = 0; }

    // Zliczanie przerwan i resetowanie licznika po dotarciu do 24h
    ++COUNTER;
    if(COUNTER >= SEC_IN_DAY * INTERRUPT_IN_SEC)
        COUNTER = 0;

    // Zamiana COUNTER na zmienne czasu
    miliseconds = COUNTER % 1000;
    if((COUNTER % 1000) == 0)
    {
        seconds++;
        seconds %= 60;
    }
    if((COUNTER % 60000) == 0)
    {
        minutes++;
        minutes %= 60;
    }
    if((COUNTER % 3600000) == 0)
    {
        hours++;
        hours %= 24;
    }

    // Sprawdzanie alarmow
    for(int i=0;i<3;i++)
    {
        if((SAVED_TIME[i][0] == hours)
        && (SAVED_TIME[i][1] == minutes)
        && (SAVED_TIME[i][2] == seconds))
        {
            BUZZER_TIME = 5000;             // Ustawianie liczby przerwan do konca alarmu
            for(int j=0;j<3;j++)
            SAVED_TIME[i][j] = 0;           // Resetowanie zapisanego alarmu
        }
    }

    // Obsluga BUZZERA
    if(BUZZER_TIME > 0)
    {
        if((BUZZER_TIME/500)%2 == 0)        // Wlaczanie BUZZERA co 500ms
        {
            if((COUNTER/5)%2 == 1)          // Przelaczanie BUZZERA co 5ms (f=200Hz)
            {
                P4OUT ^= BIT2;
            }
        }
        else
        {
            P4OUT = 0x00;
        }
        BUZZER_TIME--;                      // Zmiejszanie czasu dzialania alarmu
    }

    _BIC_SR_IRQ(LPM3_bits);                 // Wyjscie z trybu LPM3
}