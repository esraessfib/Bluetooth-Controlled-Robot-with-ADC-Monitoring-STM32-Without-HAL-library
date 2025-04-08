#include "stm32f4xx.h"
#include <stdio.h>
#include <string.h>

//Fonctions
void config_TIMER6(void);
void config_EXTI(void);
void config_ADC1(void);
void config_USART3(void);
//Variables 
int ncount = 0;
int ncount_adc = 0;
int ADC_VALUE[3];
char Buffer_rec[50];
int i = 0;
int j = 0;
int N = 0;

// EXTI Configuration   
void config_EXTI(void)
{
	RCC->AHB1ENR |= 0x1; //Enable GPIOA clock
	RCC->APB2ENR |= (0x01 << 14); //Enable SYSCFG clock
	SYSCFG->EXTICR[0] = 0x0; //Association of PA0 to EXTI0: set 0000 (PA0) in the 4 bits of EXTI0[3:0]
	EXTI->IMR |= 0x1; //Activation of the EXTI0 line
	EXTI->FTSR |= 0x1; //Configuration for falling edge trigger: set the corresponding bit in FTSR (Falling Trigger Selection Register)
	NVIC_EnableIRQ(EXTI0_IRQn); //Enabling the EXTI0 interrupt 
}

//TIMER 6 Configuration  
void config_TIMER6(void)
{
	RCC->APB1ENR |= 0x10; //Enable TIMER 6 clock
	TIM6->PSC = 15999; //Configuring the prescaler: 16M /15999 + 1 = 1000 Hz
	TIM6->ARR = 1999; //Configuring ARR : Compteur for 2000 ms (2 seconde)
	TIM6->DIER = 0x1; //Enable interrupt
	NVIC_EnableIRQ(TIM6_DAC_IRQn); //Enable the TIMER 6 interrupt in the NVIC controller
}

//ADC Configuration
void config_ADC1(void)
{
	RCC->AHB1ENR |= 0x8; //Enable GPIOD clock
	GPIOD->MODER |= 0x55 << 24; //Configure PD12, PD13, PD14, PD15 in Output mode
	RCC->AHB1ENR |= 0x1; //Enable GPIOA clock
	GPIOA->MODER |= 0x3 << 2; //Configure PA1 in Output mode
	GPIOA->MODER |= 0x3 << 4; //Configure PA2 in Output mode
	GPIOA->MODER |= 0x3 << 6; //Configure PA3 in Output mode
	RCC->APB2ENR |= 0x1 << 8; //Enable ADC clock
	ADC1->CR2 |= (1 << 0); //Activate the ADC by setting bit 0 of the CR2 register
	ADC1->CR1 |= (1 << 11); //Configure in discontinuous mode 
  ADC1->CR1 |= (1 << 5); //Enable the interrupt and wait for the rising of the EOC flag
	ADC1->SQR1 = 0; //Configure the length: a single conversion in the sequence
	NVIC_EnableIRQ(ADC_IRQn); //Enable the ADC interrupt in the NVIC controller
}

//USART3 Configuration 
void config_USART3(void)
{
	RCC->AHB1ENR |= (1 << 1); //Enable GPIOB clock
	GPIOB->MODER |= (2 << 20); //Configure PB10 (TX) in Alternate Function mode
	GPIOB->MODER |= (2 << 22); //Configure PB11 (RX) in Alternate Function mode
	//Configuration of AF7 for USART3 
	GPIOB->AFR[1] |= (7 << 8); //Shift in the register to the left, this configures pin PB10 to use AF7: Tx
	GPIOB->AFR[1] |= (7 << 12); //Shift in the register to the left, this configures pin PB11 to use AF7: Rx
	RCC->APB1ENR |= (1 << 18); //Enable USART3 clock
	USART3->BRR = 0x683;      // baud rate 9600
	USART3->CR1 |= (1 << 2); //Activate RE 
	USART3->CR1 |= (1 << 3); //Activate TE 
	USART3->CR1 |= (1 << 4); //IDLE Configuration
	USART3->CR1 |= (1 << 5); //RXNE Configuration 
	USART3->CR1 |= (1 << 13); //Activate USART 
	NVIC_EnableIRQ(USART3_IRQn); //Enable USART3 interrupt 
}

//Configuration du PWM 
void config_TIM3_PWM(void)
{
	RCC->APB1ENR |= (1 << 1); 
	RCC->AHB1ENR |= (1 << 1); 
	GPIOB->MODER |= (2<<0) | (2<<2) | (2<<8) | (2<<10); //Configuration of PB0, PB1, PB4, and PB5 in Alternate Function 
	GPIOB->AFR[0] |= (2<<0) | (2<<4) | (2<<16) | (2<<20); //Configuration of AF2 (TIM3)
	TIM3->PSC = 15;  //Configuration of the prescaler for a frequency of 1 MHz (16M/(15+1))
	TIM3->ARR = 999; //Configuration of the period for 1 kHz (ARR+1): Counter from 0 to 999, a period of 1000 cycles
	//Configuration of duty cycles (Alpha = 1 + CCRx)
	TIM3->CCR1 = 0;
	TIM3->CCR2 = 0;
	TIM3->CCR3 = 0;
	TIM3->CCR4 = 0;
	TIM3->CCMR1 |= (0x60 << 0) | (0x60 << 8); 
	TIM3->CCMR2 |= (0x60 << 0) | (0x60 << 8); 
	TIM3->CCER |= (1 << 0)|(1 << 4)|(1 << 8)|(1 << 12); //Activate Channels
	TIM3->CR1 |= (1 << 0); //start TIMER
}

