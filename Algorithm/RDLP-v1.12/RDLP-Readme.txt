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

说明：

1. 全局设置

Prime Size (16 -2048)
指的素数的大小bits,对RSA来说,N的大小是它的2倍。
RSA-512  用2个256 bits的素数；
RSA-4096 用2个2048 bits的素数；
一般不会采用更大的bits了。运算耗时，目前没有必要。

BASE
BASE 10, BASE 16 不用介绍。
其他三个: 
- Used Base36 conversion table:
  0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ
- Used Base60 conversion table:
  0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwx
- Used Base64 conversion table:
  ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/

RNG 的一些设置:
RNG主体采用安全HASH函数SHA-512, 与rc4加密。
收集了30多KB的buf pool, buf收集参考了windows的rng部分源代码。
保证hash的初始化与rc4密钥都是密码学强度随机的。

RNG salt由专门的线程刷新。可以加入自定义的键盘键入扰乱。

CPU 主频：
采用cpu time stamp counter 与 Windows的PerformanceFrequency(3.579545MHZ)对比。
计算出CPU主频，约每2s刷新一次。
由于Windows的高精度时间是MHZ的，CPU主频是GHz的，因此，精确度可做到KHZ。
也就是误差小于10KHZ。在Intel Celeron 1.7G 和 AMD64 3000+ 1.81G测试过。

2. 功能点介绍
(1)全局
Pause:
当前线程可以随时暂停/继续。当前没有任务运行时，此按钮为灰色。

Stop:
当前线程可以随时中断。当前没有任务运行时，此按钮为灰色。
虽然我做了很大处理，但是Windows的TerminateThread()必然泄露内存，所以慎用Stop。
另外,TerminateThread可能破坏线程堆栈，调试中发现，1024 bit以上的generate，
反复stop强制结束thread,如果正在运行到Miracl的mad函数，会引发溢出。
出现这个错误Exception:0xc0000095 (int_overflow)

(2) RSA-Rabin功能

Factor:
实现了较为简单的Brute,P+1,P-1,Rho,ECM。QS没有实现。
400 bits以内的因子分解推荐用msieve。速度很好。

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
生成key pairs.生成的素数都是Blum数，即 P=3 mod 4

Test: 
测试keypairs.包括RSA测试和Rabin测试。

Rev:
Reverse,翻转N。 Little "endian" <-> Big "endian"
比如：
12AB67 <-> 67AB12

PQE->D:
由公钥和P,Q,计算私钥。采用扩展欧几里德算法求逆元。

NED->PQ:
由N的任何一对正确的公私钥(E,D),可以计算出P,Q
E,D中有一个小于N^(1/2), 采用phi(N)搜索法;
E,D都很大，则采用解二次同余方程的概率算法。

Wiener Attack:
当密钥(E,D)中有一个大，一个小。
如果大的已知，记为E。小的未知，记为D。
如果 D < N^0.295, 则可以采用Wiener attack求出D。
算法是"连分数分解法"。

Fermat Attack:
N=P*Q， 如果P,Q比较靠近，|P-Q|小于或稍大于N的1/4 (bits size)，则Fermat分解法有效。
实例:
winmail 4.2的N，是fermat attack有效的
ADD1D47967E6ED701852554F88EEE22C416B71A26F3961AE922CFD6ECEC39D73
他的p,q很靠近,fermat attack一次就出来了

Next 2 Primes:
计算大于N的2个依次的素数。结果存于P,Q当中。
next 2 prime功能很有意思,你可以找到你喜欢的p,q.
比如,你可以生成含200个连续0的p,q。
不过这样的P,Q用来做RSA加密就不太好了，有所好必有所患。
会被社会工程的。

比如你可以找到如下的素数：
N =
800000000000000000000000000000000000000000000000000000000000
P =
80000000000000000000000000000000000000000000000000000000001D
Q = 
800000000000000000000000000000000000000000000000000000000083

(3) DLP功能

Generate: 
生成key pairs.生成的素数P是Blum数，即 P=3 mod 4

Test: 
测试keypairs.包括ELGAMAL加密、解密测试，和签名、验证测试。

New GX:
P不变，换一组G,X.

GXP-> Y:
输入 G,X,P,计算 Y = G^X mod P.
即离散幂数，POWMOD。
 
XYP-> G:
由Y = G^X mod P，计算出G.
G是Y的X次根模P，离散指数问题。
X=2，P为素数，可计算二次剩余的根。

XYP-> G:
由Y = G^X mod P，计算出G.
X是G为底Y的对数模P。离散对数问题。


