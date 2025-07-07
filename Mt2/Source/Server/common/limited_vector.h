#pragma once

#include <vector>

template <typename T>
class LimitedVector
{
public:
	LimitedVector(size_t limit_size) :
		_current_index(0),
		_current_count(0)
	{
		_data.resize(limit_size);
	}

	size_t get_limit_size() const { return _data.size(); }
	size_t get_count() const { return _current_count; }

	T& add()
	{
		if (_current_count == get_limit_size())
		{
			if (++_current_index == get_limit_size())
				_current_index = 0;
		}
		else
		{
			_current_count++;
		}

		return (*this)[_current_count - 1];
	}

	T& operator[](size_t index)
	{
		return const_cast<T&>(__get(index));
	}

	const T& operator[](size_t index) const
	{
		return __get(index);
	}

private:
	const T& __get(size_t index) const
	{
		if (index >= get_limit_size())
			return nullptr;

		index += _current_index;
		if (index >= get_limit_size())
			index -= get_limit_size();

		return _data[index];
	}

private:
	std::vector<T> _data;

	size_t _current_index;
	size_t _current_count;
};
