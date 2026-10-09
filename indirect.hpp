/**
 * \file indirect.hpp
 * C++20 std::indirect Implementation
 * \copyright Copyright (c) 2026 Justin Singh-Mohudpur.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE.md or copy at https://www.boost.org/LICENSE_1_0.txt)
 */

#pragma once

#include <initializer_list>
#include <memory>
#include <type_traits>
#include <utility>

namespace jsm {

namespace detail {

// by Raymond Chen, https://devblogs.microsoft.com/oldnewthing/20190710-00/?p=102678/
template<typename, typename = void>
constexpr bool is_type_complete_v = false;

template<typename T>
constexpr bool is_type_complete_v<T, std::void_t<decltype(sizeof(T))>> = true;

template<typename T>
concept complete_type = is_type_complete_v<T>;

} // namespace detail

/* 20.5.1.2 Synopsis [indirect.syn]

template<class T, class Allocator = allocator<T>>
class indirect {
public:
using value_type = T;
using allocator_type = Allocator;
using pointer = allocator_traits<Allocator>::pointer;
using const_pointer = allocator_traits<Allocator>::const_pointer;

// [indirect.ctor], constructors
constexpr explicit indirect();
constexpr explicit indirect(allocator_arg_t, const Allocator& a);
constexpr indirect(const indirect& other);
constexpr indirect(allocator_arg_t, const Allocator& a, const indirect& other);
constexpr indirect(indirect&& other) noexcept;
constexpr indirect(allocator_arg_t, const Allocator& a, indirect&& other)
    noexcept(see below);
template<class U = T>
    constexpr explicit indirect(U&& u);
template<class U = T>
    constexpr explicit indirect(allocator_arg_t, const Allocator& a, U&& u);
template<class... Us>
    constexpr explicit indirect(in_place_t, Us&&... us);
template<class... Us>
    constexpr explicit indirect(allocator_arg_t, const Allocator& a,
                                in_place_t, Us&&... us);
template<class I, class... Us>
    constexpr explicit indirect(in_place_t, initializer_list<I> ilist, Us&&... us);
template<class I, class... Us>
    constexpr explicit indirect(allocator_arg_t, const Allocator& a,
                                in_place_t, initializer_list<I> ilist, Us&&... us);

// [indirect.dtor], destructor
constexpr ~indirect();

// [indirect.assign], assignment
constexpr indirect& operator=(const indirect& other);
constexpr indirect& operator=(indirect&& other) noexcept(see below);
template<class U = T>
    constexpr indirect& operator=(U&& u);

// [indirect.obs], observers
constexpr const T& operator*() const & noexcept;
constexpr T& operator*() & noexcept;
constexpr const T&& operator*() const && noexcept;
constexpr T&& operator*() && noexcept;
constexpr const_pointer operator->() const noexcept;
constexpr pointer operator->() noexcept;
constexpr bool valueless_after_move() const noexcept;
constexpr allocator_type get_allocator() const noexcept;

// [indirect.swap], swap
constexpr void swap(indirect& other) noexcept(see below);
friend constexpr void swap(indirect& lhs, indirect& rhs) noexcept(see below);

// [indirect.relops], relational operators
template<class U, class AA>
    friend constexpr bool operator==(const indirect& lhs, const indirect<U, AA>& rhs)
    noexcept(see below);

template<class U, class AA>
    friend constexpr auto operator<=>(const indirect& lhs, const indirect<U, AA>& rhs)
    -> synth-three-way-result<T, U>;

// [indirect.comp.with.t], comparison with T
template<class U>
    friend constexpr bool operator==(const indirect& lhs, const U& rhs) noexcept(see below);

template<class U>
    friend constexpr auto operator<=>(const indirect& lhs, const U& rhs)
    -> synth-three-way-result<T, U>;
};

template<class Value>
indirect(Value) -> indirect<Value>;

template<class Allocator, class Value>
indirect(allocator_arg_t, Allocator, Value)
    -> indirect<Value, typename allocator_traits<Allocator>::template rebind_alloc<Value>>;
*/

/// An `indirect` object manages the lifetime of an owned object.
template <class T, class Allocator = std::allocator<T>>
class indirect
{
    // 20.5.1.1.5
    static_assert(std::is_object_v<T>, "T must be an object type!");
    static_assert(!std::is_array_v<T>, "T cannot be an array!");
    static_assert(!std::is_same_v<T, std::in_place_t>, "T cannot be of type std::in_place_t!");
    static_assert(!std::is_const_v<T> && !std::is_volatile_v<T>, "T cannot be a cv-qualified type!");

