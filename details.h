//
// Created by vprog on 4/6/2021.
//

#ifndef CIRCULAR_CIRCULAR_BUFFER_ITERATOR_H
#define CIRCULAR_CIRCULAR_BUFFER_ITERATOR_H
#include<iterator>

namespace circular_buffer_details {

    template<class Alloc>
    struct nonconst_traits {
        //basic types
        using value_type      = typename Alloc::value_type;
        using pointer         = typename Alloc::pointer;
        using reference       = value_type&;
        using size_type       = typename Alloc::size_type;
        using difference_type = typename Alloc::difference_type;

        //non-const self
        using nonconst_self   = nonconst_traits<Alloc>;
    };

    template<class Alloc>
    struct const_traits {
        //basic types
        using value_type      = typename Alloc::value_type;
        using pointer         = typename Alloc::pointer;
        using reference       = const value_type&;
        using size_type       = typename Alloc::size_type;
        using difference_type = typename Alloc::difference_type;

        //non-const self
        using nonconst_self   = nonconst_traits<Alloc>;
    };

    template<class Buffer, class Traits>
    struct iterator : public std::iterator<
            std::random_access_iterator_tag,
            typename Traits::value_type,
            typename Traits::difference_type,
            typename Traits::pointer,
            typename Traits::value_type&
    >
    {
        using value_type        = typename Traits::value_type;
        using difference_type   = typename Traits::difference_type;
        using pointer           = typename Traits::pointer;
        using reference         = typename Traits::reference;

        using buffer_type = Buffer;
        using traits_type = Traits;

        using non_const_iter    = iterator<Buffer, typename Traits::nonconst_self>;;

        const buffer_type* buff_;
        pointer ptr_;

        iterator() : buff_(nullptr), ptr_(nullptr) {}
        iterator(const non_const_iter& it) : buff_(it.buff_), ptr_(it.ptr_) {}
        iterator(const buffer_type* buff, pointer ptr) : buff_(buff), ptr_(ptr) {}
        iterator(iterator&& rvalue) : buff_(nullptr), ptr_(nullptr) {
            std::swap(this->buff_, rvalue.buff_);
            std::swap(this->ptr_, rvalue.ptr_);
        }

        iterator& operator =(const iterator& it) noexcept {
            if (this == &it)  return *this;
            buff_ = it.buff_;
            ptr_ = it.ptr_;
            return *this;
        }

        reference operator *() const {
            return *ptr_;
        }

        pointer operator->() const {
            return ptr_;
        }

        iterator& operator--() {
            if (ptr_ == nullptr)
                ptr_ = buff_->last_;
            buff_->prev(ptr_);
            return *this;
        }

        iterator operator--(int) {
            iterator tmp = *this;
            --*this;
            return std::move(tmp);
        }

        iterator& operator++() {
            buff_->next(ptr_);
            if (ptr_ == buff_->last_) ptr_ = nullptr;
            return *this;
        }

        iterator operator++(int) {
            iterator tmp = *this;
            ++*this;
            return std::move(tmp);
        }

        iterator& operator+=(const difference_type delta) {
            if (delta >= 0)
                buff_->incr(ptr_, delta);
            else
                buff_->decr(ptr_, -delta);
            if (ptr_ == buff_->last_) ptr_ = nullptr;
            return *this;
        }

        iterator& operator-=(const difference_type delta) {
            return *this += (-delta);
        }

        iterator operator - (difference_type delta) {
            return iterator(*this) -= delta;
        }

        iterator operator + (difference_type delta) {
            return iterator(*this) += delta;
        }

        reference operator [](difference_type delta) {
            return *(*this + delta);
        }

        difference_type operator - (const iterator& it) {
            if (ptr_ == it.ptr_) return 0;
            if (ptr_ == nullptr)
                return buff_->delta(buff_->last_, it.ptr_);
            if (it.ptr_ == nullptr)
                return buff_->delta(ptr_, buff_->last_);
            return buff_->delta(ptr_, it.ptr_);
        }

        bool operator==(const iterator& it) {
            return this->ptr_ == it.ptr_;
        }

        bool operator!=(const iterator& it) {
            return !(*this == it);
        }

        pointer linearize_pointer(const iterator& it) const {
            return it.ptr_ == 0 ? buff_->buffer_ + buff_->size_ :
                   (it.ptr_ < buff_->first_ ? it.ptr_ + (buff_->end_ - buff_->first_)
                                            : buff_->buffer_ + (it.ptr_ - buff_->first_));
        }

        difference_type operator - (iterator& it) {
            return linearize_pointer(*this) - linearize_pointer(it);
        }

        bool operator < (const iterator& it) const {
            return linearize_pointer(*this) < linearize_pointer(it);
        }

        bool operator > (const iterator& it) const {
            return linearize_pointer(*this) > linearize_pointer(it);
        }

        bool operator <= (const iterator& it) const {
            return linearize_pointer(*this) < linearize_pointer(it);
        }
    };

    template<class Iter, class Pointer, class Alloc>
    Pointer copy(Iter begin_, Iter end_, Pointer buff_, Alloc& allocator_) {
        for (; begin_ != end_; ++begin_, ++buff_) {
            std::allocator_traits<Alloc>::construct(allocator_, std::addressof(*buff_), *begin_);
        }
        return buff_;
    }
} // namespace circular_buffer_details

#endif //CIRCULAR_CIRCULAR_BUFFER_ITERATOR_H
