#ifndef _COLLECTION
#define _COLLECTION

#ifndef   NMSNFCUTIL_H
#include <NmsNfcUtil.h>
#endif

#define MAXT 25     // elements per block

template<class TYPE>
inline void  NmsCopyElements( TYPE* pDest, const TYPE* pSrc, int nCount )
{
    // default is element-copy using assigment
    while( nCount-- )
        *pDest++ = *pSrc++;
}

template<class TYPE>
inline void  NmsConstructElements(TYPE* pElements, int nCount)
{
/*
    NMSASSERT(nCount == 0 ||
		AfxIsValidAddress(pElements, nCount * sizeof(TYPE)));
*/
	// first do bit-wise zero initialization
	memset((void*)pElements, 0, nCount * sizeof(TYPE));

	// then call the constructor(s)
/*
	for (; nCount--; pElements++)
		::new((void*)pElements) TYPE;
*/
	for (; nCount--; pElements++)
		pElements = ::new TYPE;
}
    
template<class TYPE>
inline void  NmsDestructElements(TYPE* pElements, int nCount)
{
/* ilugin
    NMSASSERT(nCount == 0 ||
		AfxIsValidAddress(pElements, nCount * sizeof(TYPE)));
*/
	// call the destructor(s)
	for (; nCount--; pElements++)
		pElements->~TYPE();
}
    
    
//
//  TYPE        type to keep
//  ARG_TYPE    type to return
//
/////////////////////////////////////////////////////////////////////////////
// SnmpArray<TYPE, ARG_TYPE>

template<class TYPE, class ARG_TYPE>
class SnmpArray
{
public:
// Construction
	SnmpArray();

// Attributes
	int GetSize() const;
	int GetUpperBound() const;
	void SetSize(int nNewSize, int nGrowBy = -1);

// Operations
	// Clean up
	void FreeExtra();
	void RemoveAll();

	// Accessing elements
	TYPE GetAt(int nIndex) const;
	void SetAt(int nIndex, ARG_TYPE newElement);
	TYPE& ElementAt(int nIndex);

	// Direct Access to the element data (may return NULL)
	const TYPE* GetData() const;
	TYPE* GetData();

	// Potentially growing the array
	void SetAtGrow(int nIndex, ARG_TYPE newElement);
	int Add(ARG_TYPE newElement);
	int Append(const SnmpArray& src);
	void Copy(const SnmpArray& src);

	// overloaded operator helpers
	TYPE operator[](int nIndex) const;
	TYPE& operator[](int nIndex);

	// Operations that move elements around
	void InsertAt(int nIndex, ARG_TYPE newElement, int nCount = 1);
	void RemoveAt(int nIndex, int nCount = 1);
	void InsertAt(int nStartIndex, SnmpArray* pNewArray);

// Implementation
protected:
	TYPE* m_pData;   // the actual array of data
	int m_nSize;     // # of elements (upperBound - 1)
	int m_nMaxSize;  // max allocated
	int m_nGrowBy;   // grow amount

public:
	~SnmpArray();
//	void Serialize(CArchive&);
#ifdef _DEBUG
//	void Dump(CDumpContext&) const;
	void AssertValid() const;
#endif
};

/////////////////////////////////////////////////////////////////////////////
// SnmpArray<TYPE, ARG_TYPE> inline functions

template<class TYPE, class ARG_TYPE>
inline int SnmpArray<TYPE, ARG_TYPE>::GetSize() const
	{ return m_nSize; }
template<class TYPE, class ARG_TYPE>
inline int SnmpArray<TYPE, ARG_TYPE>::GetUpperBound() const
	{ return m_nSize-1; }
template<class TYPE, class ARG_TYPE>
inline void SnmpArray<TYPE, ARG_TYPE>::RemoveAll()
	{ SetSize(0, -1); }
template<class TYPE, class ARG_TYPE>
inline TYPE SnmpArray<TYPE, ARG_TYPE>::GetAt(int nIndex) const
	{ NMSASSERT(nIndex >= 0 && nIndex < m_nSize);
		return m_pData[nIndex]; }
template<class TYPE, class ARG_TYPE>
inline void SnmpArray<TYPE, ARG_TYPE>::SetAt(int nIndex, ARG_TYPE newElement)
	{ NMSASSERT(nIndex >= 0 && nIndex < m_nSize);
		m_pData[nIndex] = newElement; }
template<class TYPE, class ARG_TYPE>
inline TYPE& SnmpArray<TYPE, ARG_TYPE>::ElementAt(int nIndex)
	{ NMSASSERT(nIndex >= 0 && nIndex < m_nSize);
		return m_pData[nIndex]; }
template<class TYPE, class ARG_TYPE>
inline const TYPE* SnmpArray<TYPE, ARG_TYPE>::GetData() const
	{ return (const TYPE*)m_pData; }
template<class TYPE, class ARG_TYPE>
inline TYPE* SnmpArray<TYPE, ARG_TYPE>::GetData()
	{ return (TYPE*)m_pData; }
template<class TYPE, class ARG_TYPE>
inline int SnmpArray<TYPE, ARG_TYPE>::Add(ARG_TYPE newElement)
	{ int nIndex = m_nSize;
		SetAtGrow(nIndex, newElement);
		return nIndex; }
template<class TYPE, class ARG_TYPE>
inline TYPE SnmpArray<TYPE, ARG_TYPE>::operator[](int nIndex) const
	{ return GetAt(nIndex); }
template<class TYPE, class ARG_TYPE>
inline TYPE& SnmpArray<TYPE, ARG_TYPE>::operator[](int nIndex)
	{ return ElementAt(nIndex); }

/////////////////////////////////////////////////////////////////////////////
// SnmpArray<TYPE, ARG_TYPE> out-of-line functions

template<class TYPE, class ARG_TYPE>
SnmpArray<TYPE, ARG_TYPE>::SnmpArray()
{
	m_pData = NULL;
	m_nSize = m_nMaxSize = m_nGrowBy = 0;
}

template<class TYPE, class ARG_TYPE>
SnmpArray<TYPE, ARG_TYPE>::~SnmpArray()
{
	NMSASSERT_VALID(this);

	if (m_pData != NULL)
	{
		NmsDestructElements<TYPE>(m_pData, m_nSize);
		delete[] (BYTE*)m_pData;
	}
}

template<class TYPE, class ARG_TYPE>
void SnmpArray<TYPE, ARG_TYPE>::SetSize(int nNewSize, int nGrowBy)
{
	NMSASSERT_VALID(this);
	NMSASSERT(nNewSize >= 0);

	if (nGrowBy != -1)
		m_nGrowBy = nGrowBy;  // set new size

	if (nNewSize == 0)
	{
		// shrink to nothing
		if (m_pData != NULL)
		{
			NmsDestructElements<TYPE>(m_pData, m_nSize);
			delete[] (BYTE*)m_pData;
			m_pData = NULL;
		}
		m_nSize = m_nMaxSize = 0;
	}
	else if (m_pData == NULL)
	{
		// create one with exact size
#ifdef SIZE_T_MAX
		NMSASSERT(nNewSize <= SIZE_T_MAX/sizeof(TYPE));    // no overflow
#endif
		m_pData = (TYPE*) new BYTE[nNewSize * sizeof(TYPE)];
		NmsConstructElements<TYPE>(m_pData, nNewSize);
		m_nSize = m_nMaxSize = nNewSize;
	}
	else if (nNewSize <= m_nMaxSize)
	{
		// it fits
		if (nNewSize > m_nSize)
		{
			// initialize the new elements
			NmsConstructElements<TYPE>(&m_pData[m_nSize], nNewSize-m_nSize);
		}
		else if (m_nSize > nNewSize)
		{
			// destroy the old elements
			NmsDestructElements<TYPE>(&m_pData[nNewSize], m_nSize-nNewSize);
		}
		m_nSize = nNewSize;
	}
	else
	{
		// otherwise, grow array
		int nGrowBy = m_nGrowBy;
		if (nGrowBy == 0)
		{
			// heuristically determine growth when nGrowBy == 0
			//  (this avoids heap fragmentation in many situations)
			nGrowBy = m_nSize / 8;
			nGrowBy = (nGrowBy < 4) ? 4 : ((nGrowBy > 1024) ? 1024 : nGrowBy);
		}
		int nNewMax;
		if (nNewSize < m_nMaxSize + nGrowBy)
			nNewMax = m_nMaxSize + nGrowBy;  // granularity
		else
			nNewMax = nNewSize;  // no slush

		NMSASSERT(nNewMax >= m_nMaxSize);  // no wrap around
#ifdef SIZE_T_MAX
		NMSASSERT(nNewMax <= SIZE_T_MAX/sizeof(TYPE)); // no overflow
#endif
		TYPE* pNewData = (TYPE*) new BYTE[nNewMax * sizeof(TYPE)];

		// copy new data from old
		memcpy(pNewData, m_pData, m_nSize * sizeof(TYPE));

		// construct remaining elements
		NMSASSERT(nNewSize > m_nSize);
		NmsConstructElements<TYPE>(&pNewData[m_nSize], nNewSize-m_nSize);

		// get rid of old stuff (note: no destructors called)
		delete[] (BYTE*)m_pData;
		m_pData = pNewData;
		m_nSize = nNewSize;
		m_nMaxSize = nNewMax;
	}
}

template<class TYPE, class ARG_TYPE>
int SnmpArray<TYPE, ARG_TYPE>::Append(const SnmpArray& src)
{
	NMSASSERT_VALID(this);
	NMSASSERT(this != &src);   // cannot append to itself

	int nOldSize = m_nSize;
	SetSize(m_nSize + src.m_nSize);
	NmsCopyElements<TYPE>(m_pData + nOldSize, src.m_pData, src.m_nSize);
	return nOldSize;
}

template<class TYPE, class ARG_TYPE>
void SnmpArray<TYPE, ARG_TYPE>::Copy(const SnmpArray& src)
{
	NMSASSERT_VALID(this);
	NMSASSERT(this != &src);   // cannot append to itself

	SetSize(src.m_nSize);
	NmsCopyElements <TYPE> (m_pData, src.m_pData, src.m_nSize);
}

template<class TYPE, class ARG_TYPE>
void SnmpArray<TYPE, ARG_TYPE>::FreeExtra()
{
	NMSASSERT_VALID(this);

	if (m_nSize != m_nMaxSize)
	{
		// shrink to desired size
#ifdef SIZE_T_MAX
		NMSASSERT(m_nSize <= SIZE_T_MAX/sizeof(TYPE)); // no overflow
#endif
		TYPE* pNewData = NULL;
		if (m_nSize != 0)
		{
			pNewData = (TYPE*) new BYTE[m_nSize * sizeof(TYPE)];
			// copy new data from old
			memcpy(pNewData, m_pData, m_nSize * sizeof(TYPE));
		}

		// get rid of old stuff (note: no destructors called)
		delete[] (BYTE*)m_pData;
		m_pData = pNewData;
		m_nMaxSize = m_nSize;
	}
}

template<class TYPE, class ARG_TYPE>
void SnmpArray<TYPE, ARG_TYPE>::SetAtGrow(int nIndex, ARG_TYPE newElement)
{
	NMSASSERT_VALID(this);
	NMSASSERT(nIndex >= 0);

	if (nIndex >= m_nSize)
		SetSize(nIndex+1, -1);
	m_pData[nIndex] = newElement;
}

template<class TYPE, class ARG_TYPE>
void SnmpArray<TYPE, ARG_TYPE>::InsertAt(int nIndex, ARG_TYPE newElement, int nCount /*=1*/)
{
	NMSASSERT_VALID(this);
	NMSASSERT(nIndex >= 0);    // will expand to meet need
	NMSASSERT(nCount > 0);     // zero or negative size not allowed

	if (nIndex >= m_nSize)
	{
		// adding after the end of the array
		SetSize(nIndex + nCount, -1);   // grow so nIndex is valid
	}
	else
	{
		// inserting in the middle of the array
		int nOldSize = m_nSize;
		SetSize(m_nSize + nCount, -1);  // grow it to new size
		// destroy intial data before copying over it
		NmsDestructElements<TYPE>(&m_pData[nOldSize], nCount);
		// shift old data up to fill gap
		memmove(&m_pData[nIndex+nCount], &m_pData[nIndex],
			(nOldSize-nIndex) * sizeof(TYPE));

		// re-init slots we copied from
		NmsConstructElements<TYPE>(&m_pData[nIndex], nCount);
	}

	// insert new value in the gap
	NMSASSERT(nIndex + nCount <= m_nSize);
	while (nCount--)
		m_pData[nIndex++] = newElement;
}

template<class TYPE, class ARG_TYPE>
void SnmpArray<TYPE, ARG_TYPE>::RemoveAt(int nIndex, int nCount)
{
	NMSASSERT_VALID(this);
	NMSASSERT(nIndex >= 0);
	NMSASSERT(nCount >= 0);
	NMSASSERT(nIndex + nCount <= m_nSize);

	// just remove a range
	int nMoveCount = m_nSize - (nIndex + nCount);
	NmsDestructElements<TYPE>(&m_pData[nIndex], nCount);
	if (nMoveCount)
		memmove(&m_pData[nIndex], &m_pData[nIndex + nCount],
			nMoveCount * sizeof(TYPE));
	m_nSize -= nCount;
}

template<class TYPE, class ARG_TYPE>
void SnmpArray<TYPE, ARG_TYPE>::InsertAt(int nStartIndex, SnmpArray* pNewArray)
{
	NMSASSERT_VALID(this);
	NMSASSERT(pNewArray != NULL);
	NMSASSERT_VALID(pNewArray);
	NMSASSERT(nStartIndex >= 0);

	if (pNewArray->GetSize() > 0)
	{
		InsertAt(nStartIndex, pNewArray->GetAt(0), pNewArray->GetSize());
		for (int i = 0; i < pNewArray->GetSize(); i++)
			SetAt(nStartIndex + i, pNewArray->GetAt(i));
	}
}


#ifdef _DEBUG

template<class TYPE, class ARG_TYPE>
void SnmpArray<TYPE, ARG_TYPE>::AssertValid() const
{
	CObject::AssertValid();

	if (m_pData == NULL)
	{
		NMSASSERT(m_nSize == 0);
		NMSASSERT(m_nMaxSize == 0);
	}
	else
	{
		NMSASSERT(m_nSize >= 0);
		NMSASSERT(m_nMaxSize >= 0);
		NMSASSERT(m_nSize <= m_nMaxSize);
		NMSASSERT(AfxIsValidAddress(m_pData, m_nMaxSize * sizeof(TYPE)));
	}
}
#endif //_DEBUG




#endif  // end if _COLLECTION

