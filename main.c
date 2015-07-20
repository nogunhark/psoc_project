/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include <project.h>
#include <stdlib.h>
#include <string.h>


#define UARTBUFSIZE 0xFF

#define SLAVEADD    0x30
#define BUFSIZE     0xFF

#define MASK    0x0F

#define STATUSTEST  0

#define PRINTTIME   500

uint8 Mr_buf[BUFSIZE], Mw_buf[BUFSIZE];
uint8 Sr_buf[BUFSIZE], Sw_buf[BUFSIZE];
uint8 m_read_length=0;
uint8 i2c_slave_adress=0x00;

uint32 t_us_10=0;
uint32 t_ms=0, t_ms_i=0;


uint8 uart_rx_buf[UARTBUFSIZE];
uint8 intemp_print[] = "02 4e";
uint8 ch, inbuf=0;
uint8 uart_print=0;
uint8 uart_num=0;
uint32 command_error=0;

uint32 i=0;

uint32 iconv=0;
uint32 convsp=0;

uint8 status;
uint8 status_flag=0;
uint8 status_data=0;

uint8 I2C_Read_Command(uint8 add, uint8* buf);
void I2C_Write_Command(uint8 add, uint8* buf);
void I2C_Master_Read_Buf_Print(uint8* buf, uint8 bufsize);


void Buf_Clear(uint8* buf, uint8 bufsize);
uint8 Ascii_2_Hex(uint8* arr);
void Hex_2_Ascii(uint8 hex);


void Internal_Temp_Conv(uint8* buf);

CY_ISR(timer_interrupt)
{
    TIMER1_STATUS;
    
    t_us_10++;
    t_ms_i++;
    
    if(t_ms_i == 100)
    {
        t_ms_i = 0;
        t_ms++;
    }
}

CY_ISR(status_interrupt)
{
    status_flag = 1;
}


int main()
{
    /* Place your initialization/startup code here (e.g. MyInst_Start()) */    
    
    I2CS_SlaveInitReadBuf((uint8*)Sr_buf,BUFSIZE);
    I2CS_SlaveInitWriteBuf((uint8*)Sw_buf,(BUFSIZE));
    
    I2CM_Start();
    I2CS_Start();
    UART_Start();
    TIMER1_Start();
    
    STATUS_InterruptEnable();
    STATUS_WriteMask(0x02);
    
    ISR1_StartEx(timer_interrupt);
    ISR2_StartEx(status_interrupt);

    CyGlobalIntEnable; /* Enable global interrupts. */
    
    UART_PutString("start \n\r");
    UART_PutString("New user need to write 'i' or 'I' \n\r");
    
    for(;;)
    {
        /* Place your application code here. */
        
        ch = UART_GetChar();
        status = STATUS_Read();

        if(ch>0)
        {    
            if(ch == '\n')
            {
                convsp=0;
                uart_num=0; 
            }
            else if(ch == '\r')
            {
                if(uart_rx_buf[0] == 'r' || uart_rx_buf[0] == 'R')
                {
                    
                    if(uart_num != 7)
                    {
                        command_error = 1;
                    }                    
                    else
                    {
                        UART_PutString("i2c read command!! \n\r");
                        m_read_length = I2C_Read_Command(i2c_slave_adress, &uart_rx_buf[2]);
                        I2C_Master_Read_Buf_Print(Mr_buf, m_read_length); 
                        
                    }
                }
                else if(uart_rx_buf[0] == 'w' || uart_rx_buf[0] == 'W')
                {
                    
                        
                        
                        #if STATUSTEST
                            if(uart_num < 4)
                            {
                                command_error = 1;
                            }
                            else
                            {
                                status_data = Ascii_2_Hex(&uart_rx_buf[2]);
                                CONTROL_Write(status_data);
                            }
                        #else
                            if(uart_num < 10)
                            {
                                command_error = 1;
                            }
                            else
                            {
                                UART_PutString("i2c write command!! \n\r");
                                I2C_Write_Command(i2c_slave_adress, &uart_rx_buf[2]);
                            }
                        #endif
                        
                   
                }
                else if(uart_rx_buf[0] == 's' || uart_rx_buf[0] == 'S')
                {            
                    
                    
                    if(uart_num != 4)
                    {
                        command_error = 1;
                    }
                    else
                    {
                        UART_PutString("set i2c slave adress!! \n\r");
                        i2c_slave_adress = Ascii_2_Hex(&uart_rx_buf[2]);
                    }
                }
                else if(uart_rx_buf[0] == 'm' || uart_rx_buf[0] == 'M')
                {
                    if(uart_num != 1)
                    {
                        command_error = 1;
                    }
                    else
                    {
                        UART_PutString("recent status show!! \n\r");
                        UART_PutString("slave adress : ");
                        Hex_2_Ascii(i2c_slave_adress);
                        UART_PutString("\n\r");
                    }
                }
                else if(uart_rx_buf[0] == 'i' || uart_rx_buf[0] == 'I')
                {
                    if(uart_num != 1)
                    {
                        command_error = 1;
                    }
                    else
                    {
                        UART_PutString("User Guide!! \n\r");
                        UART_PutString("Command \n\r");
                        UART_PutString("(s) : set i2c slave adress [s xx] <slave adress> \n\r");
                        UART_PutString("(m) : show recent status \n\r");
                        UART_PutString("(r) : i2c read command [r xx xx] <data length, memory address> \n\r");
                        UART_PutString("(w) : i2c write command [w xx xx xx xx ...] <data length, memory address, data1, data2, ...> \n\r");
                        UART_PutString("(t) : read internal temperature [t s/p] <s : start> <p : stop> \n\r");   
                    }
                }
                else if(uart_rx_buf[0] == 't' || uart_rx_buf[0] == 'T')
                {

                    if(uart_num != 3)
                    {
                        command_error = 1;
                    }
                    else
                    {
                        Internal_Temp_Conv(&uart_rx_buf[2]);             
                    }          
                }
                else
                {
                    UART_PutString("command error!! \n\r");
                }
                
                if(command_error == 1)
                {
                    command_error = 0;             
                    UART_PutString("command length not match!! \n\r");
                }
                
                Buf_Clear(uart_rx_buf, UARTBUFSIZE);
            }
            else
            {      
                convsp = 1;
                uart_rx_buf[uart_num] = ch;
                uart_num++;
            }
        }
        
        #if STATUSTEST          
        if(t_ms == PRINTTIME)
        {
            t_ms = 0;
            
            if(status_flag == 1 && convsp == 0)
            {
                status_flag = 0;
                UART_PutString("status!! \n\r");
            }
        }
  
        #else
        if(iconv == 1 && convsp == 0)
        {
            if(t_ms == PRINTTIME)
            {
                t_ms = 0;
                m_read_length = I2C_Read_Command(i2c_slave_adress, &intemp_print[0]);
                UART_PutString("internal temp : ");
                I2C_Master_Read_Buf_Print(Mr_buf, m_read_length); 
            }
        }
        else
        {
            t_ms = 0;
        }
        #endif
    }
}

