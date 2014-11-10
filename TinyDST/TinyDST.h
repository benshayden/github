// ATTINYs have very limited memory. Calculating whether a date is in DST from first principles (e.g. the Timezone library) can consume a lot of memory.
// TinyDST calculates whether a date is in DST using as little memory as possible using a pre-computed table.

// The pre-computed table will last until 2030. If you expect to still be using this library after that, then you'll need to re-compute the table.

// Example usage:
// https://github.com/benshayden/github/blob/master/lightclock/lightclock.ino

#ifndef _TINYDST_H_
#define _TINYDST_H_

// Returns true if day (as the number of days since 2000 Jan 1)
// is in Daylight Savings or Summer Time in the US.
boolean isDST(uint16_t day);

#endif  // _TINYDST_H_
