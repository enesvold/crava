/***************************************************************************
*      Copyright (C) 2008 by Norwegian Computing Center and Statoil        *
***************************************************************************/

#include "lib/systemcall.h"
#include "nrlib/iotools/logkit.hpp"

#if defined(__WIN32__) || defined(WIN32) || defined(_WINDOWS)
#include <winsock2.h>
#include <windows.h>
#include <Lmcons.h>
#else
#include <unistd.h>
#include <pwd.h>
#endif

#include <ctime>
#include <string.h>

const std::string
SystemCall::getUserName()
{
#if defined(__WIN32__) || defined(WIN32) || defined(_WINDOWS)
  std::string strUserName;
  DWORD nUserName = UNLEN;
  char * userName = new char[nUserName];
  if (GetUserName(userName, &nUserName))
  {
    if (userName == NULL)
      strUserName= std::string("Empty user name found");
    else
      strUserName = userName;
  }
  else
  {
    strUserName = std::string("User name not found");
  }
  delete [] userName;
#else
  struct passwd on_the_stack;
  struct passwd* pw = 0;
  uid_t  uid    = geteuid();
  size_t size   = sysconf(_SC_GETPW_R_SIZE_MAX);
  char * buffer = new char[size];
#ifdef NO_GETPWUID_R_PTR
  pw = getpwuid_r(uid, &on_the_stack, buffer, size);
#else
  getpwuid_r(uid, &on_the_stack, buffer, size, &pw);
#endif
  std::string strUserName(pw->pw_name);
  delete [] buffer;
#endif
  return strUserName;
}

const std::string
SystemCall::getHostName()
{
  int max_string = 2400;
  char * hostname = new char[max_string];
#if defined(__WIN32__) || defined(WIN32) || defined(_WINDOWS)
   WORD wVersionRequested;
   WSADATA wsaData;
   wVersionRequested = MAKEWORD( 2, 2 );
   int err = WSAStartup( wVersionRequested, &wsaData );
   if ( err == 0 )
     gethostname(hostname, max_string);
   WSACleanup( );
#else
  int err = gethostname(hostname, max_string);
#endif

  std::string strHostname;

  if ( err == 0 )
    strHostname = std::string(hostname);
  else
    strHostname = std::string("*Not set*");

  delete [] hostname;

  return strHostname;
}

const std::string
SystemCall::getCurrentTime(void)
{
  time_t raw;
  time(&raw);

#if _MSC_VER >= 1400
  const size_t size = 26;
  char * buffer = new char[size];
  ctime_s(buffer, size, &raw);
  std::string strBuffer = std::string(buffer);
  delete [] buffer;
#else
  std::string strBuffer = std::string(ctime(&raw));
#endif

  return strBuffer;
}
