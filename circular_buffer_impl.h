//
// Created by vprog on 4/5/2021.
//

#ifndef CIRCULAR_CIRCULAR_BUFFER_H
#define CIRCULAR_CIRCULAR_BUFFER_H
#include "details.h"
#include <scoped_allocator>
#include <cassert>

template<class T, class Allocator = std::allocator<T>>
class circular_buffer{
public:
#define begin_ buffer_

    using alloc_type = Allocator;
    using traits = std::allocator_traits<Allocator>;

    using self_type             = circular_buffer<T, Allocator>;
    using self_reference        = self_type&;
    using self_const_reference  = const self_type&;
    using self_rvalue           = self_type&&;

    using value_type        = typename traits::value_type;
    using size_type         = typename traits::size_type;
    using pointer           = typename traits::pointer;
    using difference_type   = typename traits::difference_type;
    using reference         = value_type&;
    using const_reference   = const value_type&;

    using capacity_type     = size_type;

    using iterator          = typename circular_buffer_details::iterator<self_type,
            circular_buffer_details::nonconst_traits<traits>>;
    using const_iterator    = typename circular_buffer_details::iterator<self_type,
            circular_buffer_details::const_traits<traits>>;

    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using reverse_iterator = std::reverse_iterator<iterator>;

    friend iterator;
private:
    size_type size_;
    capacity_type capacity_;
    alloc_type allocator_;
    pointer buffer_;
    pointer end_;
    pointer first_, last_;

public:

    explicit circular_buffer(size_type capacity = 0, alloc_type alloc = alloc_type())
            : allocator_(alloc)
            , capacity_(capacity)
            , size_(0)
    {
        initialize_buffer(capacity);
        begin_ = first_ = last_ = buffer_;
    }

    circular_buffer(self_const_reference cb)
            : allocator_(cb.get_allocator())
            , capacity_(cb.capacity())
            , size_(cb.size())
    {
        initialize_buffer(cb.capacity());
        begin_ = first_ = last_ = buffer_;
        pointer it = cb.first_, it_ = first_;
        for (;it != cb.last_; cb.next(it), next(it_))
        {
            *it_ = *it;
        }
        last_ = it_;
    }

    circular_buffer(self_rvalue cb)
            : allocator_(cb.get_allocator())
            , buffer_(nullptr)
            , capacity_(nullptr)
            , size_(nullptr)
            , end_(nullptr)
            , first_(nullptr)
            , last_(nullptr)
    {
        cb.swap(*this);
    }

    ~circular_buffer() {
        destroy();
    }

// push
    void push_back(const value_type& item)
    {
        if (capacity_ == 0) return;
        allocator_.construct(last_, item);
        next(last_);
        if (capacity_ == size_)
            first_ = last_;
        else
            ++size_;
    }

    void push_front(const value_type& item)
    {
        if (capacity_ == 0) return;
        prev(first_);
        allocator_.construct(first_, item);
        if (capacity_ ==  size_)
            last_ = first_;
        else
            ++size_;
    }

    void pop_back()
    {
        assert(size_ != 0);
        prev(last_);
        allocator_.destroy(last_);
        --size_;
    }

    void pop_front()
    {
        assert(size_ != 0);
        allocator_.destroy(first_);
        next(first_);
        --size_;
    }

    pointer out()
    {
        auto it = first_;
        do
        {
            std::cout << *it << ' ';
            next(it);
        } while (it != last_);
        return buffer_;
    }

//access methods
    alloc_type get_allocator() const
    {
        return allocator_;
    }

    reference operator[](size_type delta)
    {
        pointer ptr = first_;
        incr(ptr, delta);
        return *ptr;
    }

    reference front()
    {
        assert(size_ != 0);
        return *first_;
    }

    reference back()
    {
        assert(size_ != 0);
        pointer ptr = last_;
        prev(ptr);
        return *ptr;
    }

    reference at(size_type pos)
    {
        return *this[pos];
    }

//capacity methods
    void destroy() {
        for (size_type it = 0; it < size_; ++it, next(first_)) {
            std::allocator_traits<Allocator>::destroy(allocator_, std::addressof(*first_) );
        }
        deallocate(buffer_, capacity_);
    }

    void set_capacity(capacity_type new_capacity) {
        if (new_capacity == capacity_)
            return;
        pointer buff_ = allocate(new_capacity);
        iterator b = begin();
        reset(buff_,
              circular_buffer_details::copy(b, b + std::min(size(), new_capacity), buff_, allocator_),
              new_capacity);
    }

    [[nodiscard]] bool empty() const {
        return size_ == 0;
    }

    size_type size() const {
        return size_;
    }

    capacity_type capacity() const {
        return capacity_;
    }

//iterator methods
    iterator begin() {
        if (size_ == 0)
            return iterator();
        return iterator(this, first_);
    }

    iterator end() {
        if (size_ == 0)
            return iterator();
        return iterator(this, nullptr);
    }

    reverse_iterator rbegin() {
        return reverse_iterator(end());
    }

    reverse_iterator rend() {
        return reverse_iterator(begin());
    }


private:
    void reset(pointer new_buffer, pointer new_last, capacity_type new_capacity) {
        destroy();
        capacity_ = new_capacity;
        size_ = new_last - new_buffer;
        first_ = begin_ = buffer_ = new_buffer;
        end_ = buffer_ + new_capacity;
        last_ = (new_last == end_) ? buffer_ : new_last;
    }

    pointer allocate(capacity_type count) {
        return (count == 0) ? nullptr : allocator_.allocate(count);
    }

    void deallocate(pointer ptr, capacity_type count) {
        if (count == 0) return;
        allocator_.deallocate(ptr, count);
    }

    void initialize_buffer(capacity_type cb_capacity) {
        buffer_ = allocate(cb_capacity);
        end_    = buffer_ + cb_capacity;
    }

    void initialize_buffer(capacity_type cb_capacity, const_reference item) {
        initialize_buffer(cb_capacity);
        pointer ptr = buffer_;
        for (;ptr != end_; next(ptr))
        {
            *ptr = item;
        }
    }

    template<class Ptr>
    void next(Ptr& ptr) const {
        if (++ptr == end_) {
            ptr = buffer_;
        }
    }

    template<class Ptr>
    void prev(Ptr& ptr) const {
        if (ptr == buffer_) {
            ptr = end_;
        }
        --ptr;
    }

    void incr(pointer& ptr, difference_type delta) const {
        ptr += delta < (end_ - ptr) ? delta : delta - capacity_;
    }

    void decr(pointer& ptr, difference_type delta) const {
        ptr -= delta > (ptr - buffer_) ? delta - capacity_ : delta;
    }

    void swap(self_reference buff) {
        std::swap(buffer_, buff.buffer_);
        std::swap(first_, buff.last_);
        std::swap(last_, buff.last_);
        std::swap(size_, buff.size_);
        std::swap(capacity_, buff.capacity_);
        std::swap(begin_, buff.begin_);
        std::swap(end_, buff.end_);
    }
};

#endif //CIRCULAR_CIRCULAR_BUFFER_H
