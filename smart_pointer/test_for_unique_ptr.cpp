#include <unique_ptr.h>


// The example above is from cppreference

// It is tests for unique_ptr for single object
struct Fooo {
    Foo(int _val) : val(_val) { std::cout << "Foo...\n"; }
    ~Foo() { std::cout << "~Foo...\n"; }
    int val;
};


struct Foo { // 要管理的对象
    Foo() { std::cout << "Foo ctor\n"; }
    Foo(const Foo&) { std::cout << "Foo copy ctor\n"; }
    Foo(Foo&&) { std::cout << "Foo move ctor\n"; }
    ~Foo() { std::cout << "~Foo dtor\n"; }
};
 
struct D { // 删除器
    D() {};
    D(const D&) { std::cout << "D copy ctor\n"; }
    D(D&) { std::cout << "D non-const copy ctor\n";}
    D(D&&) { std::cout << "D move ctor \n"; }
    void operator()(Foo* p) const {
        std::cout << "D is deleting a Foo\n";
        delete p;
    };
};
 
int main()
{
    // Tests for constructors
    std::cout << "Example constructor(1)...\n";
    sm_ptr::unique_ptr<Foo> up1;  // up1 为空
    sm_ptr::unique_ptr<Foo> up1b(nullptr);  // up1b 为空
 
    std::cout << "Example constructor(2)...\n";
    {
        sm_ptr::unique_ptr<Foo> up2(new Foo); // up2 现在占有 Foo
    } // Foo 被删除
 
    std::cout << "Example constructor(3)...\n";
    D d;
    {  // 删除器类型不是引用
       sm_ptr::unique_ptr<Foo, D> up3(new Foo, d); // 复制删除器
    }
    {  // 删除器类型是引用 
       sm_ptr::unique_ptr<Foo, D&> up3b(new Foo, d); // up3b 保有到 d 的引用
    }
 
    std::cout << "Example constructor(4)...\n";
    {  // 删除器不是引用
       sm_ptr::unique_ptr<Foo, D> up4(new Foo, D()); // 移动删除器
    }
 
    std::cout << "Example constructor(5)...\n";
    {
       sm_ptr::unique_ptr<Foo> up5a(new Foo);
       sm_ptr::unique_ptr<Foo> up5b(std::move(up5a)); // 所有权转移
    }
 
    std::cout << "Example constructor(6)...\n";
    {
        sm_ptr::unique_ptr<Foo, D> up6a(new Foo, d); // 复制 D
        sm_ptr::unique_ptr<Foo, D> up6b(std::move(up6a)); // 移动 D
 
        sm_ptr::unique_ptr<Foo, D&> up6c(new Foo, d); // D 是引用
        sm_ptr::unique_ptr<Foo, D> up6d(std::move(up6c)); // 复制 D
    }


    // Test for destructors
    {
        auto deleter = [](int* ptr){
            std::cout << "[deleter called]\n";
            delete ptr;
        };
    
        std::unique_ptr<int,decltype(deleter)> uniq(new int, deleter);
        std::cout << (uniq ? "not empty\n" : "empty\n");
        uniq.reset();
        std::cout << (uniq ? "not empty\n" : "empty\n");
    }


    // Tests for member function release()
    {
        std::cout << "Creating new Foo...\n";
        std::unique_ptr<Foo> up(new Foo());
    
        std::cout << "About to release Foo...\n";
        Foo* fp = up.release();
    
        assert (up.get() == nullptr);
        std::cout << "Foo is no longer owned by unique_ptr...\n";
    
        delete fp;
    }

    // Tests for reset()
    {
        std::cout << "Creating new Foo...\n";
        std::unique_ptr<Foo, D> up(new Foo(), D());  // up 占有 Foo 指针（删除器 D ）
    
        std::cout << "Replace owned Foo with a new Foo...\n";
        up.reset(new Foo());  // 调用旧者的删除器
    
        std::cout << "Release and delete the owned Foo...\n";
        up.reset(nullptr);
    }

    // Tests for swap()
    {
        std::unique_ptr<Fooo> up1(new Fooo(1));
        std::unique_ptr<Fooo> up2(new Fooo(2));
    
        up1.swap(up2);
    
        std::cout << "up1->val:" << up1->val << std::endl;
        std::cout << "up2->val:" << up2->val << std::endl;
    }

    // Tests for get()
    {
        std::unique_ptr<string> s_p = "Hello, world!";
        string* s = s_p.get();
        std::cout << *s << endl;
    }

}