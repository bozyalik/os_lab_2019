#include "libModule.h"

uint64_t libModule(uint64_t a, uint64_t b, uint64_t mod) {
  uint64_t result = 0;
  a = a % mod; // Приведение a к модулю

  while (b > 0) {
    if (b % 2 == 1) // Если b нечетное
      result = (result + a) % mod; // Добавляем a к результату

    a = (a * 2) % mod; // Удваиваем a и приводим к модулю
    b /= 2; // Делим b на два
  }

  return result % mod; // Возвращаем результат по модулю
}
