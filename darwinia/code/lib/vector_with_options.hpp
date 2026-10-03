#pragma once

#include <vector>
#include <memory>

#include <ranges>

template<typename T>
class VectorWithOptionals
{
public:
	using Optional      = std::unique_ptr<T>;
	using ConstOptional = const std::unique_ptr<const T>;

	void clear() noexcept
	{
		m_values.clear();
	}

private:
	std::vector<Optional> m_values;

public:
	using OptionalsView      = std::ranges::ref_view<decltype(m_values)>;
	using ConstOptionalsView = std::ranges::ref_view<const decltype(m_values)>;

	OptionalsView Optionals()
	{
		return std::views::all(m_values);
	}

	ConstOptionalsView Optionals() const
	{
		return std::views::all(m_values);
	}

	using EnumerateOptionalsView      = std::ranges::enumerate_view<std::ranges::ref_view<decltype(m_values)>>;
	using EnumerateConstOptionalsView = std::ranges::enumerate_view<std::ranges::ref_view<const decltype(m_values)>>;

	EnumerateOptionalsView EnumerateOptionals()
	{
		return std::views::enumerate(m_values);
	}

	EnumerateConstOptionalsView EnumerateOptionals() const
	{
		return std::views::enumerate(m_values);
	}

private:

	struct IsNotNull
	{
		bool operator()(const std::tuple<long, const Optional&>& index_value) const
		{
			auto& [index, ptr] = index_value;
			return ptr != nullptr;
		}
	};

	struct OptionalToRef
	{
		std::tuple<long, T&> operator()(const std::tuple<long, const Optional&>& index_value) const
		{
			auto& [index, ptr] = index_value;
			return std::tuple<long, T&>{index, *ptr};
		}
	};

	struct OptionalToConstRef
	{
		std::tuple<long, const T&> operator()(const std::tuple<long, const Optional&>& index_value) const
		{
			auto& [index, ptr] = index_value;
			return std::tuple<long, T&>{index, *ptr};
		}
	};

	using EnumerateFilterValuesView      = std::ranges::filter_view<EnumerateOptionalsView, IsNotNull>;
	using EnumerateFilterConstValuesView = std::ranges::filter_view<EnumerateConstOptionalsView, IsNotNull>;

public:
	using EnumerateValuesView            = std::ranges::transform_view<EnumerateFilterValuesView, OptionalToRef>;
	using EnumerateConstValuesView       = std::ranges::transform_view<EnumerateFilterConstValuesView, OptionalToConstRef>;

	EnumerateValuesView EnumerateValues()
	{
		return EnumerateOptionals() | std::views::filter(IsNotNull{}) | std::views::transform(OptionalToRef{});
	}

	EnumerateConstValuesView EnumerateValues() const
	{
		return EnumerateOptionals() | std::views::filter(IsNotNull{}) | std::views::transform(OptionalToConstRef{});
	}

	Optional& AddOptional(Optional&& opt)
	{
		return m_values.emplace_back(std::move(opt));
	}

	std::tuple<std::size_t, Optional&> AddOrReplaceFirstNull(Optional&& opt)
	{
		auto itr = std::find_if(m_values.begin(), m_values.end(), [] (Optional& opt) {return !opt;} );

		std::size_t index;

		if(itr != m_values.end())
		{
			*itr = std::move(opt);

			index = itr - m_values.begin();
		}
		else
		{
			m_values.emplace_back(std::move(opt));

			itr = std::prev(m_values.end());

			index = m_values.size() - 1;
		}

		return {index, *itr};
	}
};