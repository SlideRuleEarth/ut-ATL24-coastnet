#pragma once

#include <iostream>
#include <vector>

namespace ATL24_coastnet
{

namespace raster
{

/// @brief raster container adapter
///
/// This is a simple container that provides 2D
/// subscripting and STL compatibility.
template<typename T, class Cont = std::vector<T> >
class raster
{
    private:
    // A raster is a container with rows X cols elements
    typename Cont::size_type rows_;
    typename Cont::size_type cols_;
    Cont cont_;

    public:
    // STL support
    //
    // By adding random access iterator support, we get support for STL algorithms.
    //
    // For example,
    //
    //     raster<int> r (4, 3);
    //     // 100 101 102
    //     // 103 104 105
    //     // 106 107 108
    //     // 109 110 111
    //     iota (r.begin (), r.end (), 100);
    //     // 107
    //     clog << r (2, 1) << endl;
    //     random_shuffle (r.begin (), r.end ());
    //     // v == 111
    //     int v = *max_element (r.begin(), r.end());
    //     sort (r.begin (), r.end ());
    //     // false
    //     clog << binary_search (r.begin (), r.end (), 99) << endl;
    //     // true
    //     clog << binary_search (r.begin (), r.end (), 105) << endl;
    //
    // See "C++ Concepts: RandomAccessIterator" for more details.
    typedef raster<T,Cont> self_type;
    typedef Cont container_type;
    typedef typename Cont::value_type value_type;
    typedef typename Cont::pointer pointer;
    typedef typename Cont::const_pointer const_pointer;
    typedef typename Cont::reference reference;
    typedef typename Cont::const_reference const_reference;
    typedef typename Cont::size_type size_type;
    typedef typename Cont::difference_type difference_type;
    typedef typename Cont::allocator_type allocator_type;
    typedef typename Cont::iterator iterator;
    typedef typename Cont::const_iterator const_iterator;
    typedef typename Cont::reverse_iterator reverse_iterator;
    typedef typename Cont::const_reverse_iterator const_reverse_iterator;

    /// @brief Default constructor
    /// @param a Optional custom allocator
    explicit raster (const allocator_type &a = allocator_type ())
        : rows_ (0), cols_ (0), cont_ (a)
    { }
    /// @brief Size constructor
    /// @param rows Number of rows in raster
    /// @param cols Number of columns in raster
    /// @param v Initialization value
    /// @param a Optional custom allocator
    raster (size_type rows, size_type cols, const T &v = T (),
        const allocator_type &a = allocator_type ())
        : rows_ (rows), cols_ (cols), cont_ (rows * cols, v, a)
    { }
    /// @brief Copy constructor
    /// @param r Raster to copy
    raster (const raster<T,Cont> &r)
        : rows_ (r.rows_), cols_ (r.cols_), cont_ (r.cont_)
    { }
    /// @brief Copy constructor
    /// @tparam R Raster source type
    /// @param r Raster source
    template<typename R,class C>
    raster (const raster<R,C> &r)
        : rows_ (r.rows ()), cols_ (r.cols ()), cont_ (r.size ())
    {
        typename raster<T,Cont>::iterator dest = this->begin ();
        typename raster<R,C>::const_iterator src = r.begin ();
        const typename raster<R,C>::const_iterator src_end = r.end ();
        while (src != src_end)
            *dest++ = *src++;
    }

    /// @brief Get dimensions
    /// @return The number of rows
    size_type rows () const
    { return rows_; }
    /// @brief Get dimensions
    /// @return The number of cols
    size_type cols () const
    { return cols_; }
    /// @brief Get number of elements in the raster
    /// @return The total number of elements in the raster
    size_type size () const
    { return cont_.size (); }
    /// @brief Reserve space for more elements
    /// @param n Number of elements to reserve
    void reserve (size_type n)
    { cont_.reserve (n); }
    /// @brief Reserve space for more elements
    /// @param rows Number of rows to reserve
    /// @param cols Number of cols to reserve
    void reserve (size_type rows, size_type cols)
    { cont_.reserve (rows * cols); }
    /// @brief Indicates if the raster has a zero dimension
    /// @return True if the raster is empty, otherwise false
    bool empty() const
    { return cont_.empty (); }

