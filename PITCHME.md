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

#### @color[gray](The old API for creating Variables:)

</br>

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

+++?color=lavender
@title[More cases]

#### @color[gray](More cases:)

</br>

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

#### @color[gray](More sophisticated:)

</br>

```cpp
template <class T> 
Variable makeVariableWithVariances(const Dimensions &dimensions, 
                                   units::Unit unit);
template <class T> 
Variable makeVariableWithVariances(const Dimensions &dimensions, 
                                   const default_init_elements_t &f);
template <class T, class... Args> 
Variable makeVariable(const Dimensions &dimensions, Args &&... args);
```

+++?color=lavender
@title[The pythonic way of doing things]

#### @color[gray](The pythonic way of doing things:)

</br>

```python
# Named arguments in arbitrary order
make_variable(dimensions=[Dim.X, Dim.Y], shape=[1, 2], values=[4, 4])
# or 
make_variable(values=[4, 4], dimensions=[Dim.X, Dim.Y], shape=[1, 2])
```

</br>

```cpp
//Could we do something similar in C++? We would try:
template <class T, class... Ts> Variable makeVariable(Ts &&... ts);
```

</br>

```cpp
//Now we can write:
makeVariable(Dims{Dim::X, Dim::Y}, Shape{1, 2}, Values{4, 4});
```

+++?color=lavender
@title[Refactoring conditions and tools]

##### Refactoring conditions and tools.


| 182 files | 45739 lines of code | 1162 occurance of "makeVariable"|
|-----------|---------------------|---------------------------------|

##### clang-10 tools:
- <https://clang.llvm.org/docs/LibASTMatchersTutorial.html>
- <https://clang.llvm.org/docs/HowToSetupToolingForLLVM.html>
- <https://clang.llvm.org/docs/LibASTMatchers.html>
- <https://clang.llvm.org/docs/LibASTMatchersReference.html>

+++?color=lavender
@title[Match finder callback]

@size[xxx-large](@color[gray](Match finder callback:))

```cpp
class MakeVariableWithDimsCallBack: public MatchFinder::MatchCallback {
public:
  using ReplMap = std::map<std::string, Replacements>;
  MakeVariableWithDimsCallBack(ReplMap *rm) : replMap(rm) {}
  virtual void run(const MatchFinder::MatchResult &Result);
private:
  ReplMap *replMap;
};
``` 

</br>

```cpp
static StatementMatcher MakeVariableMatcher = ...;
int main(int argc, const char **argv) {
  CommonOptionsParser OptionsParser(argc, argv, MyToolCategory);
  RefactoringTool Tool(OptionsParser.getCompilations(), 
                       OptionsParser.getSourcePathList());
  MakeVariableWithDimsCallBack withDimensions(&Tool.getReplacements());
  MatchFinder Finder;
  Finder.addMatcher(MakeVariableMatcher, &withDimensions);
  retrun Tool.runAndSave(newFrontendActionFactory(&Finder).get());
}
```
