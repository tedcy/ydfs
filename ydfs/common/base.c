#include "base.h"

char g_base_string[62] = {'0','1','2','3','4','5','6','7','8','9',\
		'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',\
		'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z'};


char* int_to_string(int i,char *s,int len)
{
	char *pos;
	for(pos = s + len - 1;pos != s - 1;--pos)
	{
		*pos = g_base_string[i % 62];
		i /= 62;
	}
	return s;
}

int string_to_int(char *s,int len)
{
	int i = 0;
	char *pos;
	for(pos = s;pos != s + len - 1;++pos)
	{
		if(*pos < 'A')
		{
			i += *pos - '0';
		}
		else 
		{
			if(*pos < 'a')
				i += ((*pos - 'A') + 10);
			else
			{
				i += ((*pos - 'a') + 36);	
			}
		}
		i *= 62;
	}
	if(*pos < 'A')
	{
		i += *pos - '0';
	}
	else 
	{
		if(*pos < 'a')
		{
			i += ((*pos - 'A') + 10);
		}
		else
		{
			i += ((*pos - 'a') + 36);	
		}
	}
	return i;
}

void string_add_one(char *s,int len)
{
	char *pos;
	for(pos = s + len - 1;pos != s - 1;--pos)
	{
		if(*pos == 'z')
			*pos = '0';
		else break;
	}
	if(*pos == '9')
	{
		*pos = 'A';
	}
	else 
	{
		if(*pos == 'Z')
		{
			*pos = 'a';
		}
		else
		{
			++(*pos);
		}
	}
	
	return ;
}