    /// @brief Swap this raster with another
    void swap (raster<T,Cont> &r)
    {
        std::swap (rows_, r.rows_);
        std::swap (cols_, r.cols_);
        std::swap (cont_, r.cont_);
    }
    /// @brief Copy assignment
    raster<T,Cont> &operator= (const raster<T,Cont> &rhs)
    {
        if (this != &rhs)
        {
            raster<T,Cont> tmp (rhs);
            swap (tmp);
        }
        return *this;
    }
    /// @brief Assign all element values
    /// @param v value to assign
    void assign (const T &v)
    { cont_.assign (cont_.size (), v); }

    /// @brief Element access
    reference front ()
    { return cont_.front (); }
    /// @brief Element access
    const_reference front () const
    { return cont_.front (); }
    /// @brief Element access
    reference back ()
    { return cont_.back (); }
    /// @brief Element access
    const_reference back () const
    { return cont_.back (); }

    /// @brief Random access
    /// @param i element index
    reference operator[] (size_type i)
    { return *(cont_.begin () + i); }
    /// @brief Random access
    /// @param i element index
    const_reference operator[] (size_type i) const
    { return *(cont_.begin () + i); }
    /// @brief Random access
    /// @param r element row
    /// @param c element col
    reference operator() (size_type r, size_type c)
    { return *(cont_.begin () + index (r, c)); }
    /// @brief Random access
    /// @param r element row
    /// @param c element col
    const_reference operator() (size_type r, size_type c) const
    { return *(cont_.begin () + index (r, c)); }
    /// @brief Checked random access
    /// @param r element row
    /// @param c element col
    ///
    /// Throws if the subscript is invalid.
    reference at (size_type r, size_type c)
    { return cont_.at (index (r, c)); }
    /// @brief Checked random access
    /// @param r element row
    /// @param c element col
    ///
    /// Throws if the subscript is invalid.
    const_reference at (size_type r, size_type c) const
    { return cont_.at (index (r, c)); }

    /// @brief Iterator access
    iterator begin ()
    { return cont_.begin (); }
    /// @brief Iterator access
    const_iterator begin () const
    { return cont_.begin (); }
    /// @brief Iterator access
    iterator end ()
    { return cont_.end (); }
    /// @brief Iterator access
    const_iterator end () const
    { return cont_.end (); }
    /// @brief Iterator access
    reverse_iterator rbegin ()
    { return cont_.rbegin (); }
    /// @brief Iterator access
    const_reverse_iterator rbegin () const
    { return cont_.rbegin (); }
    /// @brief Iterator access
    reverse_iterator rend ()
    { return cont_.rend (); }
    /// @brief Iterator access
    const_reverse_iterator rend () const
    { return cont_.rend (); }

    /// @brief Get an element index given its subscripts
    /// @param r element row
    /// @param c element col
    size_type index (size_type r, size_type c) const
    { return r * cols_ + c; }
    /// @brief Get an iterator's row subscript
    /// @param loc iterator
    size_type row (const_iterator loc) const
    { return (loc - begin ()) / cols_; }
    /// @brief Get an iterator's column subscript
    /// @param loc iterator
    size_type col (const_iterator loc) const
    { return (loc - begin ()) % cols_; }

    void clear ()
    {
        raster<T,Cont> tmp;
        swap (tmp);
    }

    /// @brief Compare two rasters
    template<typename R,typename C>
    friend bool operator== (const raster<R,C> &a, const raster<R,C> &b);

};

/// @brief Compare two rasters
template<typename T,typename Cont>
bool operator== (const raster<T,Cont> &a, const raster<T,Cont> &b)
{
    return a.rows_ == b.rows_ &&
        a.cols_ == b.cols_ &&
        a.cont_ == b.cont_;
}

/// @brief Compare two rasters
template<typename T,typename Cont>
bool operator!= (const raster<T,Cont> &a, const raster<T,Cont> &b)
{
    return !(a == b);
}

/// @brief Print a raster
template<typename T,typename Cont>
std::ostream& operator<< (std::ostream &s, const raster<T,Cont> &r)
{
    for (size_t i = 0; i < r.rows (); ++i)
    {
        for (size_t j = 0; j < r.cols (); ++j)
        {
            s << ' ' << r (i, j);
        }
        s << std::endl;
    }
    return s;
}

} // namespace raster

} // namespace ATL24_coastnet
