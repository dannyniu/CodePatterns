This memo intends to illustrate problems associated with the type system
in C, provide advices on avoiding various pitfalls, and provide advices on
having a consistent coding style.

In the following text, all C codes are compiled using `clang` with the flags
`-Wall -Wextra` under both ILP32 and LP64 compilation environments.

# 1. Built-In Types.

According to the 2nd edition of "The C Programming Language":

> There are only a few basic data types in C: 
>
> - `char`: a single byte, 
> capable of holding one character
> in the local character set.
>
> - `int`: an integer, 
> typically reflecting the natural size of integers on the host machine,
>
> - `float`: single-precision floating point,
>
> - `double`: double-precision floating point.
>
> In addition, there are a number of qualifiers* that can be applied
> to these basic types. `short` and `long` apply to integers ...
> The intent is that `short` and `long` should provide different lengths
> of integers where practical; `int` will normall be the natural size 
> for a particular machine. `short` is often 16 bits, `long` 32 bits, 
> and int either 16 or 32 bits. Each compiler is free to choose
> appropriate sizes for its own hardware subject only to the restriction
> that `short`s and `int`s are at least 16 bits, `long` are at least 32 bits 
> and `short` is no longer than `int`, which is no longer than `long`.
>
> The qualifier `signed` or `unsigned` may be applied to `char` or 
> any integer.
>
> ...
>
> The type `long double` specifies extended-precision floating point. 
> ...

*: not in the specification sense of qualifiers 
which refers to `const`, `volatile`, etc.

Over the course of standard development, new types had been added, 
such as `long long` and `_Complex` in C99, and the `_Decimal*` types in C2X.

Floating point numbers will not be the subject of this memo, interested
readers are recommended to purchase "Handbook of Floating Point Arithmetic"
by J-M Muller et.al. to learn more.

Obviously, as these integer types have implementation-defined sizes, they
cannot be used portably in definitions of data structures intended for
exchange with foreign computer systems. Thus C99 introduced the `<stdint.h>`
header for new type definitions, and `<inttypes.h>` which "extends it with
additional facilities provided by hosted implementations".

# 2. Implicit Type Conversions.

The "usual arithmetic conversions" governs how operands' types are determined
in arithmetic operations, and in the case of integers, also bitwise operations.

The ruleset for floating point and pointer operands are not the concern of 
this memo.

The ruleset (as of C17) for integers can be described as follow:

0. after *integer promotionn* 
   (converting lesser types to `int` or `unsigned int`),
1. identical types are preferred; over
2. types of same signedness with the higher rank; over
3. unsigned type of same or higher rank; over
4. signed type *wider* than the unsigned type; over
5. unsigned type with rank corresponding to the signed type.

Now a bit of my own interpretation: bullet points 3-5 are a bit of convoluted
but in simple terms it just means "unsigned types are preferred unless 
that other signed type can represent all my values", condition being "if
the signed type have higher rank than the unsigned type".

Let's illustrate this with an example. Suppose we have a `long long` and 
an `unsigned long`, on a ILP32 system, they'd be converted to `long long` 
because it's 64-bit and can represent all the values of both types; 
**but** on a LP64 system, both types are of same width, so they'd result in 
`unsigned long long`.