    using allocator_traits = std::allocator_traits<Allocator>;

    // 20.5.1.1.2
    static_assert(std::is_same_v<T, typename allocator_traits::value_type>);

  public:
    using value_type = T;
    using allocator_type = Allocator;
    using pointer = typename allocator_traits::pointer;
    using const_pointer = typename allocator_traits::const_pointer;

    // static_assert(std::is_default_constructible_v<T>, "T must be default constructible");

    /* 20.5.1.3 [indirect.ctor], constructors */

    // default constructor
    constexpr explicit indirect()
        requires (std::is_default_constructible_v<Allocator> && std::is_default_constructible_v<T>)
      : alloc_()
      , ptr_(make_obj_())
    {}

    // allocator-aware default constructor
    constexpr explicit indirect(std::allocator_arg_t, const Allocator& a)
        requires (std::is_default_constructible_v<T>)
      : alloc_(a)
      , ptr_(make_obj_())
    {}

    // copy constructor
    constexpr indirect(const indirect& other)
        requires (std::is_copy_constructible_v<T>)
      : alloc_(allocator_traits::select_on_container_copy_construction(other.alloc_))
      , ptr_(make_obj_indirect_(other))
    {}

    // allocator-aware copy constructor
    constexpr indirect(std::allocator_arg_t, const Allocator& a, const indirect& other)
        requires (std::is_copy_constructible_v<T>)
      : alloc_(a)
      , ptr_(make_obj_indirect_(other))
    {}

    // move constructor
    constexpr indirect(indirect&& other) noexcept
      : alloc_(std::move(other.alloc_))
      , ptr_(std::exchange(other.ptr_, nullptr))
    {}

    // allocator-aware move constructor
    constexpr indirect(std::allocator_arg_t, const Allocator& a, indirect&& other)
      noexcept (allocator_traits::is_always_equal::value)
      requires (allocator_traits::is_always_equal::value || detail::complete_type<T>)
      : alloc_(a)
      , ptr_(std::exchange(other.ptr_, nullptr))
    {
        if constexpr (! allocator_traits::is_always_equal::value) {
            other.ptr_ = std::exchange(ptr_, nullptr);
            if (! other.valueless_after_move()) {
                ptr_ = make_obj_indirect_(std::move(other));
            }
            indirect tmp = std::move(other);
        }
    }

    template<class U = T>
        requires (
            std::is_constructible_v<T, U> &&
            std::is_default_constructible_v<Allocator> &&
            !std::is_same_v<std::remove_cvref_t<T>, indirect> &&
            !std::is_same_v<std::remove_cvref_t<T>, std::in_place_t>
        )
    constexpr explicit indirect(U&& value)
      : alloc_()
      , ptr_(make_obj_(std::forward<U>(value)))
    {}

    template<class U = T>
        requires (
            std::is_constructible_v<T, U> &&
            std::is_default_constructible_v<Allocator> &&
            !std::is_same_v<std::remove_cvref_t<T>, indirect> &&
            !std::is_same_v<std::remove_cvref_t<T>, std::in_place_t>
        )
    constexpr explicit indirect(std::allocator_arg_t, const Allocator& a, U&& value)
      : alloc_(a)
      , ptr_(make_obj_(std::forward<U>(value)))
    {}

    template<class... Args>
        requires (
            std::is_constructible_v<T, Args...> &&
            std::is_default_constructible_v<Allocator>
        )
    constexpr explicit indirect(std::in_place_t, Args&&... args)
      : ptr_(make_obj_(std::forward<Args>(args)...))
    {}

    template<class... Args>
        requires (std::is_constructible_v<T, Args...>)
    constexpr explicit indirect(std::allocator_arg_t, const Allocator& a, std::in_place_t, Args&&... args)
      : alloc_(a)
      , ptr_(make_obj_(std::forward<Args>(args)...))
    {}