uint8 I2C_Read_Command(uint8 add, uint8* buf)
{
    uint8 cnt=0;
    
    I2CM_MasterClearReadBuf();
    I2CM_MasterClearWriteBuf();
    I2CM_MasterClearStatus();
    
    Buf_Clear(Mw_buf,BUFSIZE);
    Buf_Clear(Mr_buf,BUFSIZE);
 
    cnt = Ascii_2_Hex(&buf[0]);
    
    Mw_buf[0] = Ascii_2_Hex(&buf[3]);   
   
    I2CM_MasterWriteBuf(add>>1,(uint8*)Mw_buf,1,I2CM_MODE_COMPLETE_XFER);  
    /* Wait for the data transfer to complete */
	while(I2CM_MasterStatus() & I2CM_MSTAT_XFER_INP);

    I2CM_MasterReadBuf(add>>1, (uint8*)Mr_buf, cnt, I2CM_MODE_COMPLETE_XFER);
    /* Wait for the data transfer to complete */
	while(I2CM_MasterStatus() & I2CM_MSTAT_XFER_INP); 

    return cnt;
}

void I2C_Write_Command(uint8 add, uint8* buf)
{
    uint8 cnt=0;
    uint32 i;
    
    I2CM_MasterClearWriteBuf();
    I2CM_MasterClearStatus();
    
    Buf_Clear(Mw_buf,BUFSIZE);
    
    cnt = Ascii_2_Hex(&buf[0]);
    
    Mw_buf[0] = Ascii_2_Hex(&buf[3]);
    
    for(i=0;i<cnt;i++)
    {
        Mw_buf[i+1] = Ascii_2_Hex(&buf[6+3*i]);
    }
    
    I2CM_MasterWriteBuf(add>>1, (uint8*)Mw_buf, cnt+1, I2CM_MODE_COMPLETE_XFER);  
    /* Wait for the data transfer to complete */
	while(I2CM_MasterStatus() & I2CM_MSTAT_XFER_INP);

}

void Buf_Clear(uint8* buf, uint8 bufsize)
{
    uint32 i;
    
    for(i=0;i<bufsize;i++)
    {
        buf[i]=0;
    }
}

void I2C_Master_Read_Buf_Print(uint8* buf, uint8 bufsize)
{
    uint32 i, cnt=0;
    
    for(i=0;i<bufsize;i++)
    {
        
        if(cnt == 16)
        {
            cnt = 0;
            UART_PutString("\n\r"); 
        }
        
        Hex_2_Ascii(buf[i]);
        UART_PutString(" "); 
        cnt++;
    }
    UART_PutString("\n\r"); 
}

uint8 Ascii_2_Hex(uint8* arr)
{
    
    uint8 num=0;
    uint32 i=0;
    uint32 shift=4;
    
    for(i=0;i<2;i++)
    {
        if(arr[i]>='0' && arr[i]<='9')
        {
            num |= (arr[i]&MASK)<<shift;
        }
        else if((arr[i]>='A'||arr[i]>='a') && (arr[i]<='F'||arr[i]<='f'))
        {
            num |= ((arr[i]&MASK)+0x09)<<shift;
        }
        else
        {
            UART_PutString("please write correct value \n\r");
            return 0;
        }
        shift = 0;
    }
    return num;
}

void Hex_2_Ascii(uint8 hex)
{
    uint8 U_temp=0;
    uint8 Ascii[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
    
    U_temp = (hex>>4)&MASK;
    UART_PutChar(Ascii[U_temp]);
    U_temp = (hex)&MASK;
    UART_PutChar(Ascii[U_temp]);
}

void Internal_Temp_Conv(uint8* buf)
{   
    if(buf[0] == 's' || buf[0] == 'S')
    {
        UART_PutString("read internal temperature \n\r");
        iconv = 1;
    }
    else if(buf[0] == 'p' || buf[0] == 'P')
    {
        UART_PutString("stop read \n\r");
        iconv = 0;
    }
    else
    {
        UART_PutString("command error \n\r");
    }
}

/* [] END OF FILE */
# Git ds4830a_host

