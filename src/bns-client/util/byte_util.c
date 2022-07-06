#include <bns-client/util/byte_util.h>
#include <ctype.h>
//change hex to byte
void bns_hex_to_byte(const char* hex, size_t size, unsigned char* byte) {
  //If hex or byte exist or size = 0 , return
  if (!hex || (size == 0) || !byte) { return; }
  // int i is 0 if even, -1 if odd. j is 0 . Continue to run as long as i is less than max size, 
  // increment i by 2, j by 1 
  for (int i = (size % 2 == 0 ? 0 : -1), j = 0; i < (int)size; i += 2, j++) {
    //set byte[j] to 0 
    byte[j] = 0;
    //if size is even
    if (i >= 0) {
      //add to byte[j] hex[i] without the 0,  
      if (isdigit(hex[i])) {
        byte[j] += (hex[i] - '0') * 16;
      } else {
        byte[j] += (tolower(hex[i]) - 'a' + 10) * 16;
      }
    }
    //TODO:always executes? 
    if (i >= -1) {
      if (isdigit(hex[i + 1])) {
        byte[j] += (hex[i + 1] - '0');
      } else {
        byte[j] += (tolower(hex[i + 1]) - 'a' + 10);
      }
    }
  }
}
//change byte to hex
void bns_byte_to_hex(const unsigned char* byte, size_t size, char* hex) {
  if (!byte || (size == 0) || !hex) { return; }
  size_t        index = 0;
  unsigned char h;
  unsigned char l;
  for (size_t i = 0; i < size; i++) {
    h = byte[i] / 16;
    l = byte[i] % 16;
    if (h <= 0x09) {
      hex[index++] = (char)(h + '0');
    } else {
      hex[index++] = (char)(h - 10 + 'a');
    }
    if (l <= 0x09) {
      hex[index++] = (char)(l + '0');
    } else {
      hex[index++] = (char)(l - 10 + 'a');
    }
  }
}
