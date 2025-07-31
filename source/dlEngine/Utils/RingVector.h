#pragma once
#include "Macros.h"

#include <vector>

namespace Daedalus
{
	template <typename T, size_t Capacity = 0>
	class DAEDALUS_API RingVector
	{
	public:
		template <size_t C = Capacity, std::enable_if_t<C == 0, int> = 0>
		explicit RingVector(size_t capacity)
			: data_(capacity), capacity_(capacity)
		{
		}

		template <size_t C = Capacity, std::enable_if_t<C != 0, int> = 0>
		explicit RingVector()
			: data_(C), capacity_(C)
		{
		}

		void push_back(const T& value)
		{
			data_[(start_ + size_) % capacity_] = value;
			if (size_ < capacity_)
			{
				++size_;
			}
			else
			{
				start_ = (start_ + 1) % capacity_;
			}
		}

		size_t size() const { return size_; }
		size_t capacity() const { return capacity_; }
		std::vector<T> linearize() const
		{
			std::vector<T> result;
			result.reserve(size_);
			for (size_t i = 0; i < size_; ++i)
				result.emplace_back((*this)[i]);
			return result;
		}

		void resize(size_t new_capacity, const T& value = {})
		{
			data_.assign(new_capacity, value);
			capacity_ = new_capacity;
			start_ = 0;
			size_ = new_capacity;
		}

		bool empty() const
		{
			return size_ == 0;
		}

		void clear()
		{
			data_.clear();
			data_.resize(capacity_);
			start_ = 0;
			size_ = 0;
		}

		const T& back() const
		{
			if (empty())
				throw std::out_of_range("RingVector::back() called on empty buffer");
			return data_[(start_ + size_ - 1) % capacity_];
		}

		T& back()
		{
			if (empty())
				throw std::out_of_range("RingVector::back() called on empty buffer");
			return data_[(start_ + size_ - 1) % capacity_];
		}

		const T& operator[](size_t index) const
		{
			return data_[(start_ + index) % capacity_];
		}

		T& operator[](size_t index)
		{
			return data_[(start_ + index) % capacity_];
		}

		class Iterator
		{
		public:
			using iterator_category = std::random_access_iterator_tag;
			using value_type = T;
			using difference_type = std::ptrdiff_t;
			using pointer = T*;
			using reference = T&;

			Iterator(RingVector* buffer, size_t pos)
				: buffer_(buffer), pos_(pos) {
			}

			reference operator*() { return (*buffer_)[pos_]; }
			pointer operator->() { return &(*buffer_)[pos_]; }

			Iterator& operator++() { ++pos_; return *this; }
			Iterator operator++(int) { Iterator tmp = *this; ++(*this); return tmp; }

			Iterator& operator--() { --pos_; return *this; }
			Iterator operator--(int) { Iterator tmp = *this; --(*this); return tmp; }

			Iterator& operator+=(difference_type n) { pos_ += n; return *this; }
			Iterator& operator-=(difference_type n) { pos_ -= n; return *this; }

			Iterator operator+(difference_type n) const { return Iterator(buffer_, pos_ + n); }
			Iterator operator-(difference_type n) const { return Iterator(buffer_, pos_ - n); }

			difference_type operator-(const Iterator& other) const { return static_cast<difference_type>(pos_) - static_cast<difference_type>(other.pos_); }

			reference operator[](difference_type n) { return (*buffer_)[pos_ + n]; }

			bool operator==(const Iterator& other) const { return pos_ == other.pos_ && buffer_ == other.buffer_; }
			bool operator!=(const Iterator& other) const { return !(*this == other); }

			bool operator<(const Iterator& other) const { return pos_ < other.pos_; }
			bool operator>(const Iterator& other) const { return pos_ > other.pos_; }
			bool operator<=(const Iterator& other) const { return pos_ <= other.pos_; }
			bool operator>=(const Iterator& other) const { return pos_ >= other.pos_; }

		private:
			RingVector* buffer_;
			size_t pos_;
		};

		class ConstIterator
		{
		public:
			using iterator_category = std::random_access_iterator_tag;
			using value_type = T;
			using difference_type = std::ptrdiff_t;
			using pointer = const T*;
			using reference = const T&;

			ConstIterator(const RingVector* buffer, size_t pos)
				: buffer_(buffer), pos_(pos) {
			}

			reference operator*() const { return (*buffer_)[pos_]; }
			pointer operator->() const { return &(*buffer_)[pos_]; }

			ConstIterator& operator++() { ++pos_; return *this; }
			ConstIterator operator++(int) { ConstIterator tmp = *this; ++(*this); return tmp; }

			ConstIterator& operator--() { --pos_; return *this; }
			ConstIterator operator--(int) { ConstIterator tmp = *this; --(*this); return tmp; }

			ConstIterator& operator+=(difference_type n) { pos_ += n; return *this; }
			ConstIterator& operator-=(difference_type n) { pos_ -= n; return *this; }

			ConstIterator operator+(difference_type n) const { return ConstIterator(buffer_, pos_ + n); }
			ConstIterator operator-(difference_type n) const { return ConstIterator(buffer_, pos_ - n); }

			difference_type operator-(const ConstIterator& other) const { return static_cast<difference_type>(pos_) - static_cast<difference_type>(other.pos_); }

			reference operator[](difference_type n) const { return (*buffer_)[pos_ + n]; }

			bool operator==(const ConstIterator& other) const { return pos_ == other.pos_ && buffer_ == other.buffer_; }
			bool operator!=(const ConstIterator& other) const { return !(*this == other); }

			bool operator<(const ConstIterator& other) const { return pos_ < other.pos_; }
			bool operator>(const ConstIterator& other) const { return pos_ > other.pos_; }
			bool operator<=(const ConstIterator& other) const { return pos_ <= other.pos_; }
			bool operator>=(const ConstIterator& other) const { return pos_ >= other.pos_; }

		private:
			const RingVector* buffer_;
			size_t pos_;
		};

		Iterator begin() { return Iterator(this, 0); }
		Iterator end() { return Iterator(this, size_); }

		ConstIterator begin() const { return ConstIterator(this, 0); }
		ConstIterator end() const { return ConstIterator(this, size_); }

		using reverse_iterator = std::reverse_iterator<Iterator>;
		using const_reverse_iterator = std::reverse_iterator<ConstIterator>;

		reverse_iterator rbegin() { return reverse_iterator(end()); }
		reverse_iterator rend() { return reverse_iterator(begin()); }

		const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
		const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

		const_reverse_iterator crbegin() const { return const_reverse_iterator(end()); }
		const_reverse_iterator crend() const { return const_reverse_iterator(begin()); }

	private:
		std::vector<T> data_;
		size_t capacity_ = 0;
		size_t start_ = 0;
		size_t size_ = 0;
	};
}