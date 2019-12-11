@title[Code Presenting Templates]

## @color[black](Code Presenting<br>Slide Templates)

@fa[arrow-down text-black]

@snap[south docslink span-50]
[The Template Docs](https://gitpitch.com/docs/the-template)
@snapend


+++?code=template/src/go/server.go&lang=golang
@title[Repo Source File]

@[1,3-6](Present code found within any repository source file.)
@[8-18](Without ever leaving your slideshow.)
@[19-28](Using GitPitch code-presenting with (optional) annotations.)

@snap[north-east template-note text-gray]
Code presenting repository source file template.
@snapend


+++?color=lavender
@title[The old API for creating Variables]

```cpp
template <class T> 
Variable makeVariable(const Dimensions &dimensions);

template <class T> 
Variable makeVariable(const Dimensions &dimensions, const detail::default_init_elements_t &init);

template <class T> 
Variable makeVariable(const std::initializer_list<Dim> &dims, const std::initializer_list<scipp::index> &shape)

template <class T> 
Variable makeVariable(T value);

template <class T>
Variable makeVariable(T value, T variance);

template <class T, class T2 = T> 
Variable makeVariable(const Dimensions &dimensions, std::initializer_list<T2> values, std::initializer_list<T2> variances = {});

template <class T, class T2 = T> 
Variable makeVariable(const Dimensions &dimensions, const units::Unit unit, std::initializer_list<T2> values, std::initializer_list<T2> variances = {});

template <class T> 
Variable makeVariableWithVariances(const Dimensions &dimensions, units::Unit unit = units::dimensionless);

template <class T> 
Variable makeVariableWithVariances(const Dimensions &dimensions, const detail::default_init_elements_t &init);
```

+++?color=lavender
@title[My favorite function]
```cpp
template <class T, class... Args> 
Variable makeVariable(const Dimensions &dimensions, Args &&... args) {
// Note: Using `if constexpr` instead of another overload, since overloading
  // on universal reference arguments is problematic.
  if constexpr (detail::is_vector<std::remove_cv_t<
                    std::remove_reference_t<Args>>...>::value) {
    // Copies to aligned memory.
    return Variable(units::dimensionless, std::move(dimensions),
                    Vector<T>(args.begin(), args.end())...);
  } else if constexpr (sizeof...(Args) == 1 &&
                       (std::is_convertible_v<Args, units::Unit> && ...)) {
    return Variable(
        args..., std::move(dimensions),
        Vector<T>(dimensions.volume(), detail::default_init<T>::value()));
  } else if constexpr (sizeof...(Args) == 2) {
    if constexpr (std::is_convertible_v<detail::nth_t<0, Args...>,
                                        std::vector<T>> &&
                  std::is_convertible_v<detail::nth_t<1, Args...>,
                                        std::vector<T>>) {
      return Variable(units::dimensionless, std::move(dimensions),
                      Vector<T>(detail::nth<0>(args...).begin(),
                                detail::nth<0>(args...).end()),
                      Vector<T>(detail::nth<1>(args...).begin(),
                                detail::nth<1>(args...).end()));
    } else {
      return Variable(units::dimensionless, std::move(dimensions),
                      Vector<T>(std::forward<Args>(args)...));
    }
  } else if constexpr (sizeof...(Args) == 3) {
    if constexpr (std::is_convertible_v<detail::nth_t<0, Args...>,
                                        units::Unit> &&
                  std::is_convertible_v<detail::nth_t<1, Args...>,
                                        std::vector<T>> &&
                  std::is_convertible_v<detail::nth_t<2, Args...>,
                                        std::vector<T>>) {
      return Variable(detail::nth<0>(args...), std::move(dimensions),
                      Vector<T>(detail::nth<1>(args...).begin(),
                                detail::nth<1>(args...).end()),
                      Vector<T>(detail::nth<2>(args...).begin(),
                                detail::nth<2>(args...).end()));
    } else {
      return Variable(units::dimensionless, std::move(dimensions),
                      Vector<T>(std::forward<Args>(args)...));
    }
  } else {
    return Variable(units::dimensionless, std::move(dimensions),
                    Vector<T>(std::forward<Args>(args)...));
  }
}
```
