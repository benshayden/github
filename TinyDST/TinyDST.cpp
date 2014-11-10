boolean isDST(uint16_t day) {
  if (day < 5181) return false;
  day -= 5181;
  // In the US, DST begins and ends on Sundays.
  day /= 7;
  for (uint16_t y = 0; y < 20; ++y) {
    // DST summer time always lasts 34 weeks.
    if (day < 34) return true;
    day -= 34;
    // standard winter time lasts 18 weeks most years except 2015, 2022, 2028.
    uint16_t w = 18;
    if ((y == 1) || (y == 8) || (y == 14)) ++w;
    if (day < w) return false;
    day -= w;
  }
  return false;
}