```plaintext
**Table of usual arithmetic conversoins applied to integer types under
various data models**

Type Abbreviations: 
{s,u}{c,s,i,l,ll}: {signed,unsigned}{char,short,int,long,long long}

Where signedness and rank differs from ILP32 are highlighted in upper case.

      sc  ss  si  sl  sll   uc  us  ui  ul  ull
SI16:
sc    si  si  si  sl  sll   si  Ui  ui  ul  ull
ss    si  si  si  sl  sll   si  Ui  ui  ul  ull
si    si  si  si  sl  sll   si  Ui  ui  ul  ull
sl    sl  sl  sl  sl  sll   sl  sl  sl  ul  ull
sll   sll sll sll sll sll   sll sll sll sll ull
uc    si  si  si  sl  sll   si  Ui  ui  ul  ull
us    Ui  Ui  Ui  sl  sll   Ui  Ui  ui  ul  ull
ui    ui  ui  ui  sl  sll   ui  ui  ui  ul  ull
ul    ul  ul  ul  ul  sll   ul  ul  ul  ul  ull
ull   ull ull ull ull ull   ull ull ull ull ull
ILP32:
sc    si  si  si  sl  sll   si  si  ui  ul  ull
ss    si  si  si  sl  sll   si  si  ui  ul  ull
si    si  si  si  sl  sll   si  si  ui  ul  ull
sl    sl  sl  sl  sl  sll   sl  sl  ul  ul  ull
sll   sll sll sll sll sll   sll sll sll sll ull
uc    si  si  si  sl  sll   si  si  ui  ul  ull
us    si  si  si  sl  sll   si  si  ui  ul  ull
ui    ui  ui  ui  ul  sll   ui  ui  ui  ul  ull
ul    ul  ul  ul  ul  sll   ul  ul  ul  ul  ull
ull   ull ull ull ull ull   ull ull ull ull ull
LP64:
sc    si  si  si  sl  sll   si  si  ui  ul  ull
ss    si  si  si  sl  sll   si  si  ui  ul  ull
si    si  si  si  sl  sll   si  si  ui  ul  ull
sl    sl  sl  sl  sl  sll   sl  sl  Sl  ul  ull
sll   sll sll sll sll sll   sll sll sll ull ull
uc    si  si  si  sl  sll   si  si  ui  ul  ull
us    si  si  si  sl  sll   si  si  ui  ul  ull
ui    ui  ui  ui  Sl  ull   ui  ui  ui  ul  ull
ul    ul  ul  ul  ul  ull   ul  ul  ul  ul  ull
ull   ull ull ull ull ull   ull ull ull ull ull

```

The rules used to be simpler in ANSI C (C89), as there was no `long long`
integer rank. In the 2nd edition of "The C Programming Language"", it is
summarized in Appendix C: 

> The "usual arithmetic conversions" are changed [from the 1st edition],
> essentially from "for integers, `unsigned` always wins; for floating point, 
> always use double" to "promote to the smallest capacious-enough type." ...

And in Appendix A section 6.5:

> There are two changes [from the 1st edition] here. First, arithmetic on 
> `float` operands ... Second, shorter unsigned types, when combined with a
> larger signed type, do not propagate the unsigned property to the 
> result type; ... . The new rules are slightly more complicated, but reduce
> somewhat the surprises that may occur when an unsigned entity meets signed. 
> Unexpected results may still occur when an unsigned expression is compared 
> to a signed expression of the same size.

# 3. Problems and Advices.

## 3.1. Case 1: Built-In Types of Implementation-Defined Size.

Consider the following example:

**Example 3-1**

```c
_Bool lcmp(unsigned long a, long long b)
{
    return a > b;
}
```

On ILP32 systems, this code compiles just fine, on LP64 environment, the 
compiler gives the following warning:

> warning: comparison of integers of different signs: 
> 'unsigned long' and 'long long' [-Wsign-compare]
>
> ...
>
> 1 warning generated.

More often, the warning is issued for `a` with a `typedef` type such as
`size_t`. In order to avoid the warning and possible erroneous behaviors, 
we must get back to what was originally intended.

- Possible Intention 1: `uint32_t` was intended for `a`.

If this was an old codebase, assumptions might have been made for the 
size of types, as such, when the new exact-width types are introduced in C,
those new types should be used. The function may therefore use `uint32_t` and
`int64_t` respectively for `a` and `b`.

- Possible Intention 2: `size_t` was intended for `a`, and `intmax_t` for `b`.

If `b` is meant to hold all possible values for some predicate, and `a` is the
size of some object, then this can reasonably occur. 

For finite values, it's reasonable to expect `b` be converted to unsigned.
But there is also some plausible edge cases, like `(a=0) > (b=-1)`.

As mentioned before, if this is a ILP32 system, such edge case would be safe,
but on LP64 systems, it's best if both are converted to *signed* 64-bit types.
This is recommended for 2 reason: 

