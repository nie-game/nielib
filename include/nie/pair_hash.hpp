#ifndef NIE_PAIR_HASH_HPP
#define NIE_PAIR_HASH_HPP

namespace nie {
  /**
   * \brief Hash to allow std::pair to be used as map key
   */
  struct pair_hash {
    static constexpr size_t hash_combiner(size_t left) {
      return left;
    }
    static constexpr size_t hash_combiner(size_t left, size_t right) {
      return left + 0x9e3779b9 + (right << 6) + (right >> 2);
    }
    template <typename... T> static constexpr size_t hash_combiner(size_t left, T... rest) {
      return hash_combiner(left, hash_combiner(rest...));
    }
    /**
     * \brief calculate hash for std::pair
     * \param[in] pair the pair to hash
     * \return the hash
     */
    template <class T1, class T2> std::size_t operator()(const std::pair<T1, T2>& pair) const {
      return hash_combiner(std::hash<T1>()(pair.first), std::hash<T2>()(pair.second));
    }
    template <class... T> std::size_t operator()(const std::tuple<T...>& pair) const {
      return std::apply([](const T&... data) { return hash_combiner(std::hash<T>()(data)...); }, pair);
    }
  };
} // namespace nie

#endif // NIE_PAIR_HASH_HPP
