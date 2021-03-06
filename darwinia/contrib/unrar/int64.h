﻿#ifndef _RAR_INT64_
#define _RAR_INT64_

#if defined(__BORLANDC__) || defined(_MSC_VER)
#define NATIVE_INT64
typedef __int64 Int64;
#endif

#if defined(__GNUC__)
#define NATIVE_INT64
typedef long long Int64;
#endif

#ifdef NATIVE_INT64

#define int64to32(x) ((unsigned int)(x))
#define int32to64(high,low) ((((Int64)(high))<<32)+(low))
#define is64plus(x) (x>=0)

#else

class Int64
{
  public:
    Int64();
    Int64(unsigned int n);
    Int64(unsigned int HighPart,unsigned int LowPart);

//    Int64 operator = (Int64 n);
    Int64 operator << (int n);
    Int64 operator >> (int n);

    friend Int64 operator / (Int64 n1,Int64 n2);
    friend Int64 operator * (Int64 n1,Int64 n2);
    friend Int64 operator % (Int64 n1,Int64 n2);
    friend Int64 operator + (Int64 n1,Int64 n2);
    friend Int64 operator - (Int64 n1,Int64 n2);
    friend Int64 operator += (Int64 &n1,Int64 n2);
    friend Int64 operator -= (Int64 &n1,Int64 n2);
    friend Int64 operator *= (Int64 &n1,Int64 n2);
    friend Int64 operator /= (Int64 &n1,Int64 n2);
    friend Int64 operator | (Int64 n1,Int64 n2);
    friend Int64 operator & (Int64 n1,Int64 n2);
    inline friend void operator -= (Int64 &n1,unsigned int n2)
    {
      if (n1.LowPart<n2)
        n1.HighPart--;
      n1.LowPart-=n2;
    }
    inline friend void operator ++ (Int64 &n)
    {
      if (++n.LowPart == 0)
        ++n.HighPart;
    }
    inline friend void operator -- (Int64 &n)
    {
      if (n.LowPart-- == 0)
        n.HighPart--;
    }
    friend bool operator == (Int64 n1,Int64 n2);
    friend bool operator > (Int64 n1,Int64 n2);
    friend bool operator < (Int64 n1,Int64 n2);
    friend bool operator != (Int64 n1,Int64 n2);
    friend bool operator >= (Int64 n1,Int64 n2);
    friend bool operator <= (Int64 n1,Int64 n2);

    void Set(unsigned int HighPart,unsigned int LowPart);
    unsigned int GetLowPart() {return(LowPart);}

    unsigned int LowPart;
    unsigned int HighPart;
};

inline unsigned int int64to32(Int64 n) {return(n.GetLowPart());}
#define int32to64(high,low) (Int64((high),(low)))
#define is64plus(x) ((int)(x).HighPart>=0)

#endif

#define INT64ERR int32to64(0x80000000,0)

void itoa(Int64 n,char *Str);
Int64 atoil(const char *Str);

#endif