1. Even 2^63 is a very big number, it's unlikely any number above it will
   ever be needed; 
   
2. It's more likely to catch unintentional overflow this way. 

It should be noted firstmost that this is not the correct way to make sure 
application logics are correct, if the application ever depends on such 
comparison, it's a clear sign that there's some inconsistency. It's strongly
advised the components relying on it be redesigned and rewritten.

But, when overly large `a` occur, converting it into signed will most likely
cause the `lcmp` to return false, thus preventing *some* incorrect behavior
**in some limited situations**.

However, this is just my personal opinion, if you find exception cases to it, 
you're more than welcome to tell me about it, and I'll pay you my appreciation.

- Possible Intention 3: *still trying to learn C programming*.

If so, they're recommended to read and learn more.

## 3.2. Case 2: A Variant of Case 1.

Consider the following example:

**Example 3-2**

```c
_Bool icmp(unsigned int a, long b)
{
    return a > b;
}
```

Unlike the previous case, this code fragment doesn't give off warning in LP64,
but in ILP32; also, the types are 32-bit now, so the suggestions from the 
previous case cannot apply. So what could be possibly intended?

- Possible Intention 1: The code is intended for both ILP32 and LP64.

In this case, it can be expected that `b` is actually something like `ssize_t`.
If so, the suggestion from the previous section can apply, convert both to a
signed type.

- Possible Intention 2: The code is an adaption from SI16 to ILP32.

If this is the case, the range of possible values to be compared are indeed
`[-2^31, 2^32)`, then we have to add a special condition to make it work:

```c
_Bool icmp(unsigned int a, long b)
{
    return b < 0 || a > b;
}
```

Also, to silence warnings, explicit type casting should be added.

## 3.3. Case 3: Exact-Width Bitfields.

Because the standard leaves the order of bit allocation in bitfield members
unspecified, it's often to see applications sensitive to the representation
of type values use integer words, bitwise operators, and endian conversion
functions to build bitfield data portably.

Consider the following example:

**Example 3-3**

```c
uint64_t set(uint64_t a, uint16_t b)
{
    a &= UINT64_C(0xffff) << 24;
    a |= b << 24;
    return a;
}
```

The intention in this example is clear - to set the 2 middle bytes of `a` to
`b`. While the compiler doesn't give off warnings on this code, there's 
something not quite right with it.

On ILP32 and LP64 systems, `b` is first promoted to `int` as the 32-bit `int`
can represent all values of `uint16_t`, then, after left-shifting 24 bits, 
bits 32-39 are truncated, and bits 32-63 are sign-extended from bit 31 as `b`
now have the signed type `int`.

So, it's a surprise to see that the value of the expression `set(0, -1)` is
`0xffffffffff000000`

The fix is simple though. Simply add a type cast to `b`, or if it's going to be
used multiple times, assign it to a local variable of type `uint64_t`, 
like this:

```c
uint64_t set_v1(uint64_t a, uint16_t b)
{
    a &= UINT64_C(0xffff) << 24;
    a |= (uint64_t)b << 24;
    return a;
}

uint64_t set_v2(uint64_t a, uint16_t b)
{
    uint64_t w = b;
    a &= UINT64_C(0xffff) << 24;
    a |= w << 24;
    ...
    return a;
}
```

# 4. Conclusion.

1. Built-in types are the foundation from which type conversion rules are
   defined on top of, and is not particularly useful for portability. 

2. Types with concrete names such as `size_t` and `time_t` from C, and 
   `uid_t`, `gid_t`, `pid_t`, `id_t`, and `mode_t`, `off_t`, etc. from POSIX
   are more useful. These should be the preferential choice, and ought to be
   used correctly - IDs should be compared only for equality and not for
   relation; bitmasks should only be applied bitwise operators; multiplication
   should not be applied to types representing absolute positions.

3. Only types with exact widths (`uint8_t`, `uint16_t`, etc.) should be 
   used in definitions for data structures intended for exchange with 
   foreign systems.

