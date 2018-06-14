#include "mico.h"

int num_str_to_int(char*str,int len)
{
    char *p = str;
    int i = 1;
    int result = 0;
    int temp = 1;
    p += (len-1);
    while(p >= str)
    {
        temp = (int)(*p-'0');
        if((temp > 9)||(temp < 0))
            break;
        result += temp*i;
        i = i*10;
        p--;
    }
    return result;

}
