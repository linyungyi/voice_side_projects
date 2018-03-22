#ifndef CNMSSTRING_H
#define CNMSSTRING_H
                                     
#ifndef	  NMSNFCDEFS_H
#include <NmsNfcDefs.h>
#endif
          
#ifndef _INC_STDARG
#include <stdarg.h>
#endif
    
class _NMSNFCCLASS CNmsString
{
public: 
    CNmsString();
    CNmsString( const CNmsString& src );
    CNmsString( LPCSTR src );
    CNmsString( BYTE* pbString, int length );

  virtual ~CNmsString();

   int		GetLength() const;
   void		Empty();
   BOOL		IsEmpty() const;
   char     GetAt( int nIndex ) const;
   void     SetAt( int nIndex, char ch );
   void     InsertAt( int nIndex, char ch );

   int        Find( char ch, int nStart = 0 ) const;
   int        ReverseFind( char ch ) const;
   CNmsString Right( int nCount );
   CNmsString Left( int nCount );
   CNmsString Mid( int nStart, int nCount );

   LPCSTR	c_str() const;

   operator	LPCSTR() const;
   char operator [] ( int nIndex ) const;

   CNmsString& operator = ( const CNmsString& src );
   CNmsString& operator = ( LPCSTR src );

   CNmsString& operator += ( const CNmsString& s);
   CNmsString& operator += ( LPCSTR s );
   CNmsString& operator += ( char c );

   friend _NMSNFCCLASS CNmsString  operator +( const CNmsString& string, LPCSTR lpsz );
   friend _NMSNFCCLASS CNmsString  operator +( const CNmsString& string1, const CNmsString& string2 );

   CNmsString& operator << ( char c );
   CNmsString& operator << ( int );
   CNmsString& operator << ( long );
   CNmsString& operator << ( LPCSTR s );

   void MakeUpper();

   friend _NMSNFCCLASS BOOL operator == ( const CNmsString& s1, const CNmsString& s2 );
   friend _NMSNFCCLASS BOOL operator < ( const CNmsString& s1, const CNmsString& s2 );

   void Format( LPCSTR lpszFormat, ... );

private:
   int	compare( LPCSTR pattern) const;
//   int	compare_nocase( LPCSTR pattern) const;
   void formatV(LPCSTR lpszFormat, va_list argList);

private:
   char*	m_szBuffer;
};
#endif // ifndef CNMSSTRING_H //

