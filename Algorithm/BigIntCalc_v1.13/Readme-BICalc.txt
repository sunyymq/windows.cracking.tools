Big Integer Calculator v1.13
by readyu#gmail.com

[About]
Freeware.


[Help]
1. Support Integer size [0 - 32768] bits (multiply product to 65536 bits).
2. Limit: (MAX: 5900 !)
3. some help.

3.1
ALL answers are saved to [Ans|GCD], besides these:
(1)
X/Y, when Remainder(X%Y) is saved to [Rem.];
(2)
GCD(X,Y), when  LCM(X,Y) is saved to [LCM];
(3)
A*X=Y MOD Z, when B = (AX-Y)/Z is saved to [B];
(4)
X^(1/n), When X is not a perfect power of n, the Remainder is saved to [Rem.];

3.2
(1)
X^Y MOD Z 
(RSA en/decrypt, X=C, Y=E/D, Z=N)
(2)
AX = Y MOD Z (or A*X = Y + B*Z) 
A = (Y * (X^(-1) MOD Z)) MOD Z
Using Extended Euclid's Method. 
In RSA pub-priv-key calc, e*d = 1 mod phi, LET x=e, y=1, z = phi, you can calc d.

[ChangeLog]
v1.13 2008-05-01
+ auto trim hex chars

v1.12  2007-08-25
* fix an init b64 bug in v1.10
* fix Prime(X) dead loop when x=1
* minor interface change
+ add power_add function

v1.10f  2007-08-17
+ update bits status
* Use base change code from BaseAnyKit

v1.10  2007-08-17
+ Larger buffer size, bigger bits
  Add some new fuctions.
+ Multi-Multiply
+ Multiply-Add
+ Multi-PowMod
+ Prime(X)
+ Clear
+ Copy Answer to X
+ Improve interface

v1.05 2007-07-14
* fix base64 bug
+ updated bits status info
 
v1.02 2007-07-13
* fix  X^ (1/n) bug£»
* fix  AX = Y MOD Z bug in some condition judgement
+ add invalid digit input check



 

