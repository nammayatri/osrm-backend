#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <boost/assert.hpp>
#include <cstdlib>
#include <iostream>
#include <new>
#include <vector>

namespace osrm::util
{

#if 1
template <typename T, size_t MinItemsInBlock = 1024> class PoolAllocator
{
  public:
    using value_type = T;

    PoolAllocator() noexcept = default;

    template <typename U> PoolAllocator(const PoolAllocator<U> &) noexcept {}

    template <typename U> struct rebind
    {
        using other = PoolAllocator<U, MinItemsInBlock>;
    };

    T *allocate(std::size_t n)
    {
        size_t free_list_index = get_next_power_of_two_exponent(n);
        auto &free_list = free_lists_[free_list_index];
        const auto items_in_block = 1u << free_list_index;
        if (free_list.empty())
        {
            // Check if there is space in current block
            if (current_block_left_items_ < items_in_block)
            {
                allocate_block(items_in_block);
            }

            free_list.push_back(current_block_ptr_);
            current_block_left_items_ -= items_in_block;
            current_block_ptr_ += items_in_block;
        }
        auto ptr = free_list.back();
        free_list.pop_back();
        return ptr;
    }

    void deallocate(T *p, std::size_t n) noexcept
    {
        size_t free_list_index = get_next_power_of_two_exponent(n);
        free_lists_[free_list_index].push_back(p);
    }

    ~PoolAllocator()
    {
        for (auto block : blocks_)
        {
            std::free(block);
        }
    }

    PoolAllocator(const PoolAllocator &) = delete;
    PoolAllocator &operator=(const PoolAllocator &) = delete;

    PoolAllocator(PoolAllocator &&other) noexcept
        : free_lists_(std::move(other.free_lists_)),
          blocks_(std::move(other.blocks_)),
          current_block_ptr_(other.current_block_ptr_),
          current_block_left_items_(other.current_block_left_items_),
          total_allocated_(other.total_allocated_)
    {
        other.current_block_ptr_ = nullptr;
        other.current_block_left_items_ = 0;
        other.total_allocated_ = 0;
    }

    PoolAllocator &operator=(PoolAllocator &&other) noexcept
    {
        if (this != &other)
        {
            for (auto block : blocks_)
            {
                std::free(block);
            }

            free_lists_ = std::move(other.free_lists_);
            blocks_ = std::move(other.blocks_);
            current_block_ptr_ = other.current_block_ptr_;
            current_block_left_items_ = other.current_block_left_items_;
            total_allocated_ = other.total_allocated_;

            other.current_block_ptr_ = nullptr;
            other.current_block_left_items_ = 0;
            other.total_allocated_ = 0;
        }
        return *this;
    }

private:
    size_t get_next_power_of_two_exponent(size_t n) const
    {
        BOOST_ASSERT(n > 0);
        return (sizeof(size_t) * 8) - std::countl_zero(n - 1);
    }

    void allocate_block(size_t items_in_block)
    {
        items_in_block = std::max(items_in_block, MinItemsInBlock);
       
        size_t block_size = items_in_block * sizeof(T);
        T *block = static_cast<T *>(std::malloc(block_size));
        if (!block)
        {
            throw std::bad_alloc();
        }
        total_allocated_ += block_size;
        blocks_.push_back(block);
        current_block_ptr_ = block;
        current_block_left_items_ = items_in_block;
    }

    std::array<std::vector<T *>, 32> free_lists_;
    std::vector<T *> blocks_;
    T *current_block_ptr_ = nullptr;
    size_t current_block_left_items_ = 0;

    size_t total_allocated_ = 0;
};

template <typename T, typename U>
bool operator==(const PoolAllocator<T> &, const PoolAllocator<U> &)
{
    return true;
}

template <typename T, typename U>
bool operator!=(const PoolAllocator<T> &, const PoolAllocator<U> &)
{
    return false;
}

#else
template <typename T> using PoolAllocator = std::allocator<T>;
#endif
} // namespace osrm::util
