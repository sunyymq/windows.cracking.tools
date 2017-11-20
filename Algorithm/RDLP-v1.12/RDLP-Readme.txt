RDLP - RSA, Rabin, DSA/DSS Keygenerator and DLP Tool
readyu [at] gmail [dot] com

v1.12, May 10th 2008
+ auto trim invalid input chars
* improve factor display
* improve Wiener attack

v1.11, Apr. 1st 2008
+ DSA/DSS key generator and test

v1.10, Mar. 29th 2008
+ use kcm to improve powmod speed
+ add MPQS factor method

v1.09, Mar. 12th 2008
+ GUI improved
* handle leak fixed
* by chance crash in factor fixed(such as gcd() crash in rho, 51E22C09D3)

v1.08, Mar. 08th 2008
+ DLP: add StrongPrime option
+ DLP: add Nyberg-Rueppel Scheme Signature & Verify
+ split & update rng152 module

v1.07f, Jan. 2008
+ add ECM parametes(ecm_num and ecm_k)
* fix init_rng_system, Error "No Floppy Drive"

v1.07, Dec. 2007
+ add benchmark
+ add quadratic residue
+ add a new rng salt type,hex bytes/all printable chars
+ add a pause button

v1.04-v1.06 beta
* fix some bugs,not public
+ add factor: Brute,P+1,P-1,Rho,ECM,QS(not yet)
+ add rc4 rng

v1.03 beta
* fix some minor bugs
* fix Rabin-Test bug
* improve rng
+ DLP base check
 
v1.01 Publi beta
+ fisrt public version

˵����

1. ȫ������

Prime Size (16 -2048)
ָ�������Ĵ�Сbits,��RSA��˵,N�Ĵ�С������2����
RSA-512  ��2��256 bits��������
RSA-4096 ��2��2048 bits��������
һ�㲻����ø����bits�ˡ������ʱ��Ŀǰû�б�Ҫ��

BASE
BASE 10, BASE 16 ���ý��ܡ�
��������: 
- Used Base36 conversion table:
  0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ
- Used Base60 conversion table:
  0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwx
- Used Base64 conversion table:
  ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/

RNG ��һЩ����:
RNG������ð�ȫHASH����SHA-512, ��rc4���ܡ�
�ռ���30��KB��buf pool, buf�ռ��ο���windows��rng����Դ���롣
��֤hash�ĳ�ʼ����rc4��Կ��������ѧǿ������ġ�

RNG salt��ר�ŵ��߳�ˢ�¡����Լ����Զ���ļ��̼������ҡ�

CPU ��Ƶ��
����cpu time stamp counter �� Windows��PerformanceFrequency(3.579545MHZ)�Աȡ�
�����CPU��Ƶ��Լÿ2sˢ��һ�Ρ�
����Windows�ĸ߾���ʱ����MHZ�ģ�CPU��Ƶ��GHz�ģ���ˣ���ȷ�ȿ�����KHZ��
Ҳ�������С��10KHZ����Intel Celeron 1.7G �� AMD64 3000+ 1.81G���Թ���

2. ���ܵ����
(1)ȫ��
Pause:
��ǰ�߳̿�����ʱ��ͣ/��������ǰû����������ʱ���˰�ťΪ��ɫ��

Stop:
��ǰ�߳̿�����ʱ�жϡ���ǰû����������ʱ���˰�ťΪ��ɫ��
��Ȼ�����˺ܴ�������Windows��TerminateThread()��Ȼй¶�ڴ棬��������Stop��
����,TerminateThread�����ƻ��̶߳�ջ�������з��֣�1024 bit���ϵ�generate��
����stopǿ�ƽ���thread,����������е�Miracl��mad�����������������
�����������Exception:0xc0000095 (int_overflow)

(2) RSA-Rabin����

Factor:
ʵ���˽�Ϊ�򵥵�Brute,P+1,P-1,Rho,ECM��QSû��ʵ�֡�
400 bits���ڵ����ӷֽ��Ƽ���msieve���ٶȺܺá�

//////////////////////////////////////////////
msieve, with qs and gnfs factor core:
msieve v1.32:
http://www.boo.net/~jasonp/qs.html

Tested msieve v1.30:
CPU CM530, 1.73G, MEM 1G, DDRII-667
using msieve v1.30, 
when 250+bits, time estimated,
every +10 bits, need +2 times,
every +40 bits, need +10 times,
every +120 bits, need +1000 times.

200 bits:
elapsed time 00:00:14

256 bits:
elapsed time 00:09:33

281 bits:
elapsed time 00:35:59

321 bits:
elapsed time 06:18:20
//////////////////////////////////////////////

Generate: 
����key pairs.���ɵ���������Blum������ P=3 mod 4

Test: 
����keypairs.����RSA���Ժ�Rabin���ԡ�

Rev:
Reverse,��תN�� Little "endian" <-> Big "endian"
���磺
12AB67 <-> 67AB12

PQE->D:
�ɹ�Կ��P,Q,����˽Կ��������չŷ������㷨����Ԫ��

NED->PQ:
��N���κ�һ����ȷ�Ĺ�˽Կ(E,D),���Լ����P,Q
E,D����һ��С��N^(1/2), ����phi(N)������;
E,D���ܴ�����ý����ͬ�෽�̵ĸ����㷨��

Wiener Attack:
����Կ(E,D)����һ����һ��С��
��������֪����ΪE��С��δ֪����ΪD��
��� D < N^0.295, ����Բ���Wiener attack���D��
�㷨��"�������ֽⷨ"��

Fermat Attack:
N=P*Q�� ���P,Q�ȽϿ�����|P-Q|С�ڻ��Դ���N��1/4 (bits size)����Fermat�ֽⷨ��Ч��
ʵ��:
winmail 4.2��N����fermat attack��Ч��
ADD1D47967E6ED701852554F88EEE22C416B71A26F3961AE922CFD6ECEC39D73
����p,q�ܿ���,fermat attackһ�ξͳ�����

Next 2 Primes:
�������N��2�����ε��������������P,Q���С�
next 2 prime���ܺ�����˼,������ҵ���ϲ����p,q.
����,��������ɺ�200������0��p,q��
����������P,Q������RSA���ܾͲ�̫���ˣ������ñ���������
�ᱻ��Ṥ�̵ġ�

����������ҵ����µ�������
N =
800000000000000000000000000000000000000000000000000000000000
P =
80000000000000000000000000000000000000000000000000000000001D
Q = 
800000000000000000000000000000000000000000000000000000000083

(3) DLP����

Generate: 
����key pairs.���ɵ�����P��Blum������ P=3 mod 4

Test: 
����keypairs.����ELGAMAL���ܡ����ܲ��ԣ���ǩ������֤���ԡ�

New GX:
P���䣬��һ��G,X.

GXP-> Y:
���� G,X,P,���� Y = G^X mod P.
����ɢ������POWMOD��
 
XYP-> G:
��Y = G^X mod P�������G.
G��Y��X�θ�ģP����ɢָ�����⡣
X=2��PΪ�������ɼ������ʣ��ĸ���

XYP-> G:
��Y = G^X mod P�������G.
X��GΪ��Y�Ķ���ģP����ɢ�������⡣


