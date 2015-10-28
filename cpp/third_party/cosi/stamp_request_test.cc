#include <iostream>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include "stamp_request.h"

using namespace std;

int main(){
  const unsigned char bytes[5]={0x00, 0x40, 0x80, 0xc0, 0xff};
  cout << "Conversion: " << Cosi::BytesToHex(bytes, 5) << "\n";

  string test;
  cout << "Requesting signature\n";
  cout << Cosi::requestSignature("localhost", 2011, " 0 1 0 0 150FFFFFF56 0 0 0 0 0 0 0 0FFFFFF42FFFF1C14FFFFFFFFFF6FFF2427FF41FF64FFFF4CFFFFFF1B7852FF55" );

  ct::SignedTreeHead sth;
  sth.set_timestamp(1000);
  cout << "Asking STH to be signed\n";
  cout << Cosi::SignTreeHead( &sth );
  cout << "Signature received\n";
  return 0;
}