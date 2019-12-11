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

@snap[south span-100]
```cpp
template <class T> 
Variable makeVariable(const Dimensions &dimensions);
template <class T> 
Variable makeVariable(const Dimensions &dimensions, 
                      const detail::default_init_elements_t &init);
template <class T> 
Variable makeVariable(const std::initializer_list<Dim> &dims, 
                      const std::initializer_list<scipp::index> &shape);
template <class T> 
Variable makeVariable(T value);
template <class T>
Variable makeVariable(T value, T variance);
```
@snapend

+++?color=lavender
@title[More cases]

```cpp
template <class T, class T2 = T> 
Variable makeVariable(const Dimensions &dimensions, 
                      std::initializer_list<T2> values, 
                      std::initializer_list<T2> variances = {});
template <class T, class T2 = T> 
Variable makeVariable(const Dimensions &dimensions, 
                      const units::Unit unit,
                      std::initializer_list<T2> values, 
                      std::initializer_list<T2> variances = {});
                      
```
                      
+++?color=lavender
@title[More sophisticated]

```cpp
template <class T> 
Variable makeVariableWithVariances(const Dimensions &dimensions, 
                                   units::Unit unit = units::dimensionless);
template <class T> 
Variable makeVariableWithVariances(const Dimensions &dimensions, 
                                   const detail::default_init_elements_t &init);
template <class T, class... Args> 
Variable makeVariable(const Dimensions &dimensions, Args &&... args);
```

