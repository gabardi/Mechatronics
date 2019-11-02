/***********************************************************
Project : Lab 4
File    : logic8bit.h
Date    : 9/25/2019
Author  : Kaitlyn Gabardi & Ryan Quinn
Class   : ME 445

Chip type               : ATmega2560
Program type            : Application
AVR Core Clock frequency: 14.745600 MHz
Memory model            : Small
External RAM size       : 0
Data Stack size         : 2048
Comments: 
This file contains functions to execute 8-bit logic 
operations.
***********************************************************/


/***********************************************************
Description: 
	Function takes two 8-bit variables as inputs and 
	computes bitwise OR of the two input variables.
    
Parameters:
    NAME          - DESCRIPTION
    variable1     - first 8-bit variable
    variable2     - second 8-bit variable

Returns:
    result_or     - bitwise OR of variable1 and variable2
***********************************************************/
char or_8bit(char variable1, char variable2) 
{
	char result_or = variable1 | variable2;
	return result_or;	
}


/***********************************************************
Description: 
	Function takes two 8-bit variables as inputs and 
	computes bitwise AND of the two input variables.
    
Parameters:
    NAME          - DESCRIPTION
    variable1     - first 8-bit variable
    variable2     - second 8-bit variable

Returns:
    result_and     - bitwise AND of variable1 and variable2
***********************************************************/
char and_8bit(char variable1, char variable2) 
{
	char result_and = variable1 & variable2;
	return result_and;	
}


/***********************************************************
Description: 
	Function takes two 8-bit variables as inputs and 
	computes bitwise XOR of the two input variables.
    
Parameters:
    NAME          - DESCRIPTION
    variable1     - first 8-bit variable
    variable2     - second 8-bit variable

Returns:
    result_xor     - bitwise XOR of variable1 and variable2
***********************************************************/
char xor_8bit(char variable1, char variable2) 
{
	char result_xor = variable1 ^ variable2;
	return result_xor;	
}