    template<class I, class... Args>
        requires (
            std::is_constructible_v<T, std::initializer_list<I>&, Args...> &&
            std::is_default_constructible_v<Allocator>
        )
    constexpr explicit indirect(std::in_place_t, std::initializer_list<I> ilist, Args&&... args)
      : alloc_()
      , ptr_(make_obj_(ilist, std::forward<Args>(args)...))
    {}

    template<class I, class... Args>
        requires (std::is_constructible_v<T, std::initializer_list<I>&, Args...>)
    constexpr explicit indirect(std::allocator_arg_t, const Allocator& a, std::in_place_t, std::initializer_list<I> ilist, Args&&... args)
      : alloc_(a)
      , ptr_(make_obj_(ilist, std::forward<Args>(args)...))
    {}

    /* 20.5.1.4 [indirect.dtor], destructor */

    constexpr ~indirect()
        requires (detail::complete_type<T>)
    {
        if (!valueless_after_move()) {
            allocator_traits::destroy(alloc_, ptr_);
            allocator_traits::deallocate(alloc_, ptr_, sizeof(T));
        }
    }

    /* 20.5.1.5 [indirect.assign], assignment */

    // copy assignment
    constexpr indirect& operator=(const indirect& other)
        requires(
            std::is_copy_assignable_v<T> &&
            std::is_copy_constructible_v<T>
        )
    {
        if (std::addressof(other) == this) {
            return;
        }

        if (other.valueless_after_move()) {
            this->~indirect();
            ptr_ = nullptr;
            return;
        }

        if (alloc_ == other.alloc_ && !valueless_after_move()) {
            *ptr_ = *other.ptr_;
            return;
        }

        if constexpr (allocator_traits::propagate_on_container_copy_assignment::value) {
            this->~indirect();
            alloc_ = other.alloc_;
            allocator_traits::allocate(alloc_, ptr_, sizeof(T));
        }

        allocator_traits::construct(alloc_, ptr_, *other.ptr_);
        return *this;
    }

    // move assignment
    constexpr indirect& operator=(indirect&& other)
        noexcept(allocator_traits::propagate_on_container_move_assignment::value || allocator_traits::is_always_equal::value)
        requires (
            std::conditional_t<
                !allocator_traits::propagate_on_container_move_assignment::value && !allocator_traits::is_always_equal::value,
                std::is_move_constructible<T>,
                std::true_type
            >::value
        )
    {
        if (std::addressof(other) == this) {
            return;
        }

        if (other.valueless_after_move()) {
            this->~indirect();
            ptr_ = nullptr;
            return;
        }

        if (alloc_ == other.alloc_) {
            ptr_ = std::exchange(other.ptr_, nullptr);
            return;
        }

        if constexpr (allocator_traits::propagate_on_container_move_assignment::value) {
            this->~indirect();
            alloc_ = other.alloc_;
            allocator_traits::allocate(alloc_, ptr_, sizeof(T));
        }

        allocator_traits::construct(alloc_, ptr_, std::move(*other.ptr_));
        return *this;
    }

    template<class U = T>
        requires (
            !std::is_same_v<std::remove_cvref_t<U>, indirect> &&
            std::is_constructible_v<T, U> &&
            std::is_assignable_v<T&, U>
        )
    constexpr indirect& operator=(U&& value)
    {
        if (valueless_after_move()) {
            ptr_ = make_obj_(std::forward<U>(value));
        } else {
            *ptr_ = std::forward<U>(value);
        }

        return *this;
    }

    /* 20.5.1.6 [indirect.obs], observers */

    constexpr const_pointer operator->() const noexcept
    {
        return ptr_;
    }

    constexpr pointer operator->() noexcept
    {
        return ptr_;
    }

    constexpr const T& operator*() const & noexcept
    {
        return *ptr_;
    }

    constexpr T& operator*() & noexcept
    {
        return *ptr_;
    }

    constexpr const T&& operator*() const && noexcept
    {
        return std::move(*ptr_);
    }

    constexpr T&& operator*() && noexcept
    {
        return std::move(*ptr_);
    }

    constexpr bool valueless_after_move() const noexcept
    {
        return ptr_ == nullptr;
    }

    constexpr allocator_type get_allocator() const noexcept
    {
        return alloc_;
    }

    /* 20.5.1.7 [indirect.swap], swap */

