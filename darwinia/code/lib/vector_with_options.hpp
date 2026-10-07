#pragma once

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include <ranges>

template<typename Optional>
struct OptionalTraits;

template<typename T>
struct OptionalTraits<std::optional<T>>
{
	using Type = T;

	static std::optional<T> make(auto&& ... args)
	{
		return std::make_optional<T>(std::forward<decltype(args)>(args)...);
	}
};

template<typename T>
struct OptionalTraits<std::unique_ptr<T>>
{
	using Type = T;

	static std::unique_ptr<T> make(auto&& ... args)
	{
		return std::make_unique<T>(std::forward<decltype(args)>(args)...);
	}
};

template<typename Optional>
class VectorWithOptionals
{
public:
	using ValueType = typename OptionalTraits<Optional>::Type;

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
		bool operator()(const Optional& opt) const
		{
			return bool(opt);
		}
	};

	struct IndexedIsNotNull
	{
		bool operator()(const std::tuple<long, const Optional&>& index_value) const
		{
			return bool(std::get<1>(index_value));
		}
	};

	struct OptionalToRef
	{
		ValueType& operator()(Optional& opt) const
		{
			return *opt;
		}
	};

	struct OptionalToConstRef
	{
		const ValueType& operator()(const Optional& opt) const
		{
			return *opt;
		}
	};

	struct IndexedOptionalToRef
	{
		std::tuple<long, ValueType&> operator()(const std::tuple<long, const Optional&>& index_value) const
		{
			auto& [index, ptr] = index_value;
			return std::tuple<long, ValueType&>{index, *ptr};
		}
	};

	struct IndexedOptionalToConstRef
	{
		std::tuple<long, const ValueType&> operator()(const std::tuple<long, const Optional&>& index_value) const
		{
			auto& [index, ptr] = index_value;
			return std::tuple<long, ValueType&>{index, *ptr};
		}
	};

	using FilterValuesView      = std::ranges::filter_view<OptionalsView, IsNotNull>;
	using FilterConstValuesView = std::ranges::filter_view<ConstOptionalsView, IsNotNull>;

	using EnumerateFilterValuesView      = std::ranges::filter_view<EnumerateOptionalsView, IndexedIsNotNull>;
	using EnumerateFilterConstValuesView = std::ranges::filter_view<EnumerateConstOptionalsView, IndexedIsNotNull>;

public:
	using ValuesView            = std::ranges::transform_view<FilterValuesView, OptionalToRef>;
	using ConstValuesView       = std::ranges::transform_view<FilterConstValuesView, OptionalToConstRef>;

	ValuesView Values()
	{
		return Optionals() | std::views::filter(IsNotNull{}) | std::views::transform(OptionalToRef{});
	}

	ConstValuesView Values() const
	{
		return Optionals() | std::views::filter(IsNotNull{}) | std::views::transform(OptionalToConstRef{});
	}

	using EnumerateValuesView            = std::ranges::transform_view<EnumerateFilterValuesView, IndexedOptionalToRef>;
	using EnumerateConstValuesView       = std::ranges::transform_view<EnumerateFilterConstValuesView, IndexedOptionalToConstRef>;

	EnumerateValuesView EnumerateValues()
	{
		return EnumerateOptionals() | std::views::filter(IndexedIsNotNull{}) | std::views::transform(IndexedOptionalToRef{});
	}

	EnumerateConstValuesView EnumerateValues() const
	{
		return EnumerateOptionals() | std::views::filter(IndexedIsNotNull{}) | std::views::transform(IndexedOptionalToConstRef{});
	}

	Optional& AddOptional(Optional&& opt)
	{
		return m_values.emplace_back(std::move(opt));
	}

	std::tuple<std::size_t, Optional&> MoveOrReplaceFirstNull(Optional&& opt)
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

	std::tuple<std::size_t, Optional&> MakeOrReplaceFirstNull(auto&& ... args)
	{
		return this->MoveOrReplaceFirstNull(OptionalTraits<Optional>::make(std::forward<decltype(args)>(args) ...));
	}
};