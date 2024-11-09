## 线程池

#### 1.知识点
* **RALL**
Resource Acquisition Is Initialization（资源获取就是初始化）
RAII的做法是使用一个对象，在其构造时获取对应的资源，在对象生命期内控制对资源的访问，使之始终保持有效，最后在对象析构的时候，释放构造时获取的资源。
<br>
* **右值引用** (&&)： 可以绑定到右值（临时对象）和左值。主要用于实现移动语义或者实现完美转发
<br> 
* **移动语义**：移动语义允许你将资源（如内存、文件句柄等）从一个对象“移动”到另一个对象，而不是复制资源，从而提高效率。
std::move:将传入对象转换为右值引用，使得该对象可以被移动。常用于类资源的移动，会调用相应的移动构造函数或移动赋值运算符。注意，在移动构造函数中只需要将原本的资源设置为空，在移动复制运算符中需要将原本的资源释放
```
class MyClass {
public:
    MyClass(int size) : data(new int[size]), size(size) {
        std::cout << "Constructing MyClass" << std::endl;
    }

    // 移动构造函数
    MyClass(MyClass&& other) noexcept
        : data(other.data), size(other.size) {
        other.data = nullptr;
        other.size = 0;
    }

    // 移动赋值运算符
    MyClass& operator=(MyClass&& other) noexcept {
        if (this != &other) {
            delete[] data;
            data = other.data;
            size = other.size;
            other.data = nullptr;
            other.size = 0;
        }
        return *this;
    }

    ~MyClass() {
        delete[] data;
    }

private:
    int* data;
    int size;
};

int main() {
    MyClass a(10);
    MyClass b = std::move(a); // 调用移动构造函数
    MyClass c(20);
    c = std::move(b); // 调用移动赋值运算符

    return 0;
}
```
* **完美转发**：通过使用 std::forward，你可以将参数按原样传递给另一个函数，无论是左值还是右值，都能保留其属性。
std::forward(task) : 根据task的类型，选择性的使用std::move(如果task是右值引用)，或者直接传递(task是左值引用)
std::emplace() ：直接在容器内构造元素。接受构造函数的参数，并将参数传递给元素的构造函数，避免了额外的复制或移动操作（push操作是先构造一个元素，然后复制到队列中）

#### 2.高效连接池
* **任务线程池**
  在任务线程池中，获得了锁的线程会尝试从任务队列中获取一个任务，使用move方法（资产转移）高效获取任务并执行，之后再次尝试获取锁并重复执行。如果尝试时发现任务队列为空，则会进入等待条件变量状态
  添加任务时，将任务通过完美转发的方式将任务移动到任务队列中，然后通知某一个正在等待条件变量的线程
* **MySql连接池**
  使用单例模式创建一个MySql连接池类。该类有一个连接队列，通过GetConn、FreeConn方法从连接池中获取、放回连接。
  但是操作获取、放回连接需要通过SqlConnRAII类，该类的一个对象在构造时调用GetConn获取一个连接并保存在成员变量中，在该对象析构时调用FreeConn方法将连接放回连接池