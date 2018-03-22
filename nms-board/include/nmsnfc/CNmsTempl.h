#ifndef CNMSTEMPL_H
#define CNMSTEMPL_H

#ifndef   NMSNFCTYPES_H
#include <NmsNfcTypes.h>
#endif

#ifndef   NMSNFCUTIL_H
#include <NmsNfcUtil.h>
#endif

#ifndef  _STRING_H
#include <string.h>
#endif

#ifndef   CNMSSYNCOBJS_H
#include <CNmsSyncObjs.h>
#endif

#ifndef _INC_MEMORY
#include <memory.h>
#endif

template <class elem_type> class CNmsArray 
{
public:
   CNmsArray ( int nInitSize = 5, int nGrowBy = 3 );
   CNmsArray ( const CNmsArray& a );

   virtual ~CNmsArray ();

   elem_type& operator[](int index) const; 

public:
   int	GetSize()     const { return _size; }
   int	GetCapacity() const { return _capacity; }
   BOOL IsEmpty()   const { return _size == 0; }

   elem_type* GetAt( int nIndex ) const;

   void Add( elem_type* e );
   void Add( elem_type e );
   void InsertAt( int index, elem_type* e );
   void InsertAt( int index, elem_type e );
   void SetAtGrow( int index, elem_type* e );

   int	IndexOf( const elem_type* e ) const; // return zero if not found
   BOOL Has ( const elem_type* e ) const { return IndexOf(e) > 0; };
   BOOL IsEqual( const CNmsArray& a ) const;

   void DeleteAt( int index );
   void RemoveAt( int index );
   void RemoveAll();
   void DeleteAll();

private:
   BOOL element_exist(int index) const; // return false if element not set or NULL
   void realloc (int new_capacity) ;

protected:
   void ** data;
   int _size;
   int _capacity;
   int _delta;
};


template <class elem_type> 
CNmsArray<elem_type>::CNmsArray(int init_capacity, int delta) 
{
    NMSASSERT( init_capacity > 0);
    data = new void *[ init_capacity ];

    memset(data, 0, init_capacity*sizeof(void*)/sizeof(char));

    _capacity = init_capacity;
    _delta = delta;
    _size = 0;
}

template <class elem_type> 
CNmsArray<elem_type>::CNmsArray(const CNmsArray& a) 
{
    data = new void *[ a._capacity ];
    _capacity = a._size;
    _delta = a._delta;
    _size = a._size;

    memcpy(data, a.data, _capacity*sizeof(void*)/sizeof(char));
}

template <class elem_type> 
elem_type* CNmsArray<elem_type>::GetAt( int nIndex ) const
{
    NMSASSERT( (nIndex >= 0) && (nIndex < _size) );
    NMSASSERT( data[nIndex] );

    return (elem_type* const) (data [nIndex]) ;
};

template <class elem_type> 
elem_type& CNmsArray<elem_type>::operator[](int index) const 
{
    return *GetAt(index);
}


template <class elem_type> 
BOOL CNmsArray<elem_type>::element_exist(int index) const 
{
    NMSASSERT( (index >= 1) && (index <= _size) );
    return (data [index - 1] != NULL) ;
}

template <class elem_type> 
CNmsArray<elem_type>::~CNmsArray() 
{
   delete [] data;
   data = 0;
}

template <class elem_type> 
void CNmsArray<elem_type>::InsertAt( int index, elem_type* e ) 
{
    NMSASSERT(index >= 0);

    if( index + 1 > _capacity ) 
    {
        realloc (index + _delta + 1);
        _size = index + 1;
    } 
    else 
    {
        if( _size == _capacity ) 
        {
            realloc (_capacity + _delta);
        }
        if( index >= _size ) 
        {
            _size = index + 1;
        } 
        else 
        {
            // we need to right shift on one position with rotate
            int elem_size = sizeof(void*) / sizeof(char);
            //char * buf = new char [elem_size];
            //memcpy( buf, (data+_size), elem_size );
            memmove( (data+index+1), (data+index),
                     (_size-index) * elem_size );
            //memcpy((data+index-1), buf, elem_size );
            _size++; 
        }
    }
    data [index] = e;
}

template <class elem_type> 
void CNmsArray<elem_type>::InsertAt( int index, elem_type e ) 
{
    elem_type* toInsert = new elem_type( e );
    InsertAt( index, toInsert );
}


template <class elem_type> 
void CNmsArray<elem_type>::SetAtGrow ( int index, elem_type* e ) 
{
    NMSASSERT(index >= 0);
    if(index < _size) 
    {
        data [index] = e;
    } 
    else
        InsertAt ( index, e );
}

