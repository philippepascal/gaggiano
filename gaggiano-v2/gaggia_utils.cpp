#include "gaggia_utils.h"
#include <cstring>

int myIndexOF(const char *str, const char ch, int fromIndex) {
  const char *result = strchr(str + fromIndex, ch);
  if (result == NULL) {
    return -1;  // Substring not found
  } else {
    return result - str;  // Calculate the index
  }
}
char *mySubString(const char *str, int start, int end) {
  char *sub = (char *)malloc(sizeof(char) * (end - start));
  if (sub == NULL) {
    return NULL;
  }

  strncpy(sub, str + start, (end - start));
  sub[(end - start)] = '\0';

  return sub;
}