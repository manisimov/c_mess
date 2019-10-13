#include <stdint.h>
#include <stdio.h>
#include "first.h"
#include "second.h"
#include "third.h"

uint8_t about_str[] = "hello";
uint8_t data[] = {0, 0, 0, 0, 0, 0};


static void fill_data(void)
{
    unsigned int i;
    
    for(i=0; i<sizeof(data); i++)
    {
        data[i] = i;
    }
}


void main(void)
{
    printf("%s\n", about_str);
    fill_data();
    
    first_fun();
    second_fun();
    third_fun();
}