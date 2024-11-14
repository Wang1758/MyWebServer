# 常见的c++异常及处理方式
### 程序抛出
1. std::exception
所有标准异常的基类，通用的异常处理

2. std::runtime_error
运行时检测到的问题

1. std::bad_cast
类型转换失败时抛出，当使用dynamic_cast进行不合法的转换

2. std::bad_typeid
无效的类型标识符，当在空指针上使用typeid操作符时抛出

3. std::out_of_range
访问容器元素超出范围时，即使用容器的at方法访问超出范围

4. std::bad_alloc
动态内存分配失败时抛出，如果使用new语句创建动态内存，需要指定new抛出异常

5. std::domain_error
数学函数接受到不合法的输入，如sqrt(-1);

6. std::invalid_argument
函数接受到不合法的参数，如stoi("abc")

### 手动抛出
1. std::invalid_argument
传递给函数的参数不符合要求

2. std::out_of_range
访问的元素超出范围，即常见的数组越界问题

3. std::logic_error
程序逻辑中的错误，需要检查和修改代码中的逻辑

5. std::overflow_error
算数运算导致溢出

### 自定义异常
```
class MyCustomException : public std::exception {
public:
    const char* what() const noexcept override {
        return "My custom exception occurred.";
    }
};

void doSomething() {
    throw MyCustomException();
}
```