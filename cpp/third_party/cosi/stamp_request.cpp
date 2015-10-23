#include <iostream>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <arpa/inet.h>

namespace Cosi{
  using std;

  string json_request = "{\"ReqNo\":0,\"Type\":1,\"Srep\":null,\"Sreq\":{\"Val\":\"";
  string json_request_end = "\"}}";
  string json_close = "{\"ReqNo\":1,\"Type\":3}\n";

  int connectTo(char *host, int port);
  int writeString(int m_sock, string msg);
  char *readString(int m_sock);
  char *HexToBytes(const string& hex);
  string requestSignature(char *host, int port);

  #define MAXRECV 1024

  string SignTreeHead(SignedTreeHead* sth){
    string serialized_sth;
    Serializer::SerializeResult res =
    Serializer::SerializeSTHSignatureInput(*sth, &serialized_sth);
    if (res != Serializer::OK) return GetSerializeError(res);

    return requestSignature("localhost", 2021, serialized_sth);
  }

  // Requests a signature from the stamp-server at host:port - returns NULL if
  // not successful or the string with the JSON-representation of the signature
  string requestSignature(char *host, int port, string msg ){
    int m_sock = connectTo(host, port);
    if ( m_sock < 0 ){
      return NULL;
    }

    string request = json_request + msg + json_request_end;
    if ( writeString(m_sock, request) < 0 ){
      return NULL;
    }

    string signature = readString(m_sock);;

    if ( writeString(m_sock, json_close) < 0 ){
      return NULL;
    }
    return signature;
  }

  int connectTo(char *host, int port){
    sockaddr_in m_addr;
    int m_sock = socket ( AF_INET, SOCK_STREAM, 0 );

    if ( m_sock == -1 ){
      return -1;
    }

    // TIME_WAIT - argh
    int on = 1;
    if ( setsockopt ( m_sock, SOL_SOCKET, SO_REUSEADDR, ( const char* ) &on, sizeof ( on ) ) == -1 ){
      return -1;
    }

    m_addr.sin_family = AF_INET;
    m_addr.sin_port = htons ( port );

    int status = inet_pton ( AF_INET, host, &m_addr.sin_addr );

    if ( errno == EAFNOSUPPORT ) return -1;

    status = ::connect ( m_sock, ( sockaddr * ) &m_addr, sizeof ( m_addr ) );

    if ( status < 0 ){
      return -1;
    }
    return m_sock;
  }

  int writeString(int m_sock, string msg){
    int status = ::write ( m_sock, msg.c_str(), msg.length() );

    if ( status == -1 ){
      int err=errno;
      cout << strerror(err);
      return -1;
    }
    return status;
  }

  char *readString(int m_sock){
    char *buf = (char*)malloc( MAXRECV + 1 );
    memset ( buf, 0, MAXRECV + 1 );
    int status = ::recv ( m_sock, buf, MAXRECV, 0 );
    if ( status == -1 ){
      free(buf);
      return NULL;
    }
    return buf;
  }

  char *HexToBytes(const string& hex) {
    char *bytes = (char*) malloc(hex.size());

    for (unsigned int i = 0; i < hex.length(); i += 2) {
      string byteString = hex.substr(i, 2);
      char byte = (char) strtol(byteString.c_str(), NULL, 16);
      bytes[i / 2] = byte;
    }

    return bytes;
  }
}
