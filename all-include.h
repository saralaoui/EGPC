#ifndef ALL_INCLUDE_H
#define ALL_INCLUDE_H
#include <stdio.h>
#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <stdarg.h>     // Header File For Variable Argument Routines
#include <string.h>

#include <time.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

#ifndef _WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#else
//#define socklen_t int
#include <winsock2.h>
#include <Ws2tcpip.h>
//#include <wspiapi.h>
#endif

#include <ctime>
#ifdef _WIN32
#include <windows.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <GL/glext.h>
#else
/*#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <GL/glx.h>
#include <GL/glext.h>*/
#endif

#include <float.h>
#include <tiffio.h>
//#include <Cg/cgGL.h>



/*#include "cg-lib.h"
#include "texjpeg.h"
#include "texpng.h"*/


#include "MessageMappingOscPacketListener.h"
#include "NetworkingUtils.h"
#include "OscException.h"
#include "OscHostEndianness.h"
#include "OscOutboundPacketStream.h"
#include "OscPacketListener.h"
#include "OscPrintReceivedElements.h"
#include "OscReceivedElements.h"
#include "OscTypes.h"
#include "PacketListener.h"
#include "TimerListener.h"
#include "UdpSocket.h"
#include "SarahPacketManager.h"
#include "IpEndpointName.h"

using std::string;
using std::ios;
using std::cerr;
using std::ifstream;
using namespace std;
#include <fstream>



#define IN_PORT 7000
#define ADDRESS "127.0.0.1"
#define OUT_PORT 7001

#define WITH_EXTERNAL_MESH

#define OUTPUT_BUFFER_SIZE 1024

extern float Kplus;
extern float Linit;
extern float Amort;
extern float mu;
extern float MassePlus;
extern int NB_MASSES;
extern int NB_EDGES;
extern float raideur[8];
extern float elongation[8];
extern float indice;
//extern int IN_PORT;


#endif
