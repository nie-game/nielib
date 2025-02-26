#ifndef EYEBREACH_ENUMERATE_HPP
#define EYEBREACH_ENUMERATE_HPP

// #include <ranges>
#include <vector>
#include <version>

#if !(__cpp_lib_ranges_enumerate > 202106)
namespace std::ranges {
  namespace r = std::ranges;
  namespace detail {
    template <typename T>
    concept has_iterator_category = requires { typename std::iterator_traits<r::iterator_t<T>>::iterator_category; };

    template <typename T>
    concept has_iterator_concept = requires { typename std::iterator_traits<r::iterator_t<T>>::iterator_concept; };

    template <typename... V> consteval auto iter_cat() {
      if constexpr ((r::random_access_range<V> && ...))
        return std::random_access_iterator_tag{};
      else if constexpr ((r::bidirectional_range<V> && ...))
        return std::bidirectional_iterator_tag{};
      else if constexpr ((r::forward_range<V> && ...))
        return std::forward_iterator_tag{};
      else if constexpr ((r::input_range<V> && ...))
        return std::input_iterator_tag{};
      else
        return std::output_iterator_tag{};
    }
  } // namespace detail
  template <class R>
  concept simple_view = // exposition only
      r::view<R> && r::range<const R> && std::same_as<r::iterator_t<R>, r::iterator_t<const R>> &&
      std::same_as<r::sentinel_t<R>, r::sentinel_t<const R>>;

  template <r::input_range V>
    requires r::view<V>
  class enumerate_view : public r::view_interface<enumerate_view<V>> {

    V base_ = {};

    template <bool> struct sentinel;

    template <bool Const> struct iterator {
    private:
      using Base = std::conditional_t<Const, const V, V>;
      using count_type = decltype([] {
        if constexpr (r::sized_range<Base>)
          return r::range_size_t<Base>();
        else {
          return std::make_unsigned_t<r::range_difference_t<Base>>();
        }
      }());

      template <typename T> struct result {
        const count_type index;
        T value;

        constexpr bool operator==(const result& other) const = default;
      };

      r::iterator_t<Base> current_ = r::iterator_t<Base>();
      count_type pos_ = 0;

      template <bool> friend struct iterator;
      template <bool> friend struct sentinel;

    public:
      using iterator_category = decltype(detail::iter_cat<Base>());
      using reference = result<r::range_reference_t<Base>>;
      using value_type = result<r::range_reference_t<Base>>;
      using difference_type = r::range_difference_t<Base>;

      iterator() = default;

      constexpr explicit iterator(r::iterator_t<Base> current, r::range_difference_t<Base> pos) : current_(std::move(current)), pos_(pos) {}
      constexpr iterator(iterator<!Const> i)
        requires Const && std::convertible_to<r::iterator_t<V>, r::iterator_t<Base>>
          : current_(std::move(i.current_)), pos_(i.pos_) {}

      constexpr const r::iterator_t<V>& base() const&
        requires std::copyable<r::iterator_t<Base>>
      {
        return current_;
      }

      constexpr r::iterator_t<V> base() && {
        return std::move(current_);
      }

      constexpr auto operator*() const {
        return reference{static_cast<count_type>(pos_), *current_};
      }

      constexpr iterator& operator++() {
        ++pos_;
        ++current_;
        return *this;
      }

      constexpr auto operator++(int) {
        ++pos_;
        if constexpr (r::forward_range<V>) {
          auto tmp = *this;
          ++*this;
          return tmp;
        } else {
          ++current_;
        }
      }

      constexpr iterator& operator--()
        requires r::bidirectional_range<V>
      {
        --pos_;
        --current_;
        return *this;
      }

      constexpr auto operator--(int)
        requires r::bidirectional_range<V>
      {
        auto tmp = *this;
        --*this;
        return tmp;
      }

      constexpr iterator& operator+=(difference_type n)
        requires r::random_access_range<V>
      {
        current_ += n;
        pos_ += n;
        return *this;
      }

      constexpr iterator& operator-=(difference_type n)
        requires r::random_access_range<V>
      {
        current_ -= n;
        pos_ -= n;
        return *this;
      }

      friend constexpr iterator operator+(const iterator& i, difference_type n)
        requires r::random_access_range<V>
      {
        return iterator{i.current_ + n, static_cast<difference_type>(i.pos_ + n)};
      }

      friend constexpr iterator operator+(difference_type n, const iterator& i)
        requires r::random_access_range<V>
      {
        return iterator{i.current_ + n, static_cast<difference_type>(i.pos_ + n)};
      }

      friend constexpr auto operator-(iterator i, difference_type n)
        requires r::random_access_range<V>
      {
        return iterator{i.current_ - n, static_cast<difference_type>(i.pos_ - n)};
      }

      friend constexpr auto operator-(difference_type n, iterator i)
        requires r::random_access_range<V>
      {
        return iterator{i.current_ - n, static_cast<difference_type>(i.pos_ - n)};
      }

      constexpr decltype(auto) operator[](difference_type n) const
        requires r::random_access_range<Base>
      {
        return reference{static_cast<count_type>(pos_ + n), *(current_ + n)};
      }

      friend constexpr bool operator==(const iterator& x, const iterator& y)
        requires std::equality_comparable<r::iterator_t<Base>>
      {
        return x.current_ == y.current_;
      }

      template <bool ConstS> friend constexpr bool operator==(const iterator<Const>& i, const sentinel<ConstS>& s) {
        return i.current_ == s.base();
      }

      friend constexpr bool operator<(const iterator& x, const iterator& y)
        requires r::random_access_range<Base>
      {
        return x.current_ < y.current_;
      }

      friend constexpr bool operator>(const iterator& x, const iterator& y)
        requires r::random_access_range<Base>
      {
        return x.current_ > y.current_;
      }

      friend constexpr bool operator<=(const iterator& x, const iterator& y)
        requires r::random_access_range<Base>
      {
        return x.current_ <= y.current_;
      }
      friend constexpr bool operator>=(const iterator& x, const iterator& y)
        requires r::random_access_range<Base>
      {
        return x.current_ >= y.current_;
      }
      friend constexpr auto operator<=>(const iterator& x, const iterator& y)
        requires r::random_access_range<Base> && std::three_way_comparable<r::iterator_t<Base>>
      {
        return x.current_ <=> y.current_;
      }

      friend constexpr difference_type operator-(const iterator& x, const iterator& y)
        requires r::random_access_range<Base>
      {
        return x.current_ - y.current_;
      }
    };