    constexpr void swap(indirect& other)
        noexcept(allocator_traits::propagate_on_container_swap::value || allocator_traits::is_always_equal::value)
    {
        using std::swap;

        if constexpr (allocator_traits::is_always_equal::value) {
            swap(ptr_, other.ptr_);
            return;
        } else {
            if constexpr (allocator_traits::propagate_on_container_swap::value) {
                swap(alloc_, other.alloc_);
            }

            if (alloc_ == other.alloc_) {
                swap(ptr_, other.ptr_);
                return;
            } else {
                auto tmp = std::move(*ptr_);
                *ptr_ = std::move(*other.ptr_);
                *other.ptr_ = std::move(tmp);
            }
        }
    }

    friend constexpr void swap(indirect& lhs, indirect& rhs)
        noexcept(allocator_traits::propagate_on_container_swap::value || allocator_traits::is_always_equal::value)
    {
        lhs.swap(rhs);
    }

    /* 20.5.1.8 [indirect.relops], relational operators */

    template<class U, class A>
    friend constexpr bool operator==(const indirect& lhs, const indirect<U, A>& rhs)
        noexcept(noexcept(*lhs == *rhs))
    {
        if (lhs.valueless_after_move()) {
            return rhs.valueless_after_move();
        }
        
        return !rhs.valueless_after_move() || *lhs == *rhs;
    }

    template<class U, class A>
    friend constexpr auto operator<=>(const indirect& lhs, const indirect<U, A>& rhs)
    {
        if (lhs.valueless_after_move() || rhs.valueless_after_move()) {
            return !lhs.valueless_after_move() <=> !rhs.valueless_after_move();
        }

        return *lhs <=> *rhs;
    }

    /* 20.5.1.9 [indirect.comp.with.t], comparison with T */

    template<class U>
    friend constexpr bool operator==(const indirect& ind, const U& value)
        noexcept(noexcept(*ind == value))
    {
        return ind.valueless_after_move() || *ind == value;
    }

    template<class U>
    friend constexpr auto operator<=>(const indirect& ind, const U& value)
    {
        if (ind.valueless_after_move()) {
            return std::strong_ordering::less;
        } else {
            return *ind <=> value;
        }
    }

  private:
    template<typename... Args>
    constexpr pointer make_obj_(Args&&... args) const
    {
        // 20.5.1.1.3
        std::unique_ptr<T> newptr = allocator_traits::allocate(alloc_, sizeof(T));
        allocator_traits::construct(alloc_, newptr.get(), std::forward<Args>(args)...);
        return newptr.release();
    }

    constexpr pointer make_obj_indirect_(const indirect& other) const
    {
        if (other.valueless_after_move()) {
            return nullptr;
        }

        return make_obj_(*other);
    }

    constexpr pointer make_obj_indirect_(indirect&& other) const
    {
        if (other.valueless_after_move()) {
            return nullptr;
        }

        return make_obj_(std::move(*other));
    }

    pointer ptr_;
    [[no_unique_address]] Allocator alloc_{};
};

template<class Value>
indirect(Value) -> indirect<Value>;

template<class Allocator, class Value>
indirect(std::allocator_arg_t, Allocator, Value)
  -> indirect<Value, typename std::allocator_traits<Allocator>::template rebind_alloc<Value>>;

namespace pmr {
    template<class T>
    using indirect = jsm::indirect<T, std::pmr::polymorphic_allocator<T>>;
} // namespace pmr

} // namespace jsm

/* 20.5.1.10 [indirect.hash], hash support */
template<class T, class Allocator>
struct std::hash<jsm::indirect<T, Allocator>> : private std::hash<T>
{
    static constexpr std::size_t valueless = static_cast<std::size_t>(-1);

    template<typename... Args>
    constexpr hash(Args... args)
        noexcept(noexcept(std::hash<T>{std::forward<Args>(args)...}))
      : std::hash<T>{std::forward<Args>(args)...}
    {}

    constexpr std::size_t operator()(const jsm::indirect<T, Allocator>& value)
        noexcept(noexcept(std::hash<T>::operator()(*value)))
    {
        if (value.valueless_after_move()) {
            return valueless;
        }
        
        return std::hash<T>::operator()(*value);
    }
};