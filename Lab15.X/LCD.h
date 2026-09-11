#ifndef LCD_H
    #define	LCD_H


    /**************************************************************************/
    /*************** Update the definitions below accordingly *****************/
    /**************************************************************************/
    
    // Pin assignments per Lab 5 Figure 1 schematic:
    //   RS = RD0, EN = RD2, D4 = RA4, D5 = RA5, D6 = RA6, D7 = RA7
    
    #define LCD_RS_TRIS TRISDbits.TRISD0  //TRIS bit of the pin connected to "RS"
    #define LCD_EN_TRIS TRISDbits.TRISD2  //TRIS bit of the pin connected to "EN"
    #define LCD_D4_TRIS TRISAbits.TRISA4  //TRIS bit of the pin connected to "D4"
    #define LCD_D5_TRIS TRISAbits.TRISA5  //TRIS bit of the pin connected to "D5"
    #define LCD_D6_TRIS TRISAbits.TRISA6  //TRIS bit of the pin connected to "D6"
    #define LCD_D7_TRIS TRISAbits.TRISA7  //TRIS bit of the pin connected to "D7"

    #define LCD_RS_LAT LATDbits.LATD0  //LAT bit of the pin connected to "RS"
    #define LCD_EN_LAT LATDbits.LATD2  //LAT bit of the pin connected to "EN"
    #define LCD_D4_LAT LATAbits.LATA4  //LAT bit of the pin connected to "D4"
    #define LCD_D5_LAT LATAbits.LATA5  //LAT bit of the pin connected to "D5"
    #define LCD_D6_LAT LATAbits.LATA6  //LAT bit of the pin connected to "D6"
    #define LCD_D7_LAT LATAbits.LATA7  //LAT bit of the pin connected to "D7"

    #define _XTAL_FREQ 16000000  //Frequency of the system clock (16 MHz HFINTOSC)

    /**************************************************************************/
    /*************************** Necessary includes ***************************/
    /**************************************************************************/

    #include <xc.h>
    #include <stdint.h>
    
    /**************************************************************************/
    /*************************** Function prototypes **************************/
    /**************************************************************************/

    void LCD_init(void);
    void LCD_cursor_set(uint8_t, uint8_t);
    void LCD_cursor_increment(void);
    void LCD_cursor_decrement(void);
    void LCD_configure_lcd_display(uint8_t);
    void LCD_configure_cursor_display(uint8_t);
    void LCD_configure_cursor_blink(uint8_t);
    void LCD_clear(void);
    void LCD_shift_reset(void);
    void LCD_shift_left(void);
    void LCD_shift_right(void);    
    void LCD_configure_entry_id(uint8_t);
    void LCD_configure_entry_shift(uint8_t);
    void LCD_add_character(char*, uint8_t);
    
    void LCD_write_string(char*);
    void LCD_write_char(char);
    void LCD_write_variable(int32_t, uint8_t);
    void LCD_write_float(float, uint8_t, uint8_t);
    
    
#endif	/* LCD_H */