    template <bool Const> struct sentinel {
    private:
      friend iterator<false>;
      friend iterator<true>;

      using Base = std::conditional_t<Const, const V, V>;

      r::sentinel_t<V> end_;

    public:
      sentinel() = default;

      constexpr explicit sentinel(r::sentinel_t<V> end) : end_(std::move(end)) {}

      constexpr auto base() const {
        return end_;
      }

      friend constexpr r::range_difference_t<Base> operator-(const iterator<Const>& x, const sentinel& y)
        requires std::sized_sentinel_for<r::sentinel_t<Base>, r::iterator_t<Base>>
      {
        return x.current_ - y.end_;
      }

      friend constexpr r::range_difference_t<Base> operator-(const sentinel& x, const iterator<Const>& y)
        requires std::sized_sentinel_for<r::sentinel_t<Base>, r::iterator_t<Base>>
      {
        return x.end_ - y.current_;
      }
    };

  public:
    constexpr enumerate_view() = default;
    constexpr enumerate_view(V base) : base_(std::move(base)) {}

    constexpr auto begin()
      requires(!simple_view<V>)
    {
      return iterator<false>(std::ranges::begin(base_), 0);
    }

    constexpr auto begin() const
      requires simple_view<V>
    {
      return iterator<true>(std::ranges::begin(base_), 0);
    }

    constexpr auto end() {
      return sentinel<false>{r::end(base_)};
    }

    constexpr auto end()
      requires r::common_range<V>
    {
      return iterator<false>{std::ranges::end(base_), static_cast<r::range_difference_t<V>>(size())};
    }

    constexpr auto end() const
      requires r::range<const V>
    {
      return sentinel<true>{std::ranges::end(base_)};
    }

    constexpr auto end() const
      requires r::common_range<const V>
    {
      return iterator<true>{std::ranges::end(base_), static_cast<r::range_difference_t<V>>(size())};
    }

    constexpr auto size()
      requires r::sized_range<V>
    {
      return std::ranges::size(base_);
    }

    constexpr auto size() const
      requires r::sized_range<const V>
    {
      return std::ranges::size(base_);
    }

    constexpr V base() const&
      requires std::copyable<V>
    {
      return base_;
    }

    constexpr V base() && {
      return std::move(base_);
    }
  };

  template <typename R>
    requires r::input_range<R>
  enumerate_view(R&& r) -> enumerate_view<r::views::all_t<R>>;

  namespace detail {

    struct enumerate_view_fn {
      template <typename R> constexpr auto operator()(R&& r) const {
        return enumerate_view{std::forward<R>(r)};
      }

      template <r::input_range R> constexpr friend auto operator|(R&& rng, const enumerate_view_fn&) {
        return enumerate_view{std::forward<R>(rng)};
      }
    };
  } // namespace detail

  inline detail::enumerate_view_fn enumerate;
  namespace views {
    inline detail::enumerate_view_fn enumerate;
  }
} // namespace std::ranges
#endif

namespace fuckplusplus {
  template <typename Iterable> class enumerate_object {
  private:
    Iterable _iter;
    std::size_t _size;
    decltype(std::begin(_iter)) _begin;
    const decltype(std::end(_iter)) _end;

  public:
    enumerate_object(Iterable iter) : _iter(iter), _size(0), _begin(std::begin(iter)), _end(std::end(iter)) {}

    const enumerate_object& begin() const {
      return *this;
    }
    const enumerate_object& end() const {
      return *this;
    }

    bool operator!=(const enumerate_object&) const {
      return _begin != _end;
    }

    void operator++() {
      ++_begin;
      ++_size;
    }

    auto operator*() const -> std::pair<std::size_t, decltype(*_begin)> {
      return {_size, *_begin};
    }
  };
  template <typename Iterable> auto enumerate(Iterable&& iter) -> enumerate_object<Iterable> {
    return {std::forward<Iterable>(iter)};
  }
} // namespace fuckplusplus

#endif