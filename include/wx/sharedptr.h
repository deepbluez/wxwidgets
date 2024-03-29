/////////////////////////////////////////////////////////////////////////////
// Name:        wx/sharedptr.h
// Purpose:     Shared pointer based on the counted_ptr<> template, which
//              is in the public domain
// Author:      Robert Roebling, Yonat Sharon
// Copyright:   Robert Roebling
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_SHAREDPTR_H_
#define _WX_SHAREDPTR_H_

#include "wx/defs.h"
#include "wx/atomic.h"

// ----------------------------------------------------------------------------
// wxSharedPtr: A smart pointer with non-intrusive reference counting.
// ----------------------------------------------------------------------------

template <class T>
class wxSharedPtr
{
public:
    typedef T element_type;

    explicit wxSharedPtr( T* ptr = nullptr )
        : m_ref(nullptr)
    {
        if (ptr)
            m_ref = new reftype(ptr);
    }

    template<typename Deleter>
    explicit wxSharedPtr(T* ptr, Deleter d)
        : m_ref(nullptr)
    {
        if (ptr)
            m_ref = new reftype_with_deleter<Deleter>(ptr, d);
    }

    ~wxSharedPtr()                           { Release(); }
    wxSharedPtr(const wxSharedPtr& tocopy)   { Acquire(tocopy.m_ref); }

    wxSharedPtr& operator=( const wxSharedPtr& tocopy )
    {
        if (this != &tocopy)
        {
            Release();
            Acquire(tocopy.m_ref);
        }
        return *this;
    }

    wxSharedPtr& operator=( T* ptr )
    {
        if (get() != ptr)
        {
            Release();
            if (ptr)
                m_ref = new reftype(ptr);
        }
        return *this;
    }

    // test for pointer validity: defining conversion to unspecified_bool_type
    // and not more obvious bool to avoid implicit conversions to integer types
    typedef T *(wxSharedPtr<T>::*unspecified_bool_type)() const;
    operator unspecified_bool_type() const
    {
        if (m_ref && m_ref->m_ptr)
           return  &wxSharedPtr<T>::get;
        else
           return nullptr;
    }

    T& operator*() const
    {
        wxASSERT(m_ref != nullptr);
        wxASSERT(m_ref->m_ptr != nullptr);
        return *(m_ref->m_ptr);
    }

    T* operator->() const
    {
        wxASSERT(m_ref != nullptr);
        wxASSERT(m_ref->m_ptr != nullptr);
        return m_ref->m_ptr;
    }

    T* get() const
    {
        return m_ref ? m_ref->m_ptr : nullptr;
    }

    void reset( T* ptr = nullptr )
    {
        Release();
        if (ptr)
            m_ref = new reftype(ptr);
    }

    template<typename Deleter>
    void reset(T* ptr, Deleter d)
    {
        Release();
        if (ptr)
            m_ref = new reftype_with_deleter<Deleter>(ptr, d);
    }

    bool unique()   const    { return (m_ref ? m_ref->m_count == 1 : true); }
    long use_count() const   { return (m_ref ? (long)m_ref->m_count : 0); }

    template <class U>
    friend bool operator == (wxSharedPtr<T> const &a, wxSharedPtr<U> const &b )
    {
        return a.get() == b.get();
    }

    template <class U>
    friend bool operator != (wxSharedPtr<T> const &a, wxSharedPtr<U> const &b )
    {
        return a.get() != b.get();
    }

private:

    struct reftype
    {
        reftype(T* ptr) : m_ptr(ptr), m_count(1) {}
        virtual ~reftype() = default;
        virtual void delete_ptr() { delete m_ptr; }

        T*          m_ptr;
        wxAtomicInt m_count;
    };

    template<typename Deleter>
    struct reftype_with_deleter : public reftype
    {
        reftype_with_deleter(T* ptr, Deleter d) : reftype(ptr), m_deleter(d) {}
        virtual void delete_ptr() override { m_deleter(this->m_ptr); }

        Deleter m_deleter;
    };

    reftype* m_ref;

    void Acquire(reftype* ref)
    {
        m_ref = ref;
        if (ref)
            wxAtomicInc( ref->m_count );
    }

    void Release()
    {
        if (m_ref)
        {
            if (!wxAtomicDec( m_ref->m_count ))
            {
                m_ref->delete_ptr();
                delete m_ref;
            }
            m_ref = nullptr;
        }
    }
};

#endif // _WX_SHAREDPTR_H_