//Configuration of priorities
void config_NVIC_Priorities(void)
{
  NVIC_SetPriority(USART3_IRQn, NVIC_EncodePriority(2, 0, 0)); //Définition de la priorité de l'interruption USART3
  NVIC_SetPriority(EXTI0_IRQn, NVIC_EncodePriority(2, 1, 0)); //Définition de la priorité de l'interruption EXTI0
  NVIC_SetPriority(TIM6_DAC_IRQn, NVIC_EncodePriority(2, 2, 0)); //Définition de la priorité de l'interruption TIMER6
  NVIC_SetPriority(ADC_IRQn, NVIC_EncodePriority(2, 3, 0)); //Définition de la priorité de l'interruption ADC
}

//Function to send a single character
void Sendchar_USART3(char c)
{
	while(!(USART3->SR & (1 << 7))) //Wait for TXE  to be 1 (SR), indicating that the data register is ready to receive a new character
    USART3->DR = c; 
}              

//Function to send a string of characters
void SendString_USART3(char *pt)
{
    while (*pt) 
    {
        Sendchar_USART3(*pt); 
        pt++; 
    }
}


int main(void)
{
	config_EXTI();
	config_TIMER6();
	config_ADC1();
	config_USART3();
	config_TIM3_PWM();
	config_NVIC_Priorities();
	while(1);
}


void EXTI0_IRQHandler(void)
{
	if ((EXTI->PR & 0x1) != 0) //Check for the triggering of the interrupt
	{
		TIM6->CR1 |= 0x0001; //Start TIMER6  
		EXTI->PR = 0x1; //Clear the interrupt bit by writing 1
	}
}


void TIM6_DAC_IRQHandler(void)
{
    if((TIM6->SR & 0x0001) != 0) //Check if the Clear Flag (UIF) is set to 1.
    {
        ncount++; 
        if(ncount==1) 
        {
          GPIOD->ODR = 0x4000; //Configure pin D15 in high mode: Sufficient voltage
		  ADC1->SQR3 = 1; //Select channel 1 for ADC conversion
		  ADC1->CR2 |= 1 << 30; //Start the ADC conversion

        }
        if(ncount==2) 
        {
          GPIOD->ODR = 0x2000; //		Configure pin D14 in high mode		
		  ADC1->SQR3 = 2; //Select channel 2 for ADC conversion
		  ADC1->CR2 |= 1 << 30; 

        }
        if(ncount==3) 
        {
		  GPIOD->ODR = 0x1000; //Configure pin D13 in high mode          
		  ADC1->SQR3 = 3; //Select channel 3 for ADC conversion
		  ADC1->CR2 |= 1 << 30; 
		  ncount=0; // Reset the counter to 0
        }
        TIM6->SR &= 0x0000; //Clear Flag (UIF = 0) 
	}
}


void ADC_IRQHandler(void) 
{
    if (ADC1->SR & (1<<1)) //Check if the EOC flag is set (EOC = 1)
	{ 
        ncount_adc++; 
        if (ncount_adc == 1) //first conversion
        {
            ADC_VALUE[0] = ADC1->DR; //Read the converted value and store it in the array.
        }
        if (ncount_adc == 2) 
        {
            ADC_VALUE[1] = ADC1->DR; 
        }
        if (ncount_adc == 3) 
        {
            ADC_VALUE[2] = ADC1->DR; 
            ncount_adc = 0; 
            //Prepare a message with the values read from the channels.
		    char msg[50]; 
			sprintf(msg, "CHx: %d, CHy: %d, CHz: %d\r\n", ADC_VALUE[0], ADC_VALUE[1], ADC_VALUE[2]);
			SendString_USART3(msg); 
        }
    }
}


void USART3_IRQHandler(void)
{
    if (USART3->SR & (1 << 5)) //Check if the RXNE flag is set
    {
      i++;  
	  Buffer_rec[i-1] = USART3->DR; //Read the received data and store it in the buffe
    }
    if (USART3->SR & (1 << 4)) //Check if the TXE flag is set
    {
		USART3->DR; //Read the register to clear the TXE flag
        
		if (strstr(Buffer_rec,"AVANCE")) 
		{
			if((TIM3->CCR1 < 500) && (TIM3->CCR3 < 500)) 
			{
				TIM3->CCR1 += 250; 
				TIM3->CCR2 = 0; 
				TIM3->CCR3 += 250; 
				TIM3->CCR4 = 0; 
			}
		}
		else if (strstr(Buffer_rec,"ARRIERE")) 
		{
			if((TIM3->CCR2 < 500) && (TIM3->CCR4 < 500)) 
			{
				TIM3->CCR1 = 0; //Stop motor 1
				TIM3->CCR2 += 250; //Increase the speed of motor 2
				TIM3->CCR3 = 0; //Stop motor 3
				TIM3->CCR4 += 250; //Increase the speed of motor 4
			}
		}
		else if (strstr(Buffer_rec,"DROITE")) 
		{
			TIM3->CCR1 = 250; 
			TIM3->CCR2 = 0;
			TIM3->CCR3 = 500; 
			TIM3->CCR4 = 0;				
		}
		else if (strstr(Buffer_rec,"GAUCHE")) 
		{
			TIM3->CCR1 = 500; 
			TIM3->CCR2 = 0;
			TIM3->CCR3 = 250; 
		  TIM3->CCR4 = 0;				
		}
        else if (strstr(Buffer_rec,"STOP")) 
        { 
			// Stop the motors (set the duty cycles to 0)
			TIM3->CCR1 = 0;
			TIM3->CCR2 = 0;
			TIM3->CCR3 = 0;
			TIM3->CCR4 = 0;		
        }
	    N = i; // Recording the size of the received string
		i = 0; 
		SendString_USART3(Buffer_rec); 
        //Reset the buffer to zero
		for(j=0;j<N;j++)
        {
			Buffer_rec[j]=0; 
		}
	}
}