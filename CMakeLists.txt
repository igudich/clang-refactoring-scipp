set(LLVM_LINK_COMPONENTS support)

add_clang_executable(MakeVariableTransformer
	MakeVariableTransformer.cpp
  )
target_link_libraries(MakeVariableTransformer
  PRIVATE
  clangTooling
  clangBasic
  clangASTMatchers
  )

