#include <fltKernel.h>
#include <string.h>

/************************************************************************/
/*                     �ӽ��ܺ���  ʹ��RC4�㷨                           */
/************************************************************************/
//�û�s�е�����Ԫ��
void swap(unsigned char *s1, unsigned char *s2)
{
	char temp;
	temp = *s1;
	*s1 = *s2;
	*s2 = temp;
}

void re_S(unsigned char *S)
{
	unsigned int i;
	for (i = 0; i<256; i++)
		S[i] = (unsigned char)i;
}
//���ܳ�key��ʼ����ʱ����T
void re_T(char *T, char *key)
{
	int i;
	int keylen;
	keylen = strlen(key);
	for (i = 0; i<256; i++)
		T[i] = key[i%keylen];
}
//��T����S�ĳ�ʼ�û�����S[0]��S[255]����ÿ��S[i]��������T[i]ȷ���ķ�������S[i]�û�ΪS�е���һ�ֽ�
void re_Sbox(unsigned char *S, char *T)
{
	int i;
	int j = 0;
	for (i = 0; i<256; i++)
	{
		j = (j + S[i] + T[i]) % 256;
		swap(&S[i], &S[j]);//�û�
	}
}
//re_RC4()ʵ��S�����ĳ�ʼ��
void re_RC4(unsigned char *S, char *key)
{
	char T[255] = { 0 };
	re_S(S);
	re_T(T, key);
	re_Sbox(S, T);
}
//RC4�㷨
void RC4(char *inBuf, char *outBuf, LONGLONG offset, ULONG bufLen, char *key)
{
	unsigned char S[255] = { 0 };
	unsigned char readbuf[1];
	int i, j, t;
	LONGLONG z; //i��j�����û�s��t���ڻ�ȡ�ܳ�����z���ڴ���ӽ�������
	re_RC4(S, key);//����re_RC4()��ʼ��s
	//fileOffset=fileOffset%256;��������ܳ���û���������������ܻ��������������ԵĻ�����Ч�ʿ����һ��
	i = j = 0;
	z = 0;
	while (z<offset)//ƫ����֮ǰ�Ĳ����ܣ�ֻ���������ɵ�ƫ�������ܳ�
	{
		i = (i + 1) % 256;
		j = (j + S[i]) % 256;
		swap(&S[i], &S[j]);
		z++;
	}
	z = 0;
	while (z<bufLen)
	{
		i = (i + 1) % 256;
		j = (j + S[i]) % 256;
		swap(&S[i], &S[j]);
		t = (S[i] + (S[j] % 256)) % 256;//�����ܳ�������s�е��±�
		readbuf[0] = inBuf[z];
		//�����뻺������һ���ֽڸ�ֵ����ʱ������    
		readbuf[0] = readbuf[0] ^ S[t];
		outBuf[z] = readbuf[0];
		z++;
	}
}