template <class elem_type> 
void CNmsArray<elem_type>::Add (elem_type* e) 
{
    InsertAt ( _size, e );
}

template <class elem_type> 
void CNmsArray<elem_type>::Add (elem_type e) 
{
    InsertAt ( _size, e );
}

template <class elem_type> 
void CNmsArray<elem_type>::RemoveAt (int index) 
{
    NMSASSERT( (index >= 0) && (index < _size) );

    memmove( (data+index), (data+index+1),
            (_size - index - 1) * sizeof(void*) / sizeof(char) );
    _size -- ;
}

template <class elem_type> 
void CNmsArray<elem_type>::DeleteAt (int index) 
{
    NMSASSERT( (index >= 0) && (index < _size) );

    if (data[index])
    {
        delete (elem_type*) (data[index]);
        data[index] = 0;
    }
    RemoveAt (index);
}

template <class elem_type> 
void CNmsArray<elem_type>::DeleteAll (void) 
{
    for(int i = 0; i < _size; i++)
        if (data[i])
            delete (elem_type*) (data[i]);

    _size = 0;
}

template <class elem_type> 
void CNmsArray<elem_type>::RemoveAll()
{
    _size = 0;
}



template <class elem_type> 
void CNmsArray<elem_type>::realloc(int new_capacity) 
{ 
    NMSASSERT(new_capacity > 0);

    void ** newdata = new void *[ new_capacity ];

    memset(newdata, 0, new_capacity*sizeof(void*)/sizeof(char));

    memcpy( newdata, data,
      nms_min(_capacity, new_capacity) * sizeof(void*) / sizeof(char) );

    delete [] data;
    data = newdata;

    _capacity = new_capacity;
}

template <class elem_type> 
int CNmsArray<elem_type>::IndexOf(const elem_type* e) const 
{
    for( int i = 0; i < _size; i ++ )
        if (e == (const elem_type*)data[i])
            return i;
   return -1;
}

template <class elem_type> 
BOOL CNmsArray<elem_type>::IsEqual (const CNmsArray& a) const 
{
   if (_size == a._size) {
      for( int i=0; i < _size; i++)
         if ((data[i] == a.data[i]) == false)
            return false;
      return true;
   }
   return false;
}




template <class T> class CNmsQueueBase
{
public:
    CNmsQueueBase( BOOL bAutoDelete = FALSE );
  virtual ~CNmsQueueBase();


  virtual void      PutQueueObject( T object );
  virtual T         GetQueueObject();
  virtual BOOL      IsEmpty() const;
          void      DeleteAll();
          DWORD     GetSize() const;

protected:
    CNmsArray < T > *m_pQueueObjects;
    CNmsMutex       m_Mutex;
    BOOL            m_bAutoDelete;
};


template <class T > 
CNmsQueueBase< T >::CNmsQueueBase( BOOL bAutoDelete )
{
    m_bAutoDelete = bAutoDelete;
};

template <class T > 
CNmsQueueBase< T >::~CNmsQueueBase() 
{
    if( m_bAutoDelete )
        DeleteAll();
};

template <class T > 
void CNmsQueueBase< T >::PutQueueObject( T object )
{
    m_Mutex.Lock();
    m_pQueueObjects->Add( object );
    m_Mutex.Unlock();
};

template <class T > 
T CNmsQueueBase< T >::GetQueueObject()
{
    m_Mutex.Lock();

    NMSASSERT( m_pQueueObjects->GetSize() );

    T object;
    object = (*m_pQueueObjects)[0];
    m_pQueueObjects->DeleteAt( 0 );
    m_Mutex.Unlock();

return object;
};

template <class T > 
BOOL CNmsQueueBase< T >::IsEmpty() const
{
    return m_pQueueObjects->IsEmpty();
};

template <class T > 
DWORD CNmsQueueBase< T >::GetSize() const
{
    return m_pQueueObjects->GetSize();
};


template <class T > 
void CNmsQueueBase< T >::DeleteAll()
{
    m_pQueueObjects->DeleteAll();
};


template <class T> class CNmsQueue : public CNmsQueueBase < T >
{
public:
    CNmsQueue( int n, BOOL bAutoDelete );
  virtual ~CNmsQueue();
};

template <class T > 
CNmsQueue< T >::CNmsQueue( int n, BOOL bAutoDelete ) :
    CNmsQueueBase< T >( bAutoDelete )
{
    m_pQueueObjects = new CNmsArray< T >( n );
};

template <class T > 
CNmsQueue< T >::~CNmsQueue() 
{
//    delete (elem_type*) 
    delete (CNmsArray< T >*) m_pQueueObjects;
};




#endif // ifndef CNMSTEMPL_